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

	// Start menu variables
	show_menu = true;
	menu_state = MenuState::INTRO_ANIMATION;
	start_menu_texture = { 0 };
	level_select_texture = { 0 };
	selected_player_car = { 0 };

	// Initialize music structures
	menu_music = { 0 };
	level1_music = { 0 };
	level2_music = { 0 };
	level3_music = { 0 };
	current_music = { 0 };

	//Intro
	intro_spritesheet = { 0 };
	intro_frame_actual = 0;
	intro_total_frames = 25;
	intro_timer = 0.0f;
	intro_frame_duration = 0.10f;
	intro_frames_por_fila = 25;   
	intro_frame_width = 1280;     
	intro_frame_height = 720;

	leaderboard = new Leaderboard();
	character_select = new CharacterSelect();

}

ModuleGame::~ModuleGame()
{
	if (leaderboard) {
		delete leaderboard;
		leaderboard = nullptr;
	}

	if (character_select) {  
		delete character_select;
		character_select = nullptr;
	}
}

bool ModuleGame::Start()
{
	bool ret = true;

	LOG("ModuleGame: Carregant recursos del mapa...");

	// Load intro texture
	intro_spritesheet = LoadTexture("Assets/Textures/UI/IntroAnimation.png");

	// Load start menu texture
	start_menu_texture = LoadTexture("Assets/Textures/UI/StartMenu.png");

	if (start_menu_texture.id == 0)
	{
		LOG("WARNING: Could not load start menu texture, using text menu");
	}
	else
	{
		LOG("Start menu texture loaded successfully!");
	}

	// Load level select menu texture
	level_select_texture = LoadTexture("Assets/Textures/UI/SelectLevelMenu.png");

	if (level_select_texture.id == 0)
	{
		LOG("WARNING: Could not load level select texture");
	}
	else
	{
		LOG("Level select texture loaded successfully!");
	}

	tile_set = LoadTexture("Assets/Map/spritesheet_tiles.png");

	if (tile_set.id == 0)
	{
		LOG("ERROR: No s'ha pogut carregar el tileset!");
		return false;
	}

	if (leaderboard) {
		leaderboard->Init();
	}

	if (character_select) {
		character_select->Init();
	}

	// Load AI car textures
	char path[256];
	for (int i = 2; i <= 8; ++i) {
		sprintf_s(path, "Assets/Textures/Cars/car%d.png", i);
		Texture2D tex = LoadTexture(path);
		if (tex.id != 0) {
			ai_car_textures.push_back(tex);
		}
	}

	// Load background music
	menu_music = LoadMusicStream("Assets/Audio/BackgroundMusic/Bandolero.mp3");
	level1_music = LoadMusicStream("Assets/Audio/BackgroundMusic/TokyoDrift.mp3");
	level2_music = LoadMusicStream("Assets/Audio/BackgroundMusic/DanzaKuduro.mp3");
	level3_music = LoadMusicStream("Assets/Audio/BackgroundMusic/Delirious.mp3");

	if (!IsMusicReady(menu_music)) {
		LOG("WARNING: Could not load menu music");
	}
	if (!IsMusicReady(level1_music)) {
		LOG("WARNING: Could not load level 1 music");
	}
	if (!IsMusicReady(level2_music)) {
		LOG("WARNING: Could not load level 2 music");
	}
	if (!IsMusicReady(level3_music)) {
		LOG("WARNING: Could not load level 3 music");
	}

	// Start menu music
	PlayBackgroundMusic(menu_music);

	// Don't load map on start, wait for level selection
	LOG("ModuleGame: Resources loaded, waiting for level selection.");

	return ret;
}

