/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#include <cstdio>
#include <iostream>
#include <fstream>
#include "Bruinbase.h"
#include "SqlEngine.h"
#include "BTreeIndex.h"

using namespace std;

#define index_file(name) name + ".idx"
#define table_file(name) name + ".tbl"

// external functions and variables for load file and sql command parsing 
extern FILE* sqlin;
int sqlparse(void);


RC SqlEngine::run(FILE* commandline)
{
  fprintf(stdout, "Bruinbase> ");

  // set the command line input and start parsing user input
  sqlin = commandline;
  sqlparse();  // sqlparse() is defined in SqlParser.tab.c generated from
               // SqlParser.y by bison (bison is GNU equivalent of yacc)

  return 0;
}

RC
SqlEngine::_preprocess_selcond(vector<SelCond>& condV, struct SelCond cond)
{
	for (vector<SelCond>::iterator it = condV.begin();
	     it != condV.end(); ++it) {
		if (cond.attr == 1 && it->comp == cond.comp) {
			int value = atoi(cond.value);
			switch(cond.comp) {
			case SelCond::LT:
			case SelCond::LE:
				// Choose more conservative
				it->value = value < it->intValue ?
					cond.value : it->value;
				it->intValue = value < it->intValue ?
					value : it->intValue;
				return 0;
			case SelCond::GT:
			case SelCond::GE:
				it->value = (value > it->intValue) ?
					cond.value : it->value;
				it->intValue = (value > it->intValue) ?
					value : it->intValue;
				return 0;
			case SelCond::EQ:
			case SelCond::NE:
				it->value = cond.value;
				it->intValue = value;
				return 0;
			default:
				goto push;
			}
		}
	}
 push:
	if (cond.attr == 1) {
		cond.intValue = atoi(cond.value);
	}
	condV.push_back(cond);

	return 0;
}

RC
SqlEngine::preprocess_selcond(vector<SelCond>& new_cond,
			      const vector<SelCond>& cond)
{
	for (vector<SelCond>::const_iterator it = cond.begin();
	     it != cond.end(); ++it) {
		_preprocess_selcond(new_cond, *it);
	}
	// for (vector<SelCond>::const_iterator it = new_cond.begin();
	//      it != new_cond.end(); ++it) {
	// 	cout << "attr: " << it->attr << ", comp: " << it->comp;
	// 	cout << ", value: " << it->value << endl;
	// }
}

RC
SqlEngine::select_from_index(BTreeIndex btIndex, int attr,
			     const string& table,
			     const vector<SelCond>& cond)
{
	int key;
	string value;
	RecordId rid;
	RecordFile rf;
	IndexCursor cursor;
	vector<SelCond> new_cond;

	preprocess_selcond(new_cond, cond);
}

RC SqlEngine::select(int attr, const string& table, const vector<SelCond>& cond)
{
  RecordFile rf;   // RecordFile containing the table
  RecordId   rid;  // record cursor for table scanning
  BTreeIndex btIndex;

  RC     rc;
  int    key;     
  string value;
  int    count;
  int    diff;

  // check if the index file exists?
  if (!btIndex.open(index_file(table), 'r')) {
	  return select_from_index(btIndex, attr, table, cond);
  }

  // open the table file
  if ((rc = rf.open(table + ".tbl", 'r')) < 0) {
    fprintf(stderr, "Error: table %s does not exist\n", table.c_str());
    return rc;
  }

  // scan the table file from the beginning
  rid.pid = rid.sid = 0;
  count = 0;
  while (rid < rf.endRid()) {
    // read the tuple
    if ((rc = rf.read(rid, key, value)) < 0) {
      fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
      goto exit_select;
    }

    // check the conditions on the tuple
    for (unsigned i = 0; i < cond.size(); i++) {
      // compute the difference between the tuple value and the condition value
      switch (cond[i].attr) {
      case 1:
	diff = key - atoi(cond[i].value);
	break;
      case 2:
	diff = strcmp(value.c_str(), cond[i].value);
	break;
      }

      // skip the tuple if any condition is not met
      switch (cond[i].comp) {
      case SelCond::EQ:
	if (diff != 0) goto next_tuple;
	break;
      case SelCond::NE:
	if (diff == 0) goto next_tuple;
	break;
      case SelCond::GT:
	if (diff <= 0) goto next_tuple;
	break;
      case SelCond::LT:
	if (diff >= 0) goto next_tuple;
	break;
      case SelCond::GE:
	if (diff < 0) goto next_tuple;
	break;
      case SelCond::LE:
	if (diff > 0) goto next_tuple;
	break;
      }
    }

    // the condition is met for the tuple. 
    // increase matching tuple counter
    count++;

    // print the tuple 
    switch (attr) {
    case 1:  // SELECT key
      fprintf(stdout, "%d\n", key);
      break;
    case 2:  // SELECT value
      fprintf(stdout, "%s\n", value.c_str());
      break;
    case 3:  // SELECT *
      fprintf(stdout, "%d '%s'\n", key, value.c_str());
      break;
    }

    // move to the next tuple
    next_tuple:
    ++rid;
  }

  // print matching tuple count if "select count(*)"
  if (attr == 4) {
    fprintf(stdout, "%d\n", count);
  }
  rc = 0;

  // close the table file and return
  exit_select:
  rf.close();
  return rc;
}

RC SqlEngine::load(const string& table, const string& loadfile, bool index)
{
	int key;
	string value, line;
	ifstream infile;
	RecordId rid = {0, 0};
	RecordFile rf;
	RC rc;
	BTreeIndex btIndex;

	infile.open(loadfile.c_str());
	if (!infile.is_open()) {
		fprintf(stderr, "Error: Failed to open %s\n", loadfile.c_str());
		return -1;
	}

	if ((rc = rf.open(table_file(table), 'w')) < 0) {
		fprintf(stderr, "Error: table %s does not exist\n", table.c_str());
		return rc;
	}

	if (index) {
		btIndex.open(index_file(table), 'w');
	}

	while(getline(infile, line)) {
		// TODO: Handle this gracefully.
		parseLoadLine(line, key, value);
		if ((rc = rf.append(key, value, rid)) < 0) {
			fprintf(stderr, "Error: Appending %d, %s failed\n",
				key, value.c_str());
			return rc;
		}
		if (index && (rc = btIndex.insert(key, rid))) {
			fprintf(stderr, "LOAD: BTreeIndex insert failed on "
				"key = %d with error = %d\n", key, rc);
			break;
		}
	}
	if (index && btIndex.close()) {
		fprintf(stderr, "LOAD, BTreeIndex close failed.\n");
	}
	rf.close();
	infile.close();
	return 0;
}

RC SqlEngine::parseLoadLine(const string& line, int& key, string& value)
{
    const char *s;
    char        c;
    string::size_type loc;
    
    // ignore beginning white spaces
    c = *(s = line.c_str());
    while (c == ' ' || c == '\t') { c = *++s; }

    // get the integer key value
    key = atoi(s);

    // look for comma
    s = strchr(s, ',');
    if (s == NULL) { return RC_INVALID_FILE_FORMAT; }

    // ignore white spaces
    do { c = *++s; } while (c == ' ' || c == '\t');
    
    // if there is nothing left, set the value to empty string
    if (c == 0) { 
        value.erase();
        return 0;
    }

    // is the value field delimited by ' or "?
    if (c == '\'' || c == '"') {
        s++;
    } else {
        c = '\n';
    }

    // get the value string
    value.assign(s);
    loc = value.find(c, 0);
    if (loc != string::npos) { value.erase(loc); }

    return 0;
}
