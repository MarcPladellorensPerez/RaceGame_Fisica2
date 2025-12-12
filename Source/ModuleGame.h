#pragma once

#include "Globals.h"
#include "Module.h"
#include "p2Point.h"
#include "raylib.h"
#include <vector>
#include <string>

class PhysBody;
class PhysicEntity;

class ModuleGame : public Module
{
public:
	ModuleGame(Application* app, bool start_enabled = true);
	~ModuleGame();

	bool Start();
	update_status Update();
	bool CleanUp();

public:
	Texture2D tile_set;
	std::vector<int> map_data;
	int map_width;
	int map_height;

private:
	void LoadMap(const char* map_path);
};