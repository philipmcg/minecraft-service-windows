#include "StdAfx.h"
#include "variable_bin.h"

#include <string>
#include <iostream>
#include <fstream>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "io_helpers.h"

void test_variable_bin(){

	boost::shared_ptr<variable_bin> bin = boost::shared_ptr<variable_bin>(new variable_bin());
	bin->load_from_file("myfile.txt");
	bin->put_int("55", 55);
	bin->Int["54"] = 54;
	auto myint2 = bin->Int["54"];
	auto myint = bin->get_int("55");
	std::cout << myint << std::endl;
	std::cout << myint2 << std::endl;
	assert(myint2 == 54);
	assert(myint == 55);
	std::cout << bin->get_int("myint42") << std::endl;
	std::cout << bin->get_int("myint1") << std::endl;
}

variable_bin::variable_bin(void) :
	string_map(), int_map(), Int(&(this->int_map))
{
}

variable_bin::~variable_bin(void)
{
}

const char int_type_specifier = '#';
const char string_type_specifier = '$';

// splits the string at the first '='.  If the first character is $, the value will be stored as a string, otherwise as an integer.
void variable_bin::process_file_input(std::string line) {
	std::cout << line << std::endl;
	int indexOfEquals = line.find_first_of('=');
	if(indexOfEquals == std::string::npos)
		return;

	char type_specifier = line[0];
	auto key = io_helpers::trim(line.substr(1, indexOfEquals - 1));
	auto value = io_helpers::trim(line.substr(indexOfEquals + 1, line.length() - indexOfEquals - 1));

	if(type_specifier == int_type_specifier)
		put_int(key, atoi(value.c_str()));
	else if(type_specifier == string_type_specifier)
		put_string(key, value); 
}
void variable_bin::load_from_file(std::string path) {
	io_helpers::iterate_file(boost::bind(&variable_bin::process_file_input, shared_from_this(), _1), path);
}
std::string variable_bin::get_string(std::string key) {
	return string_map.find(key)->second;
}
int variable_bin::get_int(std::string key) {
	return int_map.find(key)->second;
}
void variable_bin::put_string(std::string key, std::string value) {
	string_map.insert(std::make_pair(key, value));
}
void variable_bin::put_int(std::string key, int value) {
	int_map.insert(std::make_pair(key, value));
}