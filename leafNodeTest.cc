#include "BTreeNode.h"
#include <iostream>

using namespace std;

int main() {
	BTLeafNode* test = new BTLeafNode();
	RecordId rid1 = {5, 9};
	int testKey1 = 50;

	// Testing insert
	test->insert(testKey1, rid1);
	cout << endl;
	test->printBuffer();

	int testKey2 = 30;
	RecordId rid2 = {3, 5};
	test->insert(testKey2, rid2);
	cout << endl;
	test->printBuffer();

	int testKey3 = 40;
	RecordId rid3 = {2, 7};
	test->insert(testKey3, rid3);
	cout << endl;
	test->printBuffer();

	// Testing insertAndSplit and setNextNodePtr
	PageId setPtr = 68;
	test->setNextNodePtr(setPtr);
	PageId oldNextNode = test->getNextNodePtr();
	int testKey4 = 45;
	RecordId rid4 = {3, 12};
	BTLeafNode* testSplit = new BTLeafNode();
	int testKey5 = 100;
	test->insertAndSplit(testKey4, rid4, *testSplit, testKey5);
	cout << endl;
	test->printBuffer();
	cout << endl;
	testSplit->printBuffer();
	cout << endl;
	cout << "Return key is " << testKey5 << endl;

	// Testing getNextNodePtr
	PageId newNextNode = testSplit->getNextNodePtr();
	cout << "Old next node is " << oldNextNode << endl;
	cout << "New next node is " << newNextNode << endl;
	PageId newOldNextNode = test->getNextNodePtr();
	// Should return -1, need to set this in 2c
	cout << "New old next node is " << newOldNextNode << endl;

	// Testing getKeyCount
	int keyCount = test->getKeyCount();
	cout << keyCount << endl;

	// Testing locate
	int searchKey = 45;
	int eid = 100;
	test->locate(searchKey, eid);
	cout << "Return eid is " << eid << endl;
	testSplit->locate(searchKey, eid);
	cout << "Return eid is " << eid << endl;

	// Testing readEntry
	int readKey = 100;
	RecordId readRid = {100, 100};
	test->readEntry(0, readKey, readRid);
	cout << "Return readKey is " << readKey << endl;
	cout << "Return readRid pid is " << readRid.pid << endl;
	cout << "Return readRid sid is " << readRid.sid << endl;
	test->readEntry(1, readKey, readRid);
	cout << "Return readKey is " << readKey << endl;
	cout << "Return readRid pid is " << readRid.pid << endl;
	cout << "Return readRid sid is " << readRid.sid << endl;

	return 0;
}

