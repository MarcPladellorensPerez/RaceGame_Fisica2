#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"
#include "Player.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <algorithm>
#include <random>

ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	map_width = 0;
	map_height = 0;
	tile_set = { 0 };
	game_started = false;
}

ModuleGame::~ModuleGame()
{
}

bool ModuleGame::Start()
{
	bool ret = true;

	LOG("ModuleGame: Carregant recursos del mapa...");

	tile_set = LoadTexture("Assets/Map/spritesheet_tiles.png");

	if (tile_set.id == 0)
	{
		LOG("ERROR: No s'ha pogut carregar el tileset!");
		return false;
	}

	char path[256];
	for (int i = 2; i <= 8; ++i) {
		sprintf_s(path, "Assets/Textures/Cars/car%d.png", i);
		Texture2D tex = LoadTexture(path);
		if (tex.id != 0) {
			ai_car_textures.push_back(tex);
		}
	}

	LoadMap("Assets/Map/RaceTrack.tmx");
	LoadCollisions("Assets/Map/RaceTrack.tmx");
	LoadMapObjects("Assets/Map/RaceTrack.tmx");

	CreateCollisionBodies();

	LOG("ModuleGame: Mapa carregat correctament.");

	return ret;
}

bool ModuleGame::CleanUp()
{
	UnloadTexture(tile_set);

	for (auto& tex : ai_car_textures) {
		UnloadTexture(tex);
	}
	ai_car_textures.clear();

	for (auto* vehicle : ai_vehicles) {
		delete vehicle;
	}
	ai_vehicles.clear();

	waypoints.clear();
	spawn_points.clear();
	map_data.clear();
	collision_objects.clear();
	collision_bodies.clear();
	return true;
}

update_status ModuleGame::Update()
{
	if (!game_started) {
		CreateEnemiesAndPlayer();
		game_started = true;
	}

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

	float dt = GetFrameTime();
	for (auto* vehicle : ai_vehicles) {
		vehicle->Update(dt, waypoints);
		vehicle->Draw(App->physics->debug);
	}

	return UPDATE_CONTINUE;
}

std::vector<std::string> SplitString(const std::string& s, char delimiter) {
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter)) {
		token.erase(remove(token.begin(), token.end(), ' '), token.end());
		if (!token.empty()) tokens.push_back(token);
	}
	return tokens;
}

void ModuleGame::LoadMapObjects(const char* map_path)
{
	std::ifstream file(map_path);
	if (!file.is_open()) return;

	std::string line;
	bool in_spawns = false;
	bool in_waypoints = false;
	bool in_object = false;

	b2Vec2 tempPos(0, 0);
	int tempId = -1;
	std::vector<int> tempNextIds;

	while (std::getline(file, line))
	{
		if (line.find("name=\"SpawnPoints\"") != std::string::npos) in_spawns = true;
		if (line.find("name=\"AI_Waypoints\"") != std::string::npos) in_waypoints = true;

		if (line.find("</objectgroup>") != std::string::npos) {
			in_spawns = false;
			in_waypoints = false;
		}

		if (!in_spawns && !in_waypoints) continue;

		if (line.find("<object") != std::string::npos) {
			in_object = true;
			tempPos.SetZero();
			tempId = -1;
			tempNextIds.clear();

			size_t x_pos = line.find("x=\"");
			if (x_pos != std::string::npos) {
				size_t start = x_pos + 3;
				tempPos.x = std::stof(line.substr(start, line.find("\"", start) - start));
			}
			size_t y_pos = line.find("y=\"");
			if (y_pos != std::string::npos) {
				size_t start = y_pos + 3;
				tempPos.y = std::stof(line.substr(start, line.find("\"", start) - start));
			}
		}

		if (in_object && line.find("<property") != std::string::npos) {
			if (line.find("name=\"checkpoint_id\"") != std::string::npos || line.find("name=\"spawn_id\"") != std::string::npos || line.find("name=\"id\"") != std::string::npos) {
				size_t val_pos = line.find("value=\"");
				if (val_pos != std::string::npos) {
					size_t start = val_pos + 7;
					try { tempId = std::stoi(line.substr(start, line.find("\"", start) - start)); }
					catch (...) {}
				}
			}
			if (line.find("name=\"next_ids\"") != std::string::npos) {
				size_t val_pos = line.find("value=\"");
				if (val_pos != std::string::npos) {
					size_t start = val_pos + 7;
					std::string raw = line.substr(start, line.find("\"", start) - start);
					auto tokens = SplitString(raw, ',');
					for (const auto& t : tokens) {
						try { tempNextIds.push_back(std::stoi(t)); }
						catch (...) {}
					}
				}
			}
		}

		if (in_object && line.find("</object>") != std::string::npos) {
			in_object = false;

			if (in_spawns) {
				spawn_points.push_back(tempPos);
			}
			if (in_waypoints && tempId != -1) {
				Waypoint wp;
				wp.id = tempId;
				wp.position = b2Vec2(PIXELS_TO_METERS(tempPos.x), PIXELS_TO_METERS(tempPos.y));
				wp.next_ids = tempNextIds;
				waypoints.push_back(wp);
			}
		}
	}
	file.close();
}