bool ModuleGame::CleanUp()
{
	// Stop and unload music
	StopCurrentMusic();
	if (IsMusicReady(menu_music)) UnloadMusicStream(menu_music);
	if (IsMusicReady(level1_music)) UnloadMusicStream(level1_music);
	if (IsMusicReady(level2_music)) UnloadMusicStream(level2_music);
	if (IsMusicReady(level3_music)) UnloadMusicStream(level3_music);

	// Clean start menu texture
	if (start_menu_texture.id != 0)
	{
		UnloadTexture(start_menu_texture);
	}

	// Clean level select menu texture
	if (level_select_texture.id != 0)
	{
		UnloadTexture(level_select_texture);
	}

	UnloadTexture(tile_set);

	//Clean leaderboard
	if (leaderboard) {
		leaderboard->CleanUp();
	}

	//Clean characters
	if (character_select) {
		character_select->CleanUp();
	}

	//Clean Intro
	if (intro_spritesheet.id != 0) {
		UnloadTexture(intro_spritesheet);
	}

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
	// Update current music stream
	if (IsMusicReady(current_music) && IsMusicStreamPlaying(current_music))
	{
		UpdateMusicStream(current_music);
	}

	float dtt = GetFrameTime();
	if (menu_state == MenuState::INTRO_ANIMATION)
	{
		intro_timer += dtt;

		if (intro_timer >= intro_frame_duration) {
			intro_timer = 0.0f;
			intro_frame_actual++;

			if (intro_frame_actual >= intro_total_frames) {
				menu_state = MenuState::START_MENU;
				intro_frame_actual = 0; 
			}
		}

		ClearBackground(BLACK);

		if (intro_spritesheet.id != 0) {
			int columna = intro_frame_actual; 

			Rectangle source = {
				(float)(columna * intro_frame_width), 
				0.0f,                                  
				(float)intro_frame_width,              
				(float)intro_frame_height             
			};

			Rectangle dest = {0.0f,0.0f,(float)SCREEN_WIDTH,  (float)SCREEN_HEIGHT  };

			DrawTexturePro(intro_spritesheet, source, dest, { 0, 0 }, 0, WHITE);

			if (App->physics->debug) {
				DrawText(TextFormat("Frame: %d/%d", intro_frame_actual + 1, intro_total_frames),
					10, 10, 20, WHITE);
			}
		}
		else {
			DrawText("Cargando intro...", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, 20, WHITE);
		}


	}

	// Show initial menu and wait for SPACE
	if (menu_state == MenuState::START_MENU)
	{
		// Restart camera position
		App->renderer->camera_x = 0;
		App->renderer->camera_y = 0;

		// Clear screen
		ClearBackground(BLACK);

		// Draw start menu
		if (start_menu_texture.id != 0)
		{
			// Calculate scaling to fit screen while maintaining aspect ratio
			float scale_x = (float)SCREEN_WIDTH / start_menu_texture.width;
			float scale_y = (float)SCREEN_HEIGHT / start_menu_texture.height;
			float scale = fminf(scale_x, scale_y);

			int img_width = (int)(start_menu_texture.width * scale);
			int img_height = (int)(start_menu_texture.height * scale);
			int pos_x = (SCREEN_WIDTH - img_width) / 2;
			int pos_y = (SCREEN_HEIGHT - img_height) / 2;

			Rectangle source = { 0, 0, (float)start_menu_texture.width, (float)start_menu_texture.height };
			Rectangle dest = { (float)pos_x, (float)pos_y, (float)img_width, (float)img_height };
			Vector2 origin = { 0, 0 };

			DrawTexturePro(start_menu_texture, source, dest, origin, 0.0f, WHITE);
		}
		else
		{
			// Text menu fallback
			const char* title = "RACING GAME";
			const char* subtitle = "Physics II Project";

			int title_width = MeasureText(title, 80);
			int subtitle_width = MeasureText(subtitle, 30);

			DrawText(title, (SCREEN_WIDTH - title_width) / 2, 200, 80, YELLOW);
			DrawText(subtitle, (SCREEN_WIDTH - subtitle_width) / 2, 300, 30, LIGHTGRAY);
		}

		// Transition to character select on space key press
		if (IsKeyPressed(KEY_SPACE))
		{
			LOG("Transitioning to character select...");
			menu_state = MenuState::CHARACTER_SELECT;
			if (character_select) {
				character_select->Reset();
			}
		}

		return UPDATE_CONTINUE;
	}

	if (menu_state == MenuState::CHARACTER_SELECT)
	{
		App->renderer->camera_x = 0;
		App->renderer->camera_y = 0;

		float dt = GetFrameTime();

		if (character_select) {
			character_select->Update(dt);
			character_select->Draw();

			if (character_select->IsConfirmed()) {
				LOG("Character selected, moving to level select...");
				selected_player_car = character_select->GetSelectedCarTexture();
				menu_state = MenuState::LEVEL_SELECT;
			}
		}

		return UPDATE_CONTINUE;
	}


	// Show level selection and wait for 1/2/3
	if (menu_state == MenuState::LEVEL_SELECT)
	{
		// Keep camera reset
		App->renderer->camera_x = 0;
		App->renderer->camera_y = 0;

		ClearBackground(BLACK);

		// Draw level select menu
		if (level_select_texture.id != 0)
		{
			// Calculate scaling to fit screen while maintaining aspect ratio
			float scale_x = (float)SCREEN_WIDTH / level_select_texture.width;
			float scale_y = (float)SCREEN_HEIGHT / level_select_texture.height;
			float scale = fminf(scale_x, scale_y);

			int img_width = (int)(level_select_texture.width * scale);
			int img_height = (int)(level_select_texture.height * scale);
			int pos_x = (SCREEN_WIDTH - img_width) / 2;
			int pos_y = (SCREEN_HEIGHT - img_height) / 2;

			Rectangle source = { 0, 0, (float)level_select_texture.width, (float)level_select_texture.height };
			Rectangle dest = { (float)pos_x, (float)pos_y, (float)img_width, (float)img_height };
			Vector2 origin = { 0, 0 };

			DrawTexturePro(level_select_texture, source, dest, origin, 0.0f, WHITE);
		}

		// Level selection input
		if (IsKeyPressed(KEY_ONE))
		{
			StartGame("Assets/Map/RaceTrack.tmx");
			PlayBackgroundMusic(level1_music);
		}
		else if (IsKeyPressed(KEY_TWO))
		{
			StartGame("Assets/Map/RaceTrack2.tmx");
			PlayBackgroundMusic(level2_music);
		}
		else if (IsKeyPressed(KEY_THREE))
		{
			StartGame("Assets/Map/RaceTrack3.tmx");
			PlayBackgroundMusic(level3_music);
		}

		return UPDATE_CONTINUE;
	}

	// Playing state
	if (!game_started) return UPDATE_CONTINUE;

	// Check if player wants to return to level select
	if (IsKeyPressed(KEY_M))
	{
		LOG("Returning to level select menu...");
		ResetGame();
		menu_state = MenuState::LEVEL_SELECT;
		show_menu = true;
		game_started = false;
		PlayBackgroundMusic(menu_music);
		return UPDATE_CONTINUE;
	}

	// Draw map tiles
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

	if (menu_state == MenuState::PLAYING && game_started) {

		if (IsKeyPressed(KEY_TAB)) {
			leaderboard->SetVisible(!leaderboard->IsVisible());
		}

		std::vector<RacerInfo> racers;

		if (App->player->vehicle && App->player->vehicle->body) {
			RacerInfo player_info;
			player_info.name = "PLAYER";
			player_info.is_player = true;
			player_info.body = App->player->vehicle->body;

			player_info.current_waypoint = 0;

			if (!waypoints.empty()) {
				b2Vec2 player_pos = App->player->vehicle->body->GetPosition();
				b2Vec2 waypoint_pos = waypoints[0].position;
				b2Vec2 diff = waypoint_pos - player_pos;
				player_info.distance_to_next_waypoint = diff.Length();
			}
			else {
				player_info.distance_to_next_waypoint = 0.0f;
			}

			racers.push_back(player_info);
		}

		for (size_t i = 0; i < ai_vehicles.size(); ++i) {
			AIVehicle* ai = ai_vehicles[i];
			if (ai && ai->active && ai->body) {
				RacerInfo ai_info;

				char name_buffer[32];
				sprintf_s(name_buffer, "AI CAR %d", (int)i + 1);
				ai_info.name = name_buffer;

				ai_info.is_player = false;
				ai_info.body = ai->body;
				ai_info.current_waypoint = ai->current_waypoint_id;

				b2Vec2 ai_pos = ai->body->GetPosition();
				bool found_waypoint = false;

				for (const auto& wp : waypoints) {
					if (wp.id == ai->current_waypoint_id) {
						b2Vec2 diff = wp.position - ai_pos;
						ai_info.distance_to_next_waypoint = diff.Length();
						found_waypoint = true;
						break;
					}
				}

				if (!found_waypoint) {
					ai_info.distance_to_next_waypoint = 999.0f;
				}

				racers.push_back(ai_info);
			}
		}

		leaderboard->UpdatePositions(racers);

		leaderboard->Draw();
	}

	// Update AI vehicles
	float dt = GetFrameTime();
	for (auto* vehicle : ai_vehicles) {
		vehicle->Update(dt, waypoints);
		vehicle->Draw(App->physics->debug);
	}

	// Draw UI hint for returning to menu
	DrawText("[M] Back to Level Select", 10, SCREEN_HEIGHT - 30, 20, WHITE);

	return UPDATE_CONTINUE;
}

