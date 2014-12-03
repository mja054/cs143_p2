#include "BTreeNode.h"
#include <iostream>

using namespace std;

int main() {
	BTLeafNode* test = new BTLeafNode();
	RecordId rid1 = {5, 9};
	int testKey1 = 50;

	// Testing getKeyCount
	int keyCount = test->getKeyCount();
	cout << keyCount << endl;

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

	/*
	// Testing insertAndSplit
	int testKey4 = 40;
	PageId pid5 = 12;
	BTLeafNode* testSplit = new BTLeafNode();
	int testKey5 = 100;
	test->insertAndSplit(testKey4, pid5, *testSplit, testKey5);
	cout << endl;
	test->printBuffer();
	cout << endl;
	testSplit->printBuffer();
	cout << endl;
	cout << "Return key is " << testKey5 << endl;

	// Testing locateChildPtr
	int searchKey = 35;
	PageId pid6 = 100;
	test->locateChildPtr(searchKey, pid6);
	cout << "Return pid is " << pid6 << endl;
	*/
	return 0;
}

