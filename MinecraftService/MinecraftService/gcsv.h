#pragma once

#include "stdafx.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>
#include <assert.h>

typedef std::vector<std::string> vector_str;

class GcsvTableCollection;

namespace gcsv {
	const char kGcsvInitialCharacter = '~';
	void test_gcsv();
	std::shared_ptr<GcsvTableCollection> read(std::string file);
}

class GcsvHeader {
public:
	GcsvHeader(std::string name, const vector_str& columns);
	~GcsvHeader();
	int operator[](const std::string& key);
	bool ContainsKey(const std::string& key);
	int size();
	std::string name(); 
	std::string GetFirstColumnName();

private:
	std::map<std::string, int> fields_;
	std::string name_;
	std::string first_field_;
};

// http://stackoverflow.com/a/627663
template<typename T>
struct my_array_deleter
{
   void operator()(T* p)
   {
      delete [] p;
   }
};
typedef std::shared_ptr<GcsvHeader> GcsvHeaderPtr;

// GcsvLine is a glorified map<string,string>.
// The values are stored in a simple array, and indices are retrieved through the header
// which maps column names to integer indices.
class GcsvLine {
public:
	GcsvLine(std::shared_ptr<GcsvHeader> header, const vector_str& fields); 
	~GcsvLine();
	std::string operator[](const std::string& key);
	std::string get(const std::string& key);

private:
	std::shared_ptr<std::string> values_;
	std::shared_ptr<GcsvHeader> header_;
};
typedef std::shared_ptr<GcsvLine> GcsvLinePtr;


// GcsvTable has a vector of GcsvLines.  If you had a single gcsv in a file, reading the file would yield a single GcsvTable.
// The table has a header, which all of the lines will point to.  The lines do NOT contain a reference to the GcsvTable they are in.
// The table can go out of scope and be cleaned up, even if some lines are still referenced in user code.
//
// The table has a mapping of keys to lines -- the keys are the first field in each line.
// So if I read the following GCSV:
// ~test_name,row_name,col1,col2
// row1,r1c1,r1c2
// row2,r2c1,r2c2
//  the mapping_key will be "row_name", and if I say mytable["row1"] I will get the GcsvLine for row 1.
class GcsvTable {
public:
	GcsvTable(std::shared_ptr<GcsvHeader> header);
	~GcsvTable();
	void Add(GcsvLinePtr line);
	bool ContainsKey(const std::string& key);
	GcsvLinePtr operator[](const std::string& key);
	GcsvLinePtr get(const std::string& key);
	std::string name();
	std::vector<GcsvLinePtr>::iterator begin();
	std::vector<GcsvLinePtr>::iterator end();
	std::shared_ptr<GcsvHeader> header();

private:
	// the mapping_key is the column used for creating the lines_map.  
	std::string mapping_key_;
	std::string name_;
	std::shared_ptr<GcsvHeader> header_;
	std::map<std::string, GcsvLinePtr> lines_map_;
	std::vector<GcsvLinePtr> lines_;
};
typedef std::shared_ptr<GcsvTable> GcsvTablePtr;

// A GcsvTableCollection is a table of GcsvTables.  This is what you get
// after reading a gcsv file, since one file may have several GcsvTables in it.
// Tables can be accessed by name using the [] operator.
class GcsvTableCollection {
public:
	GcsvTableCollection(); 
	~GcsvTableCollection();
	std::shared_ptr<GcsvTable> operator[](const std::string& key);
	std::shared_ptr<GcsvTable> get(const std::string& key);
	void AddTable(std::shared_ptr<GcsvTable> table);
private:
	std::map<std::string, std::shared_ptr<GcsvTable>> tables_;
};
typedef std::shared_ptr<GcsvTableCollection> GcsvTableCollectionPtr;