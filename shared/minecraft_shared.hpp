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
    const std::string delimiter = ",";

// I can remove this macro use when I'm done defining new commands.
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

	COMMAND(get_coords);
#undef COMMAND

    std::string make(std::string command, std::string param1) {
        std::stringstream stream;
        stream << command << delimiter << param1;
        return stream.str();
    }
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

 
class MinecraftMessage {

public:
	MinecraftMessage(std::string command, const std::string& user, std::string params) : command_(command), user_(user) {
		auto tokens = tokenize(params, ',');		
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
		auto tokens = tokenize(message, ',');		
		command_ = tokens.front();
		tokens.pop_front();
		user_ = tokens.front();
		tokens.pop_front();

		params_ = std::deque<std::string>(tokens);
	}

	std::string const& command() const { return command_; }
	std::string const& user() const { return user_; }

	// returns the nth parameter, indexed from 0.  
	// So if the message is "teleport,PhilipM,200,300"
	// then msg[0] == 200 and msg[1] == 300
	std::string operator[](int place) {
		return params_.at(place); 
	}

	// formats the message as a comma delimited string as it will be sent over the network
	std::string AsMessage() {
		std::stringstream stream;
		stream << command_ << commands::delimiter << user_;
		foreach(str, params_) {
			stream << commands::delimiter << *str;
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
	static const char delimiter = minecraft::kDelimiter3;
	std::string World;
	std::string Location1;
	std::string Location2;
	Coordinates Coords1;
	Coordinates Coords2;

	Teleport(std::string world, std::string location1, std::string location2, Coordinates coords1, Coordinates coords2) 
		: World(world), Location1(location1), Location2(location2), Coords1(coords1), Coords2(coords2) { 
	}

	Teleport(std::string packed) : Coords1(), Coords2() {
		auto unpacked = util::tokenize(packed, delimiter);
		World = unpacked[0];
		Location1 = unpacked[1];
		Location2 = unpacked[2];
	}

	std::string ToString() {
		std::stringstream stream;
		stream << World << delimiter << Location1 << delimiter << Location2;
		return stream.str();
	}

	bool Equals(const Teleport& other) {
		return other.World == World && other.Location1 == Location1 && other.Location2 == Location2;
	}
};