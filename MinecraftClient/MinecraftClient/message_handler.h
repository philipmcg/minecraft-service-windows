#pragma once

#include "stdafx.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>
#include <boost/function.hpp>
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
	UserActionInterface() : message_("void,void"), actions_() {
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
		AddAction("Quit", "quit", "");
		foreach(it, actions_) {
			(*it).OutputOption();
		}
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
};

class SayPrompt : public UserActionInterface {
public:
	UserAction HandleUserInput() {
		std::cout << "server says " << this->message()[0] << std::endl;

		AddAction("Teleport", "get_teleports", "");

		return PromptUser();
	}
};

class Prompt : public UserActionInterface {
public:
	UserAction HandleUserInput() {
		std::cout << user() << ", please enter a command." << std::endl;

		AddAction("Say", commands::say, "");

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

		if(response == commands::quit) {
			has_quit_ = true;
			return;
		}
		else
			send_to_server_callback_(response);
	}

	// This takes the response message from the server and chooses what to display for the user here.
	std::string MapCommandToAction(const MinecraftMessage& msg) {
		std::string cmd = msg.command();
		if(commands::acknowledge == cmd)
			return HandleUserAction<Prompt>(msg);
		else if(commands::say == cmd)
			return HandleUserAction<SayPrompt>(msg);
		
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