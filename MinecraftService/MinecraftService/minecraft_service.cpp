
#include "stdafx.h"
#include "minecraft_service.h"
#include <stdio.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "stdarg.h"

#include "../../shared/minecraft_shared.hpp"
#include "io_helpers.h"

const std::string kExecutable = "WorldSwitch.exe";
const std::string kIniFile = "worldswitch.ini";

std::deque<std::string> list( int Count, ... )
{
	va_list args;
	va_start(args, Count); 
	std::deque<std::string> list;
	for(int i = 0; i < Count; ++i )
		list.push_back(std::string(va_arg(args, char*)));
	va_end(args);
	return list;
}

const std::stringstream begin_command() {
	std::stringstream stream;
	stream << kExecutable << " " << kIniFile;
	return stream;
}

std::string make_command(std::string command, std::deque<std::string> params) {
	auto stream = begin_command();
	stream << command << " ";
	foreach(param, params) {
		stream << *param << " ";
	}
	return stream.str();
}

void minecraft_service::invoke_world_switch(std::string player, std::string world1, std::string world2) {
	auto stream = begin_command();
	stream << commands::worldswitch;
	stream << " " << world1;
	stream << " " << world2;
	stream << " " << player;
	auto systemcall = stream.str();
	system(systemcall.c_str());
}

std::string ResponseCommand(std::string command, std::string player, std::deque<std::string> params) {
	params.push_front(player);
	params.push_front(command);
	return MinecraftMessage(params).AsMessage();
}

std::string system_with_output(std::string command) {
	FILE* output = _popen(command.c_str(), "r");
	char buffer[1024];
	fgets(buffer, sizeof(buffer), output);
	_pclose(output);
	return std::string(buffer);
}

struct Coordinates {
	double x;
	double y;
	double z;

	Coordinates() {
		x = 1;
		y = 2;
		z = 3;
	}
};

Coordinates make_coords(std::string str) {
	auto list = io_helpers::tokenize(str, ':');
	Coordinates coords;
	coords.x = atof(list[0].c_str());
	coords.y = atof(list[1].c_str());
	coords.z = atof(list[2].c_str());
	return coords;
}

std::string GetPlayerCoordinates(std::string player, std::string world) {



	
	return "";
}

// if the message is in the right format, 
// this function invokes the WorldSwitch.exe with arguments from the message
std::string minecraft_service::handle_message(std::string message) {
	
	auto params = io_helpers::tokenize(message, ','); 

	if(params.size() == 0)
		return "";

	auto command = params[0];
	auto player = params[1];
	params.erase(params.begin());
	params.erase(params.begin());
	int numparams = params.size();
	
	if(command == commands::worldswitch && numparams == 2) {
		auto world1 = params[0];
		auto world2 = params[1];
		invoke_world_switch(player, world1, world2);
		return ResponseCommand(commands::say, player, list(1, "Transferred inventory between worlds"));
	}
	else if(command == commands::say && numparams == 0) {
		return ResponseCommand(commands::say, player, list(1, "this is a server message"));
	}
	else if(command == commands::login && numparams == 0) {
		GetPlayerCoordinates(player, "world1");
		return ResponseCommand(commands::acknowledge, player, list(0));
	}
	else {
		std::cout << message << std::endl;
	}

	return "";
}





















