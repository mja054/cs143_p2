#include "BTreeNode.h"
#include <stdio.h>

using namespace std;


/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf)
{
	return pf.read(pid, this->buffer);
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf)
{
	return pf.write(pid, this->buffer);
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{
	int count = 0;
	char *tbuffer = this->buffer;

	/*
	 * Logic: We assume that the page is initialized to zero
	 * and key value will never be 0.
	 */
	while (*(int *) tbuffer && (count < 70)) {
		count++;
		tbuffer += (RECORD_ID_SIZE + KEY_SIZE);
	}

	return count;
}

RC BTLeafNode::_insert(char *buffer, int count, int key, const RecordId& rid)
{
	int i = 0;

	while (count && (key < *(int *) buffer)) {
		memcpy(buffer + (RECORD_ID_SIZE + KEY_SIZE), buffer,
		       RECORD_ID_SIZE + KEY_SIZE);
		buffer -= (RECORD_ID_SIZE + KEY_SIZE);
		count--;
	}

	memcpy(buffer, &key, KEY_SIZE);
	memcpy(buffer + KEY_SIZE, &rid, RECORD_ID_SIZE);
	return 0;
}

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid)
{
	int count = getKeyCount();
	char *tbuffer = this->buffer;

	if (count == MAX_LEAF_KEY_COUNT) {
		return RC_NODE_FULL;
	} else if (count == 0) {
		memcpy(tbuffer, &key, KEY_SIZE);
		memcpy(tbuffer + KEY_SIZE, &rid, RECORD_ID_SIZE);
		return 0;
	}
	return _insert(tbuffer + (KEY_SIZE + RECORD_ID_SIZE) * (count - 1),
		       count, key, rid);
}

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, 
                              BTLeafNode& sibling, int& siblingKey)
{ return 0; }

/*
 * Find the entry whose key value is larger than or equal to searchKey
 * and output the eid (entry number) whose key value >= searchKey.
 * Remeber that all keys inside a B+tree node should be kept sorted.
 * @param searchKey[IN] the key to search for
 * @param eid[OUT] the entry number that contains a key larger than or equalty to searchKey
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::locate(int searchKey, int& eid)
{ return 0; }

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid)
{
	char *tbuffer;

	if (eid == 0) {
		tbuffer = this->buffer;
	} else {
		tbuffer = this->buffer + (KEY_SIZE + RECORD_ID_SIZE) * (eid - 1);
	}

	memcpy(&key, tbuffer, KEY_SIZE);
	memcpy(&rid, tbuffer + KEY_SIZE, RECORD_ID_SIZE);

	return 0;
}

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{
	return *(int *) (this->buffer + PAGE_SIZE - PAGE_ID_SIZE - 1);
}

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{
	*(int *) (this->buffer + PAGE_SIZE - PAGE_ID_SIZE - 1) = pid;
}

/* ------------------------------------------------------------------- */

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf)
{ 
	RC rc = pf.read(pid, this->buffer);
	return rc;
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)
{
	RC rc = pf.write(pid, this->buffer);
	return rc;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount()
{
	int ind = 0;
	int numKeys = 0;

	int tmpKey = -1;
	PageId tmpPid = 0;

	// assumes buffer has been initialized to 0
	while(tmpKey != 0) {
		// Parse the pid first and increment the index
		memcpy(&tmpPid, this->buffer + ind, sizeof(PageId));
		ind += sizeof(PageId);

		// Parse the key next and increment the index
		memcpy(&tmpKey, this->buffer + ind, sizeof(int));
		ind += sizeof(int);

		// Check to see if key value is 0 or if we have run out of keys
		if (tmpKey == 0) {
			memcpy(&tmpPid, this->buffer + ind, sizeof(PageId));
			// If next pid is valid (aka not 0), continue parsing
			if (tmpPid != 0) {
				numKeys++;
				continue;
			}
		}
		else
			numKeys++;
	}
	return numKeys;
}


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{ return 0; }

/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{ return 0; }

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{ 
	PageId currPid = 0;
	int currKey = -1;
	int ind = 0;

	while(currKey != 0) {
		// Parse the pid first and increment the index
		memcpy(&currPid, this->buffer + ind, sizeof(PageId));
		ind += sizeof(PageId);

		// Parse the key next and increment the index
		memcpy(&currKey, this->buffer + ind, sizeof(int));
		ind += sizeof(int);

		if ((searchKey < currKey) && (currKey != 0)) {
			pid = currPid;
			return 0;
		}
	}

	// break out of loop if currKey is 0
	// assign last known pid to the output
	pid = currPid;
	return 0;
}

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{
	// initialize the buffer to 0's
	memset(this->buffer, 0, PageFile::PAGE_SIZE);

	// copy the inputs into the buffer
	memcpy(this->buffer, &pid1, sizeof(PageId));
	memcpy(this->buffer + sizeof(PageId), &key, sizeof(int));
	memcpy(this->buffer + sizeof(PageId) + sizeof(int), &pid2, sizeof(PageId));

	return 0; 
}

