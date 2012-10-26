#pragma once

#include "stdafx.h"
#include "gcsv.h"
#include <vector>


struct WorldData {
	
public:
	static std::vector<std::shared_ptr<WorldData>> LoadWorldsFromFile(std::string file) {
		return WorldData::LoadWorlds(gcsv::read(file)->operator[]("worlds"));
	}

	static std::vector<std::shared_ptr<WorldData>> LoadWorlds(GcsvTablePtr table) {
		std::vector<std::shared_ptr<WorldData>> worlds;
	
		BOOST_FOREACH(auto line, std::make_pair(table->begin(), table->end())){	
			std::shared_ptr<WorldData> world(new WorldData(line->get("name"), line->get("path")));
			worlds.push_back(world);
		}

		return worlds;
	}
	
	WorldData(std::string name, std::string path)
		: name_(name), path_(path) {
	}

	std::string name() const {
		return name_;
	}
	std::string path() const {
		return path_;
	}

private:
	std::string name_;
	std::string path_;
};