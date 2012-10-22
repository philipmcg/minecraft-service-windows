#pragma once

#include "stdafx.h"
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <map>
#include <boost/enable_shared_from_this.hpp>

class IntegerMap {
public: 
	IntegerMap(std::map<std::string,int>* map) {
		this->map_ = map;
	}
	int& operator[](const std::string& _Keyval) {
		// find element matching _Keyval or insert with default mapped
		auto _Where = this->map_->lower_bound(_Keyval);
		if (_Where == this->map_->end()
			|| this->map_->comp(_Keyval, this->map_->_Key(_Where._Mynode())))
			_Where = this->map_->insert(_Where,
				_STD pair<std::string, int>(
					_STD move(_Keyval),
					int()));
		return ((*_Where).second);
	}
private:
	std::map<std::string, int>* map_;
};


class variable_bin
	: public boost::enable_shared_from_this<variable_bin>
{
private:
	std::map<std::string, std::string> string_map;
	std::map<std::string, int> int_map;

public:
	IntegerMap Int;
	variable_bin(void);
	~variable_bin(void);

	void load_from_file(std::string path);
	void process_file_input(std::string line);
	std::string get_string(std::string key);
	int get_int(std::string key);
	void put_string(std::string key, std::string value);
	void put_int(std::string, int value);
};

