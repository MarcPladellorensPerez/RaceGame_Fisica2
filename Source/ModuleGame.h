#pragma once

#include "Globals.h"
#include "Module.h"
#include "p2Point.h"
#include "raylib.h"
#include <vector>
#include <string>

#pragma warning(push)
#pragma warning(disable : 26495)
#include "box2d/box2d.h"
#pragma warning(pop)

#include "AIVehicle.h"

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

// Struct for map waypoints
struct Waypoint {
	int id;
	b2Vec2 position;
	std::vector<int> next_ids;

	Waypoint() : id(-1), position(0, 0) {}
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

	// IA & Game data
	std::vector<Waypoint> waypoints;
	std::vector<b2Vec2> spawn_points;

	// Vector IA Vehicles
	std::vector<AIVehicle*> ai_vehicles;

	std::vector<Texture2D> ai_car_textures;

	bool game_started;

private:
	void LoadMap(const char* map_path);
	void LoadCollisions(const char* map_path);
	void LoadMapObjects(const char* map_path);
	void CreateCollisionBodies();
	void CreateEnemiesAndPlayer();
};