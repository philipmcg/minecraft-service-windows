#pragma once

#include "stdafx.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>
#include <boost/function.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "../../shared/minecraft_shared.hpp"


class UserAction {
public:
	UserAction(std::string shortcut, std::string text, MinecraftMessage command) : shortcut_(shortcut), text_(text), command_(command) {}
	std::string GetOptionText() {
		return shortcut_ + ". " + text_ + "\n";
	}
	void OutputOption() {
		std::cout << GetOptionText();
	}
	bool MatchesShortcut(std::string shortcut) {
		return shortcut_ == shortcut;
	}
	MinecraftMessage command() {
		return command_;
	}
private:
	std::string shortcut_;
	std::string text_;
	MinecraftMessage command_;
};


template <typename Type>
std::string HandleUserAction(const MinecraftMessage& message) {
	Type user_action;
	user_action.SetMessage(message);
	return user_action.HandleUserInput().command().AsMessage();
}


class UserActionInterface {
public:
	UserActionInterface() : message_("void,void"), actions_(), is_main_menu_(false) {
	}
	virtual UserAction HandleUserInput() = 0;
	void SetMessage(const MinecraftMessage& message) {
		message_ = MinecraftMessage(message);
	}

protected:
	std::string const& user() const { return message_.user(); }
	std::string const& command() const { return message_.command(); }
	std::string readline() {
		const int buffer_length = 512;
		char buffer[buffer_length];
		std::cin.getline(buffer, buffer_length); 
		return std::string(buffer);
	}
	void AddAction(std::string text, std::string command, std::string params) {
		std::stringstream stream;
		stream << (actions_.size() + 1);
		actions_.push_back(UserAction(stream.str(), text, MinecraftMessage(command, message_.user(), params)));
	}

	// Adds the quit option the list of actions, prints all actions, takes input, and returns the command whose shortcut matches the input.
	UserAction PromptUser() {
		if(!is_main_menu_)
			AddAction("Back to main menu", commands::menu, "");
		AddAction("Quit", "quit", "");
		foreach(it, actions_) {
			(*it).OutputOption();
		}
		std::cout << "--> ";
		std::string input = readline();
		foreach(it, actions_) {
			if(it->MatchesShortcut(input))
				return *it;
		}
		return actions_.back();
	}

	MinecraftMessage message() {
		return message_;
	}
protected:
	// message is the message that arrived FROM the server that spawned this interaction
	MinecraftMessage message_;
	std::vector<UserAction> actions_;
	bool is_main_menu_;
};

class SayPrompt : public UserActionInterface {
public:
	UserAction HandleUserInput() {
		std::cout << "server says " << this->message()[0] << std::endl;

		AddAction("Teleport", "get_teleports", "");
		return PromptUser();
	}
};

class MainPrompt : public UserActionInterface {
public:	UserAction HandleUserInput() {
		is_main_menu_ = true;

		if(this->message().num_params() > 0) {
			std::string server_message = this->message()[0];
			if(!server_message.empty())
				std::cout << std::endl << server_message << std::endl;
		}
		std::cout << std::endl << user() << ", please enter a command (i.e., type '1' and press enter)." << std::endl;

		AddAction("Teleport Menu", commands::get_teleports, "");
		AddAction("World Switch Menu", commands::get_worldswitches, "");
//		AddAction("Say", commands::say, "");

		return PromptUser();
	}
};

class TeleportsPrompt : public UserActionInterface {
public:	UserAction HandleUserInput() {
		auto teleports_string = this->message().operator[](0);
		auto teleports = util::tokenize(teleports_string, minecraft::kDelimiter3);

		foreach(teleport_string, teleports) {
			Teleport teleport(*teleport_string);
			std::stringstream text;
			text << teleport.World << ": " << teleport.Location1 << " to " << teleport.Location2;
			AddAction(text.str(), commands::teleport, teleport.ToString());
		}

		std::cout << std::endl << user() << ", choose the teleport you wish to use." << std::endl;

		return PromptUser();
	}
};
class WorldSwitchPrompt : public UserActionInterface {
public:	UserAction HandleUserInput() {
		auto worldswitches_string = this->message().operator[](0);
		auto worldswitches = util::tokenize(worldswitches_string, minecraft::kDelimiter3);

		foreach(worldswitch_string, worldswitches) {
			WorldSwitch worldswitch(*worldswitch_string);
			std::stringstream text;
			text << "Swap between " << worldswitch.World1 << " and " << worldswitch.World2;
			AddAction(text.str(), commands::worldswitch, worldswitch.ToString());
		}

		std::cout << std::endl << user() << ", choose the pair of worlds to swap inventory between." << std::endl;

		return PromptUser();
	}
};

class MessageHandler {
public:
	MessageHandler(boost::function<void(std::string)> send_to_server_callback) : has_quit_(false) {
		send_to_server_callback_ = send_to_server_callback;
	}
	void HandleMessage(std::string message) {
		const int buffer_length = 512;
		char buffer[buffer_length];
		
		MinecraftMessage msg(message);
		std::string response(commands::quit);
		
		response = MapCommandToAction(msg);

		if(boost::starts_with(response, commands::quit)) {
			has_quit_ = true;
			return;
		}
		else
			send_to_server_callback_(response);
	}

	// This takes the response message from the server and chooses what to display for the user here.
	std::string MapCommandToAction(const MinecraftMessage& msg) {
		std::string cmd = msg.command();
		if(commands::menu_response == cmd)
			return HandleUserAction<MainPrompt>(msg);
		else if(commands::say == cmd)
			return HandleUserAction<SayPrompt>(msg);
		else if(commands::get_teleports_response == cmd)
			return HandleUserAction<TeleportsPrompt>(msg);
		else if(commands::teleport_response == cmd)
			return HandleUserAction<MainPrompt>(msg);
		else if(commands::get_worldswitches_response == cmd)
			return HandleUserAction<WorldSwitchPrompt>(msg);
		else if(commands::worldswitch_response == cmd)
			return HandleUserAction<MainPrompt>(msg);
		
		return commands::quit;
	}

	// called by the main program thread, to find out when we've quit. 
	// (this handler runs in an async thread)
	bool has_quit() {
		quit_lock_.lock();
		bool quit = has_quit_;
		quit_lock_.unlock();
		return quit;
	}

private:
	boost::function<void(std::string)> send_to_server_callback_;
	bool has_quit_;
	boost::mutex quit_lock_;
};