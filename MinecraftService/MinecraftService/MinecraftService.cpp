
#include "stdafx.h"
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include "../../shared/chat_message.hpp"
#include "variable_bin.h"

#include "chat_server.h"

#include "minecraft_service.h"
#include "gcsv.h"

//----------------------------------------------------------------------


void test_variable_bin();

void run_tests() {
	std::cout << "running tests..." << std::endl;
	gcsv::test_gcsv();
	test_variable_bin();
	std::cout << "finished tests..." << std::endl;
}




#define DEBUG_
int main(int argc, char* argv[])
{
	// uncomment this to run unit tests
#ifdef DEBUG
	 run_tests();
#endif

	if(argc < 2) {
		// These arguments will create one server at the given port
		char* argv2[2];
		argv = argv2;
		argv[0] = "";
		argv[1] = "25500";
		argc = 2;
	}

	boost::shared_ptr<minecraft_service> my_minecraft_service = boost::shared_ptr<minecraft_service>(new minecraft_service());

	try
	{
		if (argc < 2)
		{
			std::cerr << "Usage: chat_server <port> [<port> ...]\n";
			return 1;
		}

		boost::asio::io_service io_service;

		chat_server_list servers;
		for (int i = 1; i < argc; ++i)
		{
			int port = std::atoi(argv[i]);
			std::cout << "listening on port " << port << std::endl;
			tcp::endpoint endpoint(tcp::v4(), port);
			chat_server_ptr server(new chat_server(io_service, endpoint, boost::bind(&minecraft_service::handle_message, my_minecraft_service, _1)));
			servers.push_back(server);
		}

		io_service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}