void ModuleGame::StartGame(const char* map_path)
{
	LOG("Starting game with map: %s", map_path);

	// Change to playing state
	menu_state = MenuState::PLAYING;
	show_menu = false;

	// Load selected map and its data
	LoadMap(map_path);
	LoadCollisions(map_path);
	LoadMapObjects(map_path);
	CreateCollisionBodies();
	CreateEnemiesAndPlayer();

	// Reset player nitro to full charge when starting a new level
	if (App->player)
	{
		App->player->ResetNitro();
	}

	game_started = true;
}

void ModuleGame::ResetGame()
{
	// Clean up AI vehicles and their physics bodies
	for (auto* vehicle : ai_vehicles) {
		if (vehicle && vehicle->body) {
			App->physics->GetWorld()->DestroyBody(vehicle->body);
			vehicle->body = nullptr;
		}
		delete vehicle;
	}
	ai_vehicles.clear();

	// Clean up collision bodies - only destroy Box2D body, PhysBody wrapper is deleted separately
	for (auto* pbody : collision_bodies) {
		if (pbody && pbody->body) {
			App->physics->GetWorld()->DestroyBody(pbody->body);
			pbody->body = nullptr;
		}
	}
	collision_bodies.clear();

	// Clear game data
	waypoints.clear();
	spawn_points.clear();
	map_data.clear();
	collision_objects.clear();

	// Reset player nitro when returning to menu
	if (App->player)
	{
		App->player->ResetNitro();
	}
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

	// Shuffle spawn points for random positioning
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(spawn_points.begin(), spawn_points.end(), g);

	// Assign first spawn to player
	if (App->player != nullptr && App->player->vehicle != nullptr) {
		if (selected_player_car.id != 0) {
			if (App->player->vehicle_texture.id != 0) {
				UnloadTexture(App->player->vehicle_texture);
			}
			App->player->vehicle_texture = selected_player_car;
		}
		App->player->SetPosition(spawn_points[0].x, spawn_points[0].y, -90.0f);
		LOG("Player teleported to spawn: %.2f, %.2f", spawn_points[0].x, spawn_points[0].y);
	}
	else {
		LOG("ERROR: Player vehicle not ready during spawn assignment!");
	}

	// Create AI vehicles at remaining spawn points
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
			vec2i absolute_point(0, 0);
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

void ModuleGame::PlayBackgroundMusic(Music music)
{
	// Stop current music if playing
	StopCurrentMusic();

	// Play new music
	if (IsMusicReady(music)) {
		current_music = music;
		PlayMusicStream(current_music);
		SetMusicVolume(current_music, 1.0f);
		LOG("Playing background music");
	}
	else {
		LOG("WARNING: Music not ready!");
	}
}

void ModuleGame::StopCurrentMusic()
{
	if (IsMusicReady(current_music) && IsMusicStreamPlaying(current_music)) {
		StopMusicStream(current_music);
		LOG("Stopped background music");
	}
}