#include "BTreeNode.h"
#include <iostream>

using namespace std;

int main() {
	BTNonLeafNode* test = new BTNonLeafNode();
	PageId pid1 = 10;
	PageId pid2 = 5;
	int testKey = 50;

	// Testing initializeRoot
	test->initializeRoot(pid1, testKey, pid2);
	test->printBuffer();

	// Testing getKeyCount
	int keyCount = test->getKeyCount();
	cout << keyCount << endl;

	// Testing insert
	int testKey2 = 60;
	PageId pid3 = 3;
	test->insert(testKey2, pid3);
	cout << endl;
	test->printBuffer();

	int testKey3 = 30;
	PageId pid4 = 5;
	test->insert(testKey3, pid4);
	cout << endl;
	test->printBuffer();

	// Testing insertAndSplit
	int testKey4 = 40;
	PageId pid5 = 12;
	BTNonLeafNode* testSplit = new BTNonLeafNode();
	int testKey5 = 100;
	test->insertAndSplit(testKey4, pid5, *testSplit, testKey5);
	cout << endl;
	test->printBuffer();
	cout << endl;
	testSplit->printBuffer();
	cout << endl;
	cout << "Return key is " << testKey5 << endl;

	// Testing locateChildPtr
	
	return 0;
}
