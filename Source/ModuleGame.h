#pragma once

#include "Globals.h"
#include "Module.h"
#include "p2Point.h"
#include "raylib.h"
#include <vector>
#include <string>

class PhysBody;
class PhysicEntity;

struct CollisionObject
{
	std::vector<vec2i> points;
	std::string type;
	std::string collision_type;
	std::string name;
	int offset_x = 0;
	int offset_y = 0;
};

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

	std::vector<CollisionObject> collision_objects;
	std::vector<PhysBody*> collision_bodies;

private:
	void LoadMap(const char* map_path);
	void LoadCollisions(const char* map_path);
	void CreateCollisionBodies();
};