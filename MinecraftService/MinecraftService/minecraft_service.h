#pragma once

#include "stdafx.h"
#include <cstdlib>
#include <algorithm>
#include <deque>
#include <iostream>
#include <list>
#include <set>
class minecraft_service {
public:
	std::string handle_message(std::string message);

private:
	void invoke_world_switch(std::string player, std::string world1, std::string world2);
	void invoke_teleport(std::string world, std::string player, std::string teleport1, std::string teleport2);
};