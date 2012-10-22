
#include "stdafx.h"
#include "minecraft_service.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "stdarg.h"

#include "../../shared/minecraft_shared.hpp"
#include "io_helpers.h"

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

void minecraft_service::invoke_world_switch(std::string inifile, std::string world1, std::string world2, std::string player) {
	std::stringstream stream;
	stream << "WorldSwitch.exe";
	stream << " " << inifile;
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
		invoke_world_switch(kIniFile, world1, world2, player);
	}
	else if(command == commands::say && numparams == 0) {
		return ResponseCommand(commands::say, player, list(1, "this is a server message"));
	}
	else if(command == commands::login && numparams == 0) {
		return ResponseCommand(commands::acknowledge, player, list(0));
	}
	else {
		std::cout << message << std::endl;
	}

	return "";
}





















