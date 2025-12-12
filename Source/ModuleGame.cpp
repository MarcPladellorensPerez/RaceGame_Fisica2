#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"
#include <fstream>
#include <sstream>
#include <iostream>

ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	map_width = 0;
	map_height = 0;
	tile_set = { 0 };
}

ModuleGame::~ModuleGame()
{
}

bool ModuleGame::Start()
{
	bool ret = true;

	tile_set = LoadTexture("Assets/Map/spritesheet_tiles.png");

	LoadMap("Assets/Map/RaceTrack.tmx");

	if (map_width > 0 && map_height > 0) {
		int ground_points[] = {
			0, 0,
			0, map_height * 128,
			map_width * 128, map_height * 128,
			map_width * 128, 0
		};
		App->physics->CreateChain(0, 0, ground_points, 8, PhysBodyType::STATIC);
	}

	return ret;
}

bool ModuleGame::CleanUp()
{
	UnloadTexture(tile_set);
	map_data.clear();
	return true;
}

update_status ModuleGame::Update()
{
	int tile_size = 128;
	int columns = 18;

	for (int y = 0; y < map_height; ++y)
	{
		for (int x = 0; x < map_width; ++x)
		{
			int id = map_data[y * map_width + x];
			if (id > 0)
			{
				int gid = id - 1;
				int tx = gid % columns;
				int ty = gid / columns;

				Rectangle source = { (float)tx * tile_size, (float)ty * tile_size, (float)tile_size, (float)tile_size };
				App->renderer->Draw(tile_set, x * tile_size, y * tile_size, &source);
			}
		}
	}

	return UPDATE_CONTINUE;
}

void ModuleGame::LoadMap(const char* map_path)
{
	std::ifstream file(map_path);
	if (file.is_open())
	{
		std::string line;
		bool data_found = false;

		map_width = 60;
		map_height = 50;

		while (std::getline(file, line))
		{
			if (line.find("<data encoding=\"csv\">") != std::string::npos)
			{
				data_found = true;
				continue;
			}
			if (line.find("</data>") != std::string::npos)
			{
				break;
			}

			if (data_found)
			{
				std::stringstream ss(line);
				std::string value;
				while (std::getline(ss, value, ','))
				{
					if (!value.empty() && value != "\n" && value != "\r")
					{
						try {
							map_data.push_back(std::stoi(value));
						}
						catch (...) {}
					}
				}
			}
		}
		file.close();
	}
}