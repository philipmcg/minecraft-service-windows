#pragma once

#include <algorithm>
#include <cstdlib>
#include <vector>
#include <deque>


namespace commands {
    const std::string delimiter = ",";

// I can remove this macro use when I'm done defining new commands.
#define COMMAND( x, n ) \
	const std::string x = #x

	COMMAND(quit);
	COMMAND(login);
	COMMAND(acknowledge);
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

