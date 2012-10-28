#pragma once

#include <algorithm>
#include <cstdlib>
#include <vector>
#include <deque>

namespace minecraft  {
	const char kDelimiter1 = ',';
	const char kDelimiter2 = ':';
	const char kDelimiter3 = '|';
}

namespace util  {
	
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

namespace commands {

// I can remove this macro use when I'm done defining new commands.
// Each entry defines a command that can be passed between the client and server.
// Generally, "response" commands are sent from server to client.
#define COMMAND( x, n ) \
	const std::string x = #x

	COMMAND(quit);
	COMMAND(login);
	COMMAND(menu); // back to main menu
	COMMAND(menu_response);
	COMMAND(say);
	COMMAND(teleport);
	COMMAND(teleport_response);

	COMMAND(worldswitch);
	COMMAND(worldswitch_response);
	COMMAND(get_teleports);
	COMMAND(get_teleports_response);
	COMMAND(get_worldswitches);
	COMMAND(get_worldswitches_response);

	COMMAND(get_coords);
#undef COMMAND

}


// Converts: 
// "a,b,c"   --> ["a", "b", "c"]
// "a,b,c,"  --> ["a", "b", "c"]
// "a,,b,c," --> ["a", "", "b", "c"]
// ","       --> []
// ""        --> []
std::deque<std::string> tokenize(std::string text, char delimiter) {
	
	std::deque<std::string> split;

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

 
// Message format that can be sent between client and server.
// Messages consist of a command, username, and variable number of parameters.
// All of these fields are stored as a comma-delimited string when passed to 
// the client and server classes in the code.
class MinecraftMessage {

	static const char delimiter = minecraft::kDelimiter1;

public:
	MinecraftMessage(std::string command, const std::string& user, std::string params) : command_(command), user_(user) {
		auto tokens = tokenize(params, delimiter);		
		params_ = std::deque<std::string>(tokens);
	}
	MinecraftMessage(std::deque<std::string> tokens) {
		command_ = tokens.front();
		tokens.pop_front();
		user_ = tokens.front();
		tokens.pop_front();

		params_ = std::deque<std::string>(tokens);
	}

	MinecraftMessage(std::string message) {
		auto tokens = tokenize(message, delimiter);		
		command_ = tokens.front();
		tokens.pop_front();
		user_ = tokens.front();
		tokens.pop_front();

		params_ = std::deque<std::string>(tokens);
	}

	std::string const& command() const { return command_; }
	std::string const& user() const { return user_; }

	int num_params() {
		return params_.size();
	}

	// returns the nth parameter, indexed from 0.  
	// So if the message is "teleport,PhilipM,200,300"
	// then msg[0] == 200 and msg[1] == 300
	std::string operator[](int place) {
		return params_.at(place); 
	}

	// formats the message as a comma delimited string as it will be sent over the network
	std::string AsMessage() {
		std::stringstream stream;
		stream << command_ << delimiter << user_;
		foreach(str, params_) {
			stream << delimiter << *str;
		}
		return stream.str();
	}

private:
	std::deque<std::string> params_;
	std::string command_;
	std::string user_;
};

// represents a point in three-dimensional space of the minecraft world
struct Coordinates {
	static const char delimiter = minecraft::kDelimiter2;
	double x;
	double y;
	double z;

	Coordinates() {
		x = 0;
		y = 0;
		z = 0;
	}
	Coordinates(std::string x_, std::string y_, std::string z_) {
		x = atof(x_.c_str());
		y = atof(y_.c_str());
		z = atof(z_.c_str());
	}
	Coordinates(std::string str) {
		auto list = util::tokenize(str, delimiter);
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

struct Teleport {
	std::string World;
	std::string Location;
	Coordinates Coords;
	Teleport() {}
	Teleport(std::string world, std::string location, Coordinates coords) 
		: World(world), Location(location), Coords(coords) {
	}

	bool Equals(const Teleport& other) {
		return other.World == World && other.Location == Location;
	}
};

// Represents a pair of teleport locations in a single world
// that a player could teleport between.
struct TeleportPair {
	static const char delimiter = minecraft::kDelimiter2;
	std::string World;
	Teleport Teleport1;
	Teleport Teleport2;

	TeleportPair(std::string world, std::string location1, std::string location2, Coordinates coords1, Coordinates coords2) 
		: World(world), Teleport1(world, location1, coords1), Teleport2(world, location2, coords2) {
	}

	TeleportPair(std::string packed) {
		auto unpacked = util::tokenize(packed, delimiter);
		World = unpacked[0];
		auto location1 = unpacked[1];
		auto location2 = unpacked[2];
		Teleport1 = Teleport(World, location1, Coordinates());
		Teleport2 = Teleport(World, location2, Coordinates());
	}

	std::string ToString() {
		std::stringstream stream;
		stream << World << delimiter << Teleport1.Location << delimiter << Teleport2.Location;
		return stream.str();
	}

	bool Equals(const TeleportPair& other) {
		return other.World == World && Teleport1.Equals(other.Teleport1) && Teleport2.Equals(other.Teleport2);
	}
};


// Represents a pair of worlds (stored as world names)
// that a player could switch between.
struct WorldSwitch {
	static const char delimiter = minecraft::kDelimiter2;
	std::string World1;
	std::string World2;

	WorldSwitch(std::string world1, std::string world2) 
		: World1(world1), World2(world2) { 
	}

	WorldSwitch(std::string packed) {
		auto unpacked = util::tokenize(packed, delimiter);
		World1 = unpacked[0];
		World2 = unpacked[1];
	}

	std::string ToString() {
		std::stringstream stream;
		stream << World1 << delimiter << World2;
		return stream.str();
	}
};