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
#include <boost/function.hpp>
#include <fstream>
#include <vector>

typedef std::vector<std::string> vector_str;

namespace io_helpers {

	const std::string comment = "//";
	const std::string whitespace_chars = " \t";

	vector_str tokenize(std::string str, char delimiter);

	// returns true if the line does not begin with a comment.
	inline bool is_valid_line(const std::string& str) {
		if(str.length() == 0 || str.find_first_of(comment) == 0)
			return false;
		else
			return true;
	}

	inline std::string trim(const std::string& str,
					 const std::string& whitespace = whitespace_chars) 
	{
		const auto strBegin = str.find_first_not_of(whitespace);
		if (strBegin == std::string::npos)
			return ""; // no content

		const auto strEnd = str.find_last_not_of(whitespace);
		const auto strRange = strEnd - strBegin + 1;

		return str.substr(strBegin, strRange);
	}

	inline void iterate_file(boost::function<void(std::string)> func, std::string path) {
		std::ifstream myfile(path);
		const int buffer_length = 1024;
		char buffer[buffer_length];
		if(myfile.is_open()) {
			while(myfile.good()) {
				myfile.getline(buffer, buffer_length);
				std::string str(buffer);
				std::string trimmed = trim(str);
				if(is_valid_line(trimmed))
					func(trimmed);
			}
			myfile.close();
		}
		else {
			throw std::exception("failed to open file");
		}
	}
}