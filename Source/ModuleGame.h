#pragma once

#include "Globals.h"
#include "Module.h"
#include "Leaderboard.h"
#include "SelectCharacters.h"
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

enum class MenuState {
	INTRO_ANIMATION,
	START_MENU,
	LEVEL_SELECT,
	CHARACTER_SELECT,
	PLAYING
};

class ModuleGame : public Module
{
public:
	ModuleGame(Application* app, bool start_enabled = true);
	~ModuleGame();

	bool Start();
	update_status Update();
	bool CleanUp();

	// Start Menu
	MenuState menu_state;
	bool show_menu;
	Texture2D start_menu_texture;
	Texture2D level_select_texture;

	// Background music
	Music menu_music;
	Music level1_music;
	Music level2_music;
	Music level3_music;
	Music current_music;

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

	//Leaderboard
	Leaderboard* leaderboard;

	//System Select Characters
	CharacterSelect* character_select;
	Texture2D selected_player_car;

	//Intro
	Texture2D intro_spritesheet;
	int intro_frame_actual;
	int intro_total_frames;
	float intro_timer;
	float intro_frame_duration;
	int intro_frames_por_fila;
	int intro_frame_width;
	int intro_frame_height;

	//Traffic Lights
	Texture2D traffic_light_spritesheet;
	int traffic_light_current_frame;
	int traffic_light_total_frames;
	float traffic_light_timer;
	float traffic_light_duration_per_light;
	bool traffic_light_active;
	bool race_can_start;
	int traffic_light_frame_width;
	int traffic_light_frame_height;

	int player_current_waypoint;
	float player_distance_to_waypoint;

	float current_map_spawn_rotation;


private:
	void LoadMap(const char* map_path);
	void LoadCollisions(const char* map_path);
	void LoadMapObjects(const char* map_path);
	void CreateCollisionBodies();
	void CreateEnemiesAndPlayer();
	void StartGame(const char* map_path);
	void ResetGame();
	void UpdatePlayerWaypoint();
	void PlayBackgroundMusic(Music music);
	void StopCurrentMusic();

};