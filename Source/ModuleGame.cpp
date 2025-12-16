#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <algorithm>

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

	LOG("ModuleGame: Carregant recursos del mapa...");

	tile_set = LoadTexture("Assets/Map/spritesheet_tiles.png");

	if (tile_set.id == 0)
	{
		LOG("ERROR: No s'ha pogut carregar el tileset!");
		return false;
	}

	LoadMap("Assets/Map/RaceTrack.tmx");
	LoadCollisions("Assets/Map/RaceTrack.tmx");
	CreateCollisionBodies();

	LOG("ModuleGame: Mapa carregat correctament amb %d objectes de col·lisio", (int)collision_objects.size());

	return ret;
}

bool ModuleGame::CleanUp()
{
	UnloadTexture(tile_set);
	map_data.clear();
	collision_objects.clear();
	collision_bodies.clear();
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
	std::string current_property_name;

	while (std::getline(file, line))
	{
		// Detect collision layer
		if (line.find("name=\"Collisions\"") != std::string::npos)
		{
			in_collisions_layer = true;
			LOG("ModuleGame: Trobada capa de col·lisions");
			continue;
		}

		// Exit collision layer
		if (in_collisions_layer && line.find("</objectgroup>") != std::string::npos)
		{
			in_collisions_layer = false;
			LOG("ModuleGame: Fi de la capa de col·lisions");
			break;
		}

		if (!in_collisions_layer) continue;

		// Detect object start
		if (line.find("<object") != std::string::npos)
		{
			in_object = true;
			current_object = CollisionObject();

			// Name if available
			size_t name_pos = line.find("name=\"");
			if (name_pos != std::string::npos)
			{
				size_t name_start = name_pos + 6;
				size_t name_end = line.find("\"", name_start);
				current_object.name = line.substr(name_start, name_end - name_start);
			}

			// Object X position
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

			// Object Y position
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

		// Process properties
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

		// Detect polygon
		if (in_object && line.find("<polygon") != std::string::npos)
		{
			in_polygon = true;

			// Extract polygon points
			size_t points_pos = line.find("points=\"");
			if (points_pos != std::string::npos)
			{
				size_t points_start = points_pos + 8;
				size_t points_end = line.find("\"", points_start);
				std::string points_str = line.substr(points_start, points_end - points_start);

				// Parse points
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

		// End of the object
		if (in_object && line.find("</object>") != std::string::npos)
		{
			in_object = false;
			in_polygon = false;

			// Only add if valid and of correct type
			if (!current_object.points.empty() && current_object.type == "wall_chain")
			{
				collision_objects.push_back(current_object);
				LOG("ModuleGame: Col·lisio carregada '%s' amb %d punts a offset (%d, %d)",
					current_object.name.c_str(), (int)current_object.points.size(),
					current_object.offset_x, current_object.offset_y);
			}
		}
	}

	file.close();
	LOG("ModuleGame: Total de col·lisions carregades: %d", (int)collision_objects.size());
}

void ModuleGame::CreateCollisionBodies()
{
	const float MIN_DISTANCE_SQ = 1.0f;

	for (const auto& collision : collision_objects)
	{
		if (collision.points.size() < 2)
		{
			LOG("WARNING: Objecte '%s' te menys de 2 punts, s'ignora", collision.name.c_str());
			continue;
		}

		// Apply offset
		std::vector<vec2i> absolute_points;
		absolute_points.reserve(collision.points.size());

		for (size_t i = 0; i < collision.points.size(); ++i)
		{
			vec2i absolute_point;
			absolute_point.x = collision.points[i].x + collision.offset_x;
			absolute_point.y = collision.points[i].y + collision.offset_y;
			absolute_points.push_back(absolute_point);
		}

		// We invert order in "Exterior"
		if (collision.name == "Exterior")
		{
			std::reverse(absolute_points.begin(), absolute_points.end());
			LOG("ModuleGame: Ordre de punts invertit per '%s'", collision.name.c_str());
		}

		// Delete points that are too close
		std::vector<vec2i> filtered_points;
		filtered_points.push_back(absolute_points[0]);

		for (size_t i = 1; i < absolute_points.size(); ++i)
		{
			const vec2i& current = absolute_points[i];
			const vec2i& last_added = filtered_points.back();

			// Cakculate squared distance
			int dx = current.x - last_added.x;
			int dy = current.y - last_added.y;
			float dist_sq = (float)(dx * dx + dy * dy);

			// Only add if far enough
			if (dist_sq >= MIN_DISTANCE_SQ)
			{
				filtered_points.push_back(current);
			}
		}

		// Verify distance between first and last point
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

		// Validate final number of points
		if (filtered_points.size() < 3)
		{
			LOG("WARNING: Objecte '%s' te menys de 3 punts valids despres del filtre (%d), s'ignora",
				collision.name.c_str(), (int)filtered_points.size());
			continue;
		}

		// Convert to array of ints
		std::vector<int> points_array;
		points_array.reserve(filtered_points.size() * 2);

		for (size_t i = 0; i < filtered_points.size(); ++i)
		{
			points_array.push_back(filtered_points[i].x);
			points_array.push_back(filtered_points[i].y);
		}

		// Create chain physics body
		PhysBody* body = App->physics->CreateChain(
			0, 0,
			points_array.data(),
			(int)points_array.size(),
			PhysBodyType::STATIC
		);

		if (body != nullptr)
		{
			collision_bodies.push_back(body);
			LOG("ModuleGame: Chain creada per '%s' amb %d punts filtrats (original: %d) a posicio (%d,%d)",
				collision.name.c_str(), (int)filtered_points.size(), (int)collision.points.size(),
				collision.offset_x, collision.offset_y);
		}
		else
		{
			LOG("ERROR: No s'ha pogut crear chain per '%s'", collision.name.c_str());
		}
	}

	LOG("ModuleGame: %d collision bodies creats correctament", (int)collision_bodies.size());
}