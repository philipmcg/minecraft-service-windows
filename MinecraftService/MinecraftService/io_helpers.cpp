

#include "stdafx.h"
#include <cstdlib>
#include <map>
#include <vector>


namespace io_helpers {

	// Converts: 
	// "a,b,c"   --> ["a", "b", "c"]
	// "a,b,c,"  --> ["a", "b", "c"]
	// "a,,b,c," --> ["a", "", "b", "c"]
	// ","       --> []
	// ""        --> []
	std::vector<std::string> tokenize(std::string text, char delimiter) {
		
		std::vector<std::string> split;

		int index = text.find_first_of(delimiter);
		while(std::string::npos != index) {
			split.push_back(text.substr(0, index));
			text = text.substr(index + 1, text.length() - index - 1);
			index = text.find_first_of(delimiter);
		}

		if(text.length() > 0)
			split.push_back(text);
		
		return split;
	}
}
