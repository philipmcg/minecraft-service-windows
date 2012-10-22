#include "stdafx.h"
#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/function.hpp>
#include "../../MinecraftShared/minecraft_shared.hpp"
#include "message_handler.h"
#include "chat_client.h"

// for Sleep()
#include "windows.h"


#define DEBUG
int main(int argc, char* argv[])
{
#ifdef DEBUG
	argv = (char**)malloc(sizeof(char*) * 4);
	argv[0] = "";
	argv[1] = "66.66.235.201";
	argv[2] = "25500";
	//argv[3] = "worldswitch,world1,world2,PhilipM";
	argv[3] = "login,PhilipM";
	argc = 4;
#endif
	try
	{
		if (argc != 4)
		{
			std::cerr << "Usage: chat_client <host> <port> <command>\n";
			return 1;
		}

		char* host = argv[1];
		char* port = argv[2];

		boost::asio::io_service io_service;

		tcp::resolver resolver(io_service);

		tcp::resolver::query query(host, port);
		tcp::resolver::iterator iterator = resolver.resolve(query);

		chat_client c(io_service, iterator);
		MessageHandler handler(boost::bind(&chat_client::send_message, &c, _1));
		c.handler_for_messages_from_server(boost::bind(&MessageHandler::HandleMessage, &handler, _1));

		boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));

		char line[chat_message::max_body_length + 1];

		auto initial_message = argv[3];

		std::cout << "press enter continue" << std::endl;
		std::cin.getline(line, chat_message::max_body_length + 1); 
		c.send_message(initial_message);

		while(!handler.has_quit()) {
			Sleep(500);
		}

		std::cout << "quiting..." << std::endl;

		c.close();
		t.join();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}