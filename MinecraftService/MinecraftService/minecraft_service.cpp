
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
#include "gcsv.h"
#include <boost/filesystem.hpp>

#include "../../shared/minecraft_shared.hpp"
#include "io_helpers.h"

typedef std::string str;

const std::string kExecutable = "WorldSwitch.exe";
const std::string kIniFile = "worldswitch.ini";
const std::string kWorldsFile = "worlds.csv";


std::deque<std::string> list( int Count, ... )
{
	va_list args;
	va_start(args, Count); 
	std::deque<std::string> list;
	for(int i = 0; i < Count; ++i )
		list.push_back(va_arg(args, std::string));
	va_end(args);
	return list;
}

const std::stringstream begin_command() {
	std::stringstream stream;
	stream << kExecutable << " " << kIniFile << " ";
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

// generates a response to send back to the client
std::string ResponseCommand(std::string command, std::string player, std::deque<std::string> params) {
	params.push_front(player);
	params.push_front(command);
	return MinecraftMessage(params).AsMessage();
}

// performs the given system call and returns the first line of output
std::string system_with_output(std::string command) {
	FILE* output = _popen(command.c_str(), "r");
	char buffer[1024];
	fgets(buffer, sizeof(buffer), output);
	_pclose(output);
	return std::string(buffer);
}

std::string InvokeCommand(std::string command, std::deque<std::string> params) {
	auto cmd = make_command(command, params);
	return system_with_output(cmd);
}

struct Coordinates {
	static const char delimiter = minecraft::kDelimiter2;
	double x;
	double y;
	double z;

	Coordinates(std::string x_, std::string y_, std::string z_) {
		x = atof(x_.c_str());
		y = atof(y_.c_str());
		z = atof(z_.c_str());
	}
	Coordinates(std::string str) {
		auto list = io_helpers::tokenize(str, delimiter);
		x = atof(list[0].c_str());
		y = atof(list[1].c_str());
		z = atof(list[2].c_str());
	}

	std::string ToString() {
		std::stringstream stream;
		stream << x << delimiter << y << delimiter << z;
		return stream.str();
	}

	bool Within(Coordinates other, double distance) {
		return (abs(other.x - x) < distance)
			&& (abs(other.y - y) < distance)
			&& (abs(other.z - z) < distance);
	}
};

void InvokeWorldSwitch(std::string player, std::string world1, std::string world2) {
	auto output = InvokeCommand(commands::worldswitch, list(3, player, world1, world2));
}

Coordinates InvokeGetCoordinates(std::string player, std::string world) {
	auto coords = InvokeCommand(commands::get_coords, list(2, player, world));
	return Coordinates(coords);
}

const double kCloseEnoughToTeleportFrom = 20;



std::string PackTeleportString(std::string world, std::string loc1, std::string loc2) {
	std::stringstream stream;
	stream << world << ":" << loc1 << ":" << loc2;
	return stream.str();
}

// What needs to happen for the player to teleport:
// client -> get_teleports -> server
// server:
//   get worlds
//   foreach world:
//     get coordinates
//     get all teleports
//	   filter for valid teleports
//     add to list
//   pack and return list of valid teleports
//   teleports formatted as  world:loc1:loc2
//   packed in pipe-delimited string
std::string InvokeGetTeleports(std::string player) {

	auto worlds = gcsv::read(kWorldsFile)->operator[]("worlds");
	auto it = worlds->begin();
	
	std::stringstream packed_teleports;

	BOOST_FOREACH(auto world, std::make_pair(worlds->begin(), worlds->end())){
		auto world_name = (*world)["name"];
		auto path = boost::filesystem::path((*world)["path"]);
		auto coords = InvokeGetCoordinates(player, world_name);
		auto teleports_path = path/"teleports.csv";

		vector_str valid_teleports;

		if(boost::filesystem::exists(teleports_path)) {
			auto teleports_csv = gcsv::read(teleports_path.string());
			auto locations = teleports_csv->get("locations");
			auto world_teleports = teleports_csv->get("teleports");
			for(auto tp = world_teleports->begin(); tp != world_teleports->end(); ++tp) {
				auto loc1 = locations->get(tp->get()->get("a"));
				auto loc1_name = locations->get(tp->get()->get("a"))->get("name");
				auto loc2_name = locations->get(tp->get()->get("b"))->get("name");
				auto coords1 = Coordinates(loc1->get("x"),loc1->get("y"),loc1->get("z"));
				if(coords1.Within(coords, kCloseEnoughToTeleportFrom)) {
					valid_teleports.push_back(PackTeleportString(world_name, loc1_name, loc2_name));
				}
			}
		} // after this, the valid_teleports list is populated.

		// pack the teleports into a pipe-delimited string to send to client
		foreach(teleport, valid_teleports) {
			packed_teleports << *teleport << minecraft::kDelimiter3;
		}
	}
	auto packed_string = packed_teleports.str();
	return packed_string.substr(0, packed_string.length() - 1); // remove the last dangling pipe
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
		InvokeWorldSwitch(player, world1, world2);
		return ResponseCommand(commands::say, player, list(1, str("Transferred inventory between worlds")));
	}
	else if(command == commands::get_teleports && numparams == 0) {
		auto teleports = InvokeGetTeleports(player);
		return ResponseCommand(commands::get_teleports_response, player, list(1, teleports));
	}
	else if(command == commands::login && numparams == 0) {
		return ResponseCommand(commands::menu_response, player, list(0));
	}
	else if(command == commands::menu && numparams == 0) {
		return ResponseCommand(commands::menu_response, player, list(0));
	}
	else {
		std::cout << message << std::endl;
	}

	return "";
}





















