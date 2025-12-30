#pragma once
#include "Module.h"
#include "Globals.h"
#include "p2Point.h"
#include "raylib.h"
#include <vector>

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

	void SetPosition(float x, float y, float rotation_degrees = 0.0f);

	// Reset nitro to full charge (called when changing levels)
	void ResetNitro();

	// Nitro system methods
	void UpdateNitro(float dt);
	void DrawNitroBar();
	void DrawNitroEffects();
	void OnCollision(PhysBody* bodyA, PhysBody* bodyB) override;

public:
	PhysBody* vehicle;
	Texture2D vehicle_texture;

	// Nitro system variables
	bool nitro_active;
	float nitro_duration;
	float nitro_timer;
	float nitro_cooldown;
	float nitro_cooldown_timer;
	const float NITRO_MAX_DURATION = 5.0f;
	const float NITRO_COOLDOWN_TIME = 30.0f;

	// Visual effects for nitro
	float nitro_particle_timer;
	struct NitroParticle
	{
		vec2f position = { 0.0f, 0.0f };
		vec2f velocity = { 0.0f, 0.0f };
		float lifetime = 0.0f;
		float max_lifetime = 0.0f;
		Color color = WHITE;
	};
	std::vector<NitroParticle> nitro_particles;

	unsigned int sfx_engine;
	unsigned int sfx_crash;
	unsigned int sfx_nitro;
	unsigned int sfx_drift;
};