#include "BTreeNode.h"
#include <iostream>

using namespace std;

int main() {
	BTNonLeafNode* test = new BTNonLeafNode();
	PageId pid1 = 10;
	PageId pid2 = 15;
	int testKey = 50;

	test->initializeRoot(pid1, testKey, pid2);
	test->printBuffer();

	int keyCount = test->getKeyCount();
	cout << keyCount << endl;

	int testKey2 = 60;
	PageId pid3 = 30;
	test->insert(testKey2, pid3);
	cout << endl;
	test->printBuffer();

	int testKey3 = 30;
	PageId pid4 = 50;
	test->insert(testKey3, pid4);
	cout << endl;
	test->printBuffer();
	return 0;
}
