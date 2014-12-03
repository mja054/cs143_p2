/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#include "BTreeIndex.h"
#include "BTreeNode.h"

using namespace std;
#define SUCCESS 0
/*
 **********************************************************
 * BTreeIndex metadata format:
 * ===========================
 *
 * -----------------------------------------
 * |  rootPid  |  treeHeight  |         |  |
 * -----------------------------------------
 *    4Bytes       4Bytes
 *
 **********************************************************
 */

int BTreeIndex::fetch_new_page()
{
	PageId pid = pf.endPid();

	memset(buffer, 0xff, PageFile::PAGE_SIZE);
	pf.write(pid, buffer);

	return pid;
}


int BTreeIndex::read_metadata()
{
	int *ptr = (int *) buffer;

	if (!pf.read(BTINDEX_MD_PID, buffer)) {
		rootPid = *ptr;
		treeHeight = *(ptr + 1);
		return 0;
	} else {
		return -1;
	}
}

int BTreeIndex::commit_metadata()
{
	int *ptr = (int *) buffer;

	*ptr = rootPid;
	*(ptr + 1) = treeHeight;

	return pf.write(BTINDEX_MD_PID, buffer);
}

void BTreeIndex::printTree() {
	queue<PageId> bfs;
	bfs.push(rootPid);
	bfs.push(-1);
	BTNonLeafNode currNonLeafNode;
	BTLeafNode currLeafNode;
	int currHeight = 1;
	PageId currPid = -1;
	while(!bfs.empty()) {
		currPid = bfs.front();
		bfs.pop();
		if (currPid == -1) {
			cout << endl;
			if (bfs.empty())
				break;
			bfs.push(-1);
			currHeight++;
		}
		else {
			if (currHeight == treeHeight) {
				currLeafNode.read(currPid, pf);
				currLeafNode.printBuffer();
				cout << "||";
			}
			else {
				currNonLeafNode.read(currPid, pf);
				currNonLeafNode.printBuffer(bfs);
				cout << "||";
			}

		}
	}
	return;
}

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
    rootPid = -1;
    treeHeight = 0;
}

/*
 * Open the index file in read or write mode.
 * Under 'w' mode, the index file should be created if it does not exist.
 * @param indexname[IN] the name of the index file
 * @param mode[IN] 'r' for read, 'w' for write
 * @return error code. 0 if no error
 */
RC BTreeIndex::open(const string& indexname, char mode)
{
	pf.open(indexname, mode);
	if (pf.endPid() == 0) {
		rootPid = -1;
		treeHeight = 0;
	} else {
		read_metadata();
	}
	return 0;
}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close()
{
	commit_metadata();
	return pf.close();
}

RC
BTreeIndex::_insert(int pid, int depth, int key, const RecordId& rid,
		    int &splitkey, int &splitpid)
{
	RC ret;

	// Leaf nodes
	if (depth == treeHeight) {
		BTLeafNode node;
		node.read(pid, pf);
		ret = node.insert(key, rid);
		if (ret == RC_NODE_FULL) {
			BTLeafNode sibling;

			splitpid = fetch_new_page();
			node.insertAndSplit(key, rid, sibling, splitkey);
			sibling.write(splitpid, pf);
			node.setNextNodePtr(splitpid);
		}
		node.write(pid, pf);
	} else {
	// NonLeaf nodes
		PageId n_pid;
		BTNonLeafNode node;
		node.read(pid, pf);
		ret = node.locateChildPtr(key, n_pid);
		if (ret) {
			return ret;
		}

		ret = this->_insert(n_pid, depth + 1, key,
				    rid, splitkey, splitpid);
		if (ret == SUCCESS) {
			// Do nothing
		} else if (ret == RC_NODE_FULL) {
			ret = node.insert(splitkey, splitpid);
			if (ret == RC_NODE_FULL) {
				PageId new_pid = fetch_new_page();
				BTNonLeafNode sibling;
				int midkey;
				node.insertAndSplit(splitkey, splitpid,
						    sibling, midkey);
				splitkey = midkey;
				splitpid = new_pid;
				sibling.write(new_pid, pf);
			}
			node.write(pid, pf);
		}
	}
	return ret;
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid)
{
	RC ret;
	int splitkey = -1, splitpid = -1;

	if (treeHeight == 0) {
		treeHeight = 1;
		fetch_new_page();
		rootPid = fetch_new_page();
	}

	ret = this->_insert(rootPid, 1, key, rid, splitkey, splitpid);
	// check if there was a split, we should create new node
	// and initialize it as root, and also update rootPid
	if (ret == RC_NODE_FULL) {
		PageId new_pid = fetch_new_page();
		BTNonLeafNode root_node;

		root_node.initializeRoot(rootPid, splitkey, splitpid);
		rootPid = new_pid;
		root_node.write(new_pid, pf);
		treeHeight++;
	}

	return 0;
}


/*
 * Find the leaf-node index entry whose key value is larger than or
 * equal to searchKey, and output the location of the entry in IndexCursor.
 * IndexCursor is a "pointer" to a B+tree leaf-node entry consisting of
 * the PageId of the node and the SlotID of the index entry.
 * Note that, for range queries, we need to scan the B+tree leaf nodes.
 * For example, if the query is "key > 1000", we should scan the leaf
 * nodes starting with the key value 1000. For this reason,
 * it is better to return the location of the leaf node entry
 * for a given searchKey, instead of returning the RecordId
 * associated with the searchKey directly.
 * Once the location of the index entry is identified and returned
 * from this function, you should call readForward() to retrieve the
 * actual (key, rid) pair from the index.
 * @param key[IN] the key to find.
 * @param cursor[OUT] the cursor pointing to the first index entry
 *                    with the key value.
 * @return error code. 0 if no error.
 */
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor)
{
    return 0;
}

/*
 * Read the (key, rid) pair at the location specified by the index cursor,
 * and move foward the cursor to the next entry.
 * @param cursor[IN/OUT] the cursor pointing to an leaf-node index entry in the b+tree
 * @param key[OUT] the key stored at the index cursor location.
 * @param rid[OUT] the RecordId stored at the index cursor location.
 * @return error code. 0 if no error
 */
RC BTreeIndex::readForward(IndexCursor& cursor, int& key, RecordId& rid)
{
    return 0;
}