void ModuleGame::CreateEnemiesAndPlayer()
{
	if (spawn_points.empty()) {
		LOG("ERROR: No spawn points loaded!");
		return;
	}

	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(spawn_points.begin(), spawn_points.end(), g);

	if (App->player != nullptr && App->player->vehicle != nullptr) {
		App->player->SetPosition(spawn_points[0].x, spawn_points[0].y, -90.0f);
		LOG("Player teleported to spawn: %.2f, %.2f", spawn_points[0].x, spawn_points[0].y);
	}
	else {
		LOG("ERROR: Player vehicle not ready during spawn assignment!");
	}

	for (size_t i = 1; i < spawn_points.size(); ++i) {
		AIVehicle* newAI = new AIVehicle();

		Texture2D tex;
		if (!ai_car_textures.empty()) {
			tex = ai_car_textures[rand() % ai_car_textures.size()];
		}
		else {
			tex = App->player->vehicle_texture;
		}

		int startWP = rand() % 4;
		b2Vec2 spawnPosMeters(PIXELS_TO_METERS(spawn_points[i].x), PIXELS_TO_METERS(spawn_points[i].y));

		newAI->Init(App->physics->GetWorld(), spawnPosMeters, tex, startWP);
		ai_vehicles.push_back(newAI);

		LOG("AIVehicle created at spawn %d", (int)i);
	}
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

void ModuleGame::LoadCollisions(const char* map_path)
{
	std::ifstream file(map_path);
	if (!file.is_open())
	{
		LOG("ERROR: No s'ha pogut obrir el fitxer TMX per carregar col·lisions");
		return;
	}

	std::string line;
	bool in_collisions_layer = false;
	bool in_object = false;
	bool in_polygon = false;

	CollisionObject current_object;

	while (std::getline(file, line))
	{
		if (line.find("name=\"Collisions\"") != std::string::npos)
		{
			in_collisions_layer = true;
			continue;
		}

		if (in_collisions_layer && line.find("</objectgroup>") != std::string::npos)
		{
			in_collisions_layer = false;
			break;
		}

		if (!in_collisions_layer) continue;

		if (line.find("<object") != std::string::npos)
		{
			in_object = true;
			current_object = CollisionObject();

			size_t name_pos = line.find("name=\"");
			if (name_pos != std::string::npos)
			{
				size_t name_start = name_pos + 6;
				size_t name_end = line.find("\"", name_start);
				current_object.name = line.substr(name_start, name_end - name_start);
			}

			size_t x_pos = line.find("x=\"");
			if (x_pos != std::string::npos)
			{
				size_t x_start = x_pos + 3;
				size_t x_end = line.find("\"", x_start);
				try {
					current_object.offset_x = (int)std::stof(line.substr(x_start, x_end - x_start));
				}
				catch (...) {}
			}

			size_t y_pos = line.find("y=\"");
			if (y_pos != std::string::npos)
			{
				size_t y_start = y_pos + 3;
				size_t y_end = line.find("\"", y_start);
				try {
					current_object.offset_y = (int)std::stof(line.substr(y_start, y_end - y_start));
				}
				catch (...) {}
			}
		}

		if (in_object && line.find("<property") != std::string::npos)
		{
			size_t name_pos = line.find("name=\"");
			size_t value_pos = line.find("value=\"");

			if (name_pos != std::string::npos && value_pos != std::string::npos)
			{
				size_t name_start = name_pos + 6;
				size_t name_end = line.find("\"", name_start);
				std::string prop_name = line.substr(name_start, name_end - name_start);

				size_t value_start = value_pos + 7;
				size_t value_end = line.find("\"", value_start);
				std::string prop_value = line.substr(value_start, value_end - value_start);

				if (prop_name == "collision_type")
				{
					current_object.collision_type = prop_value;
				}
				else if (prop_name == "type")
				{
					current_object.type = prop_value;
				}
			}
		}

		if (in_object && line.find("<polygon") != std::string::npos)
		{
			in_polygon = true;

			size_t points_pos = line.find("points=\"");
			if (points_pos != std::string::npos)
			{
				size_t points_start = points_pos + 8;
				size_t points_end = line.find("\"", points_start);
				std::string points_str = line.substr(points_start, points_end - points_start);

				std::stringstream ss(points_str);
				std::string pair;

				while (std::getline(ss, pair, ' '))
				{
					size_t comma_pos = pair.find(',');
					if (comma_pos != std::string::npos)
					{
						try
						{
							float x = std::stof(pair.substr(0, comma_pos));
							float y = std::stof(pair.substr(comma_pos + 1));
							current_object.points.push_back(vec2i((int)x, (int)y));
						}
						catch (...)
						{
							LOG("ERROR: No s'ha pogut parsejar punt: %s", pair.c_str());
						}
					}
				}
			}
		}

		if (in_object && line.find("</object>") != std::string::npos)
		{
			in_object = false;
			in_polygon = false;

			if (!current_object.points.empty() && current_object.type == "wall_chain")
			{
				collision_objects.push_back(current_object);
			}
		}
	}

	file.close();
}

void ModuleGame::CreateCollisionBodies()
{
	const float MIN_DISTANCE_SQ = 1.0f;

	for (const auto& collision : collision_objects)
	{
		if (collision.points.size() < 2) continue;

		std::vector<vec2i> absolute_points;
		absolute_points.reserve(collision.points.size());

		for (size_t i = 0; i < collision.points.size(); ++i)
		{
			vec2i absolute_point;
			absolute_point.x = collision.points[i].x + collision.offset_x;
			absolute_point.y = collision.points[i].y + collision.offset_y;
			absolute_points.push_back(absolute_point);
		}

		if (collision.name == "Exterior")
		{
			std::reverse(absolute_points.begin(), absolute_points.end());
		}

		std::vector<vec2i> filtered_points;
		filtered_points.push_back(absolute_points[0]);

		for (size_t i = 1; i < absolute_points.size(); ++i)
		{
			const vec2i& current = absolute_points[i];
			const vec2i& last_added = filtered_points.back();

			int dx = current.x - last_added.x;
			int dy = current.y - last_added.y;
			float dist_sq = (float)(dx * dx + dy * dy);

			if (dist_sq >= MIN_DISTANCE_SQ)
			{
				filtered_points.push_back(current);
			}
		}

		if (filtered_points.size() > 1)
		{
			const vec2i& first = filtered_points.front();
			const vec2i& last = filtered_points.back();

			int dx = first.x - last.x;
			int dy = first.y - last.y;
			float dist_sq = (float)(dx * dx + dy * dy);

			if (dist_sq < MIN_DISTANCE_SQ)
			{
				filtered_points.pop_back();
			}
		}

		if (filtered_points.size() < 3) continue;

		std::vector<int> points_array;
		points_array.reserve(filtered_points.size() * 2);

		for (size_t i = 0; i < filtered_points.size(); ++i)
		{
			points_array.push_back(filtered_points[i].x);
			points_array.push_back(filtered_points[i].y);
		}

		PhysBody* body = App->physics->CreateChain(
			0, 0,
			points_array.data(),
			(int)points_array.size(),
			PhysBodyType::STATIC
		);

		if (body != nullptr)
		{
			collision_bodies.push_back(body);
		}
	}
}