
#include "stdafx.h"
#include <algorithm>
#include <cstdlib>
#include <array>
#include <boost/bind.hpp>

#include "gcsv.h"

#include "io_helpers.h"

class GcsvReader {
public:
	GcsvReader() : has_table_(false), collection_(new GcsvTableCollection()), name_("") {
	}

	void HandleLine(std::string line) {
		// here's where we handle each input line when reading a gcsv file !!

		auto tokens = io_helpers::tokenize(line, ',');

		// If this is the start of a gcsv
		if(tokens[0][0] == gcsv::kGcsvInitialCharacter) {
			// If we had already started a gcsv, this must be the next one,
			// so we return the one we were filling up.
			if(has_table_) {
				collection_->AddTable(table_);
			}
			name_ = tokens.at(0).substr(1, tokens[0].length() - 1);

			// If the gcsv header is defined on the same line as the name
			// the line will have multiple delimited values and the second
			// value, which is the first field name in the header, will
			// not be empty.
			if(tokens.size() > 1 && tokens[1].length() > 0)
			{
				tokens.erase(tokens.begin());
				StartGcsv(tokens);
			}
		}
		// If we've already started a gcsv, we can add this line to it.
		else if(has_table_) {
			auto header = table_->header();
			if(tokens.size() < header->size()) {
				for(int k = tokens.size(); k < header->size(); k++) {
					tokens.push_back("");
				}
			}
			table_->Add(std::shared_ptr<GcsvLine>(new GcsvLine(header, tokens)));
		}
		else {
			// If we've read the name of the gcsv, but not the header
			// then this line is the header.
			if(name_.empty()) {
				StartGcsv(tokens);
			}
		}
	}

	// Called when a new Gcsv header has been read.  
	// Creates a new GcsvTable with the given header and the current name.
	void StartGcsv(const vector_str& tokens) {
		vector_str header_tokens;
		for(auto it = tokens.begin(); it < tokens.end(); it++) {
			if(!(*it).empty())
				header_tokens.push_back(*it);
		}
		auto header = std::shared_ptr<GcsvHeader>(new GcsvHeader(name_, header_tokens));
		table_ = std::shared_ptr<GcsvTable>(new GcsvTable(header));
		has_table_ = true;
	}

	// Called when the file is all read.
	// This closes the last gcsv and adds it to the table.
	void Finish() {
		if(has_table_) {
			collection_->AddTable(table_);
			has_table_ = false;
		}
	}

	std::shared_ptr<GcsvTableCollection> GetTableCollection() { return collection_; }
	
private:
	std::shared_ptr<GcsvTableCollection> collection_;
	std::shared_ptr<GcsvTable> table_;
	bool has_table_;
	std::string name_;
};


// unit test and IO function
namespace gcsv {

	void test_gcsv() {
		std::cout << "testing gcsv..." << std::endl;
		{
			int numtimes = 1;
			for(int i = 0; i < numtimes; i++) {
				auto file = "gcsv_sample.csv";
				auto table_collection = gcsv::read(file);
				assert((*table_collection)["test_name"]);
				auto x = (*table_collection)["test_name"];

				for(auto it = x->begin(); it != x->end(); ++it) {
					std::cout << (**it)["col1"] << std::endl;
				}
			}
		}
		// out of scope, all gcsv objects should be cleaned up
		std::cout << "finished testing gcsv" << std::endl;
	}
	
	std::shared_ptr<GcsvTableCollection> read(std::string path) {
		auto reader = boost::shared_ptr<GcsvReader>(new GcsvReader());

		// To read the Gcsv, we iterate over all of the trimmed, valid lines in the file
		// using the GcsvReader to add them to a new GcsvTableCollection.
		io_helpers::iterate_file(boost::bind(&GcsvReader::HandleLine, reader, _1), path);
		reader->Finish();

		return reader->GetTableCollection();
	}
}




//////////////////// GcsvHeader Implementation

// Adds the columns to the mapping in order.
GcsvHeader::GcsvHeader(std::string name, const vector_str& columns) : fields_(), name_(name) {
	int count = 0;
	for(auto it = columns.begin(); it != columns.end(); ++it) {
		fields_.insert(std::make_pair (*it, count));
		count++;
	}
	first_field_ = columns.at(0);
}
GcsvHeader::~GcsvHeader() {
	//std::cout << " deleting GcsvHeader " << name() << std::endl;
}
int GcsvHeader::operator[](const std::string& key) {
	return fields_.find(key)->second;
}
bool GcsvHeader::ContainsKey(const std::string& key) {
	return fields_.count(key) == 0;
}
int GcsvHeader::size() { 
	return fields_.size();
}
std::string GcsvHeader::name() {
	return name_;
}
std::string GcsvHeader::GetFirstColumnName() {
	return first_field_;
}




//////////////////// GcsvLine Implementation

// values are copied from the input vector
GcsvLine::GcsvLine(std::shared_ptr<GcsvHeader> header, const vector_str& fields) {
	header_ = header;
	values_ = std::shared_ptr<std::string>(new std::string[header->size()], my_array_deleter<std::string>());
	std::string* values_ptr = values_.get();
	for(int i = 0; i < fields.size(); i++) {
		values_ptr[i] = fields.at(i);
	}
	for(int i = fields.size(); i < header->size(); i++) {
		values_ptr[i] = std::string("");
	}
}

// values_ should be automatically deleted at this point
// header_ may be shared by other GcsvLines.
GcsvLine::~GcsvLine() {
	//std::cout << " deleting GcsvLine " << (values_.get())[0] << std::endl;
}

std::string GcsvLine::operator[](const std::string& key) {
	int index = (*header_)[key];
	assert(index >= 0 && index < header_->size());
	return values_.get()[index];
}





//////////////////// GcsvTable Implementation

GcsvTable::GcsvTable(std::shared_ptr<GcsvHeader> header) : name_(header->name()), header_(header), lines_map_(), lines_() {
	mapping_key_ = header->GetFirstColumnName();
}
GcsvTable::~GcsvTable() {
	//std::cout << " deleting GcsvTable " << name() << std::endl;
}
void GcsvTable::Add(GcsvLinePtr line) {
	lines_map_.insert(std::make_pair((*line)[mapping_key_], line));
	lines_.push_back(line);
}

bool GcsvTable::ContainsKey(const std::string& key) {
	return lines_map_.count(key) > 0;
}

GcsvLinePtr GcsvTable::operator[](const std::string& key) {
	return lines_map_.find(key)->second;
}

std::string GcsvTable::name() { 
	return name_;
}

std::vector<GcsvLinePtr>::iterator GcsvTable::begin() {
	return lines_.begin();
}

std::vector<GcsvLinePtr>::iterator GcsvTable::end() {
	return lines_.end();
}

std::shared_ptr<GcsvHeader> GcsvTable::header() { return header_; }




//////////////////// GcsvTableCollection Implementation

GcsvTableCollection::GcsvTableCollection() : tables_() { }
GcsvTableCollection::~GcsvTableCollection() {
	//std::cout << " deleting GcsvTableCollection " << std::endl;
}

std::shared_ptr<GcsvTable> GcsvTableCollection::operator[](const std::string& key) {
	return tables_.find(key)->second;
}

void GcsvTableCollection::AddTable(std::shared_ptr<GcsvTable> table) {
	tables_.insert(std::make_pair(table->name(), table));
}