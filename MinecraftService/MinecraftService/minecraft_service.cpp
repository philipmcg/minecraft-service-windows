
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
#include "gcsv_worlds.h"

typedef std::string str;
typedef std::vector<std::pair<std::string,std::string>> vector_pair;

// the below three files will be in the same directory as this executable
const std::string kExecutable = "WorldSwitch.exe";
const std::string kIniFile = "worldswitch.ini";
const std::string kWorldsFile = "worlds.csv"; 

// file specifications from minecraft server itself
const std::string kPlayerFileExtension = ".dat"; 
const std::string kPlayersDirectory = "players"; 

// this will be in the world directory for each world
const std::string kTeleportsFile = "teleports.csv"; 

// Packs variable number of arguments into std::deque.
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

// Returns the beginning of a WorldSwitch command, with the exe and ini file specified.
const std::stringstream begin_command() {
	std::stringstream stream;
	stream << kExecutable << " " << kIniFile << " ";
	return stream;
}

// Returns a full WorldSwitch command, with the exe, ini file, 
// command and parameters ready to be sent to the system call.
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

std::string GetPlayerFile(std::string world_path, std::string player) {
	auto path = boost::filesystem::path(world_path);
	path /= kPlayersDirectory;
	path /= (player + kPlayerFileExtension);
	return path.string();
}

bool PlayerIsInWorld(std::string world_path, std::string player) {
	return boost::filesystem::exists(GetPlayerFile(world_path, player));
}

// returns all valid pairs of worlds for the player to switch between
vector_pair GetWorldsToSwitch(std::string player) {
	auto worlds = WorldData::LoadWorldsFromFile(kWorldsFile);
	vector_pair pairs;
	std::vector<str> valid_worlds;
	
	BOOST_FOREACH(auto world, worlds)  {
		if(PlayerIsInWorld(world->path(), player))
			valid_worlds.push_back(world->name());
	}
	for(auto it = valid_worlds.begin(); it != valid_worlds.end(); ++it) {
		for(auto k = it; k != valid_worlds.end(); ++k) {
			if(*it != *k) {
				pairs.push_back(std::make_pair(*it, *k));
			}
		}
	}
	return pairs;
}


std::string GetPackedWorldsToSwitch(std::string player) {
	auto pairs = GetWorldsToSwitch(player);
	std::stringstream stream;
	BOOST_FOREACH(auto pair, pairs) {
		stream << pair.first << minecraft::kDelimiter2 << pair.second;
		stream << minecraft::kDelimiter3;
	}
	auto packed_string = stream.str();
	return packed_string.substr(0, packed_string.length() - 1); // remove the last dangling delimiter
	return packed_string;
}

void InvokeWorldSwitch(std::string player, WorldSwitch worldswitch) {
	auto output = InvokeCommand(commands::worldswitch, list(3, player, worldswitch.World1, worldswitch.World2));
}
Coordinates InvokeGetCoordinates(std::string player, std::string world) {
	auto coords = InvokeCommand(commands::get_coords, list(2, player, world));
	return Coordinates(coords);
}

const double kCloseEnoughToTeleportFrom = 20;


bool PlayerIsNearTeleport(std::string player) {

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
std::vector<TeleportPair> InvokeGetTeleports(std::string player) {

	auto worlds = WorldData::LoadWorldsFromFile(kWorldsFile);

	std::stringstream packed_teleports;
	std::vector<TeleportPair> teleports;

	BOOST_FOREACH(auto world, worlds) { 
		if(!PlayerIsInWorld(world->path(), player)) 
			continue;

		auto world_name = world->name();
		auto path = boost::filesystem::path(world->path());
		auto player_coords = InvokeGetCoordinates(player, world_name);
		auto teleports_path = path/"teleports.csv";

		if(boost::filesystem::exists(teleports_path)) {
			auto teleports_csv = gcsv::read(teleports_path.string());
			auto locations = teleports_csv->get("locations");
			auto world_teleports = teleports_csv->get("teleports");
			for(auto tp = world_teleports->begin(); tp != world_teleports->end(); ++tp) {
				auto loc1 = locations->get(tp->get()->get("a"));
				auto loc2 = locations->get(tp->get()->get("b"));
				auto loc1_name = locations->get(tp->get()->get("a"))->get("name");
				auto loc2_name = locations->get(tp->get()->get("b"))->get("name");
				auto coords1 = Coordinates(loc1->get("x"),loc1->get("y"),loc1->get("z"));
				auto coords2 = Coordinates(loc2->get("x"),loc2->get("y"),loc2->get("z"));
				if(coords1.Within(player_coords, kCloseEnoughToTeleportFrom)) {
					teleports.push_back(TeleportPair(world_name, loc1_name, loc2_name, coords1, coords2));
				}
			}
		} // after this, the teleports vector is populated.
	}
	return teleports;
}

bool InvokeTeleport(std::string player, TeleportPair teleport) {

	auto teleports = InvokeGetTeleports(player);
	foreach(possible_teleport, teleports) {
		if(possible_teleport->Equals(teleport)) {
			auto output = InvokeCommand(commands::teleport, list(3, player, possible_teleport->World, possible_teleport->Teleport2.Coords.ToString()));
			return true;
		}
	}
	return false;
}

//   pack and return list of valid teleports
//   teleports formatted as  world:loc1:loc2
//   packed in pipe-delimited string
std::string GetPackedTeleportsList(std::string player) {
	auto teleports = InvokeGetTeleports(player);
	std::stringstream packed_teleports;
	foreach(teleport, teleports) {
		packed_teleports << teleport->ToString() << minecraft::kDelimiter3;
	}
	// pack the teleports into a pipe-delimited string to send to client
	auto packed_string = packed_teleports.str();
	return packed_string.substr(0, packed_string.length() - 1); // remove the last dangling delimiter
	return packed_string;
}

// if the message is in the right format, 
// this function invokes the WorldSwitch.exe with arguments from the message
std::string minecraft_service::handle_message(std::string message) {
	
	auto params = io_helpers::tokenize(message, minecraft::kDelimiter1); 

	if(params.size() == 0)
		return "";

	auto command = params[0];
	auto player = params[1];
	params.erase(params.begin());
	params.erase(params.begin());
	int numparams = params.size();
	
	if(command == commands::worldswitch && numparams == 1) {
		auto pair = params[0];
		InvokeWorldSwitch(player, WorldSwitch(pair));
		return ResponseCommand(commands::worldswitch_response, player, list(1, str("Transferred inventory between worlds")));
	}
	else if(command == commands::teleport && numparams == 1) {
		TeleportPair teleport(params[0]);
		bool success = InvokeTeleport(player, teleport);
		if(success)
			return ResponseCommand(commands::teleport_response, player, list(1, str("Teleported successfully")));
		else
			return ResponseCommand(commands::teleport_response, player, list(1, str("Teleport failed")));
	}
	else if(command == commands::get_teleports && numparams == 0) {
		auto teleports = GetPackedTeleportsList(player);
		return ResponseCommand(commands::get_teleports_response, player, list(1, teleports));
	}
	else if(command == commands::get_worldswitches && numparams == 0) {
		auto worldswitches = GetPackedWorldsToSwitch(player);
		return ResponseCommand(commands::get_worldswitches_response, player, list(1, worldswitches));
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





















