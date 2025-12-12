#pragma once
#include "Module.h"
#include "Globals.h"
#include "p2Point.h"
#include "raylib.h"

// Ignoramos warnings de Box2D para que no fallen al compilar
#pragma warning(push)
#pragma warning(disable : 26495)
#include "ModulePhysics.h"
#pragma warning(pop)

class ModulePlayer : public Module
{
public:
	ModulePlayer(Application* app, bool start_enabled = true);
	virtual ~ModulePlayer();

	bool Start();
	update_status Update();
	bool CleanUp();

public:
	PhysBody* vehicle;
	Texture2D vehicle_texture;
};