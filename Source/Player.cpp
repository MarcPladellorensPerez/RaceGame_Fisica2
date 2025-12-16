#include "Globals.h"
#include "Application.h"
#include "Player.h"
#include "ModuleRender.h"
#include <stdlib.h>

#pragma warning(push)
#pragma warning(disable : 26495)
#include "ModulePhysics.h"
#pragma warning(pop)

ModulePlayer::ModulePlayer(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	vehicle = nullptr;
	vehicle_texture = { 0 };

	// Initialize nitro system
	nitro_active = false;
	nitro_duration = NITRO_MAX_DURATION;
	nitro_timer = 0.0f;
	nitro_cooldown = NITRO_COOLDOWN_TIME;
	nitro_cooldown_timer = 0.0f;
	nitro_particle_timer = 0.0f;
}

ModulePlayer::~ModulePlayer()
{
}

bool ModulePlayer::Start()
{
	LOG("ModulePlayer: Starting...");

	vehicle_texture = LoadTexture("Assets/Textures/Cars/car1.png");

	if (vehicle_texture.id == 0)
	{
		LOG("ERROR loading vehicle texture");
		return false;
	}

	LOG("ModulePlayer: Texture loaded successfully (%dx%d)", vehicle_texture.width, vehicle_texture.height);

	// Create vehicle body at starting position
	int start_x = 5208;
	int start_y = 5826;

	// Vehicle collider
	vehicle = App->physics->CreateRectangle(start_x, start_y,
		vehicle_texture.width,
		vehicle_texture.height,
		PhysBodyType::DYNAMIC);

	if (vehicle == nullptr || vehicle->body == nullptr)
	{
		LOG("ERROR: Could not create vehicle physics body!");
		return false;
	}

	// Vehicle physics properties
	vehicle->body->SetAngularDamping(3.0f);
	vehicle->body->SetLinearDamping(1.5f);
	vehicle->body->SetFixedRotation(false);

	// Configure fixture properties
	b2Fixture* fixture = vehicle->body->GetFixtureList();
	if (fixture)
	{
		fixture->SetDensity(1.2f);
		fixture->SetFriction(0.3f);
		fixture->SetRestitution(0.1f);
		vehicle->body->ResetMassData();
	}

	LOG("ModulePlayer: Vehicle initialized successfully at (%d, %d)", start_x, start_y);

	return true;
}


bool ModulePlayer::CleanUp()
{
	UnloadTexture(vehicle_texture);
	nitro_particles.clear();
	return true;
}

void ModulePlayer::UpdateNitro(float dt)
{
	// Update nitro cooldown
	if (nitro_cooldown_timer > 0.0f)
	{
		nitro_cooldown_timer -= dt;
		if (nitro_cooldown_timer < 0.0f)
		{
			nitro_cooldown_timer = 0.0f;
			nitro_duration = NITRO_MAX_DURATION;
		}
		else
		{
			// Gradually recharge during cooldown
			nitro_duration = NITRO_MAX_DURATION * (1.0f - (nitro_cooldown_timer / NITRO_COOLDOWN_TIME));
		}
	}

	// Activate nitro with N key
	if (IsKeyPressed(KEY_N) && nitro_duration >= NITRO_MAX_DURATION && nitro_cooldown_timer <= 0.0f)
	{
		nitro_active = true;
		nitro_timer = 0.0f;
		LOG("ModulePlayer: NITRO ACTIVATED!");
	}

	// Update active nitro
	if (nitro_active)
	{
		nitro_timer += dt;

		if (nitro_timer >= NITRO_MAX_DURATION)
		{
			// Nitro depleted
			nitro_active = false;
			nitro_timer = 0.0f;
			nitro_duration = 0.0f;
			nitro_cooldown_timer = NITRO_COOLDOWN_TIME;
			LOG("ModulePlayer: Nitro depleted, entering cooldown");
		}
	}
}

void ModulePlayer::DrawNitroBar()
{
	// Nitro bar position and size
	int bar_x = SCREEN_WIDTH - 250;
	int bar_y = 30;
	int bar_width = 200;
	int bar_height = 30;
	int border_thickness = 3;

	// Draw bar background
	DrawRectangle(bar_x, bar_y, bar_width, bar_height, ColorAlpha(BLACK, 0.6f));

	// Draw bar border
	DrawRectangleLinesEx({ (float)bar_x, (float)bar_y, (float)bar_width, (float)bar_height },
		border_thickness, WHITE);

	// Calculate fill percentage
	float fill_percentage = 0.0f;
	Color fill_color;

	if (nitro_active)
	{
		// Show remaining time during nitro use
		fill_percentage = 1.0f - (nitro_timer / NITRO_MAX_DURATION);
		fill_color = Color{ 0, 150, 255, 255 }; // Bright blue
	}
	else if (nitro_cooldown_timer > 0.0f)
	{
		// Show recharge progress during cooldown
		fill_percentage = nitro_duration / NITRO_MAX_DURATION;
		fill_color = Color{ 100, 100, 150, 255 };
	}
	else
	{
		// Fully charged
		fill_percentage = 1.0f;
		fill_color = Color{ 0, 200, 255, 255 };
	}

	// Draw fill with glow effect
	int fill_width = (int)((bar_width - border_thickness * 2) * fill_percentage);
	if (fill_width > 0)
	{
		// Inner glow
		DrawRectangle(bar_x + border_thickness, bar_y + border_thickness,
			fill_width, bar_height - border_thickness * 2, fill_color);

		// Outer glow when fully charged
		if (fill_percentage >= 1.0f && !nitro_active)
		{
			DrawRectangleLinesEx({ (float)bar_x - 2, (float)bar_y - 2,
				(float)bar_width + 4, (float)bar_height + 4 }, 2,
				ColorAlpha(fill_color, 0.5f));
		}
	}

	// Draw text label
	const char* label = "NITRO";
	DrawText(label, bar_x + 10, bar_y + 7, 16, WHITE);

	// Draw status text
	if (nitro_active)
	{
		const char* status = "BOOST!";
		DrawText(status, bar_x + bar_width - 70, bar_y + 7, 16, YELLOW);
	}
	else if (nitro_cooldown_timer > 0.0f)
	{
		char cooldown_text[32];
		sprintf_s(cooldown_text, "%.1fs", nitro_cooldown_timer);
		DrawText(cooldown_text, bar_x + bar_width - 50, bar_y + 7, 16, LIGHTGRAY);
	}
	else
	{
		const char* status = "READY";
		DrawText(status, bar_x + bar_width - 60, bar_y + 7, 16, GREEN);
	}

	// Draw control hint
	if (!nitro_active && nitro_cooldown_timer <= 0.0f)
	{
		DrawText("Press [N]", bar_x + 50, bar_y + bar_height + 5, 14, LIGHTGRAY);
	}
}

void ModulePlayer::DrawNitroEffects()
{
	if (!nitro_active) return;

	int x, y;
	vehicle->GetPosition(x, y);
	float rotation = vehicle->GetRotation() * DEG_TO_RAD;

	// Calculate rear position of the vehicle
	float rear_offset = vehicle_texture.height * 0.4f;
	float rear_x = x - sin(rotation) * rear_offset;
	float rear_y = y + cos(rotation) * rear_offset;

	// Spawn particles
	nitro_particle_timer += GetFrameTime();
	if (nitro_particle_timer >= 0.02f)
	{
		nitro_particle_timer = 0.0f;

		// Create multiple particles
		for (int i = 0; i < 3; i++)
		{
			NitroParticle particle;

			// Random offset from center
			float offset_x = ((rand() % 20) - 10) * 0.5f;
			float offset_y = ((rand() % 20) - 10) * 0.5f;

			particle.position.x = rear_x + offset_x;
			particle.position.y = rear_y + offset_y;

			// Velocity opposite to vehicle direction
			float speed = 100.0f + (rand() % 50);
			particle.velocity.x = sin(rotation) * speed + ((rand() % 40) - 20);
			particle.velocity.y = -cos(rotation) * speed + ((rand() % 40) - 20);

			particle.max_lifetime = 0.5f + (rand() % 100) / 200.0f;
			particle.lifetime = particle.max_lifetime;

			// Blue flame colors
			int color_variant = rand() % 3;
			if (color_variant == 0)
				particle.color = Color{ 0, 150, 255, 255 };
			else if (color_variant == 1)
				particle.color = Color{ 100, 200, 255, 255 };
			else
				particle.color = Color{ 255, 255, 255, 255 };

			nitro_particles.push_back(particle);
		}
	}

	// Update and draw particles
	float dt = GetFrameTime();
	for (size_t i = 0; i < nitro_particles.size(); )
	{
		NitroParticle& p = nitro_particles[i];
		p.lifetime -= dt;

		if (p.lifetime <= 0.0f)
		{
			// Remove dead particle
			nitro_particles.erase(nitro_particles.begin() + i);
			continue;
		}

		// Update position
		p.position.x += p.velocity.x * dt;
		p.position.y += p.velocity.y * dt;

		// Fade out
		float alpha = p.lifetime / p.max_lifetime;
		Color draw_color = p.color;
		draw_color.a = (unsigned char)(255 * alpha);

		// Calculate size based on lifetime
		float size = 8.0f * (1.0f + (1.0f - alpha) * 2.0f);

		// Draw particle with camera offset
		float screen_x = p.position.x + App->renderer->camera_x;
		float screen_y = p.position.y + App->renderer->camera_y;

		// Draw glow effect
		DrawCircle((int)screen_x, (int)screen_y, size + 4, ColorAlpha(draw_color, alpha * 0.3f));
		DrawCircle((int)screen_x, (int)screen_y, size, draw_color);

		i++;
	}

	// Draw flame trails
	DrawCircle((int)(rear_x + App->renderer->camera_x),
		(int)(rear_y + App->renderer->camera_y),
		15, ColorAlpha(Color{ 0, 150, 255, 255 }, 0.6f));
	DrawCircle((int)(rear_x + App->renderer->camera_x),
		(int)(rear_y + App->renderer->camera_y),
		10, ColorAlpha(Color{ 100, 200, 255, 255 }, 0.8f));
}

update_status ModulePlayer::Update()
{
	if (vehicle == nullptr || vehicle->body == nullptr)
	{
		return UPDATE_CONTINUE;
	}

	float dt = GetFrameTime();

	// Update nitro system
	UpdateNitro(dt);

	// Movement parameters
	float acceleration = 25.0f;
	float max_speed_forward = 6.0f;
	float max_speed_reverse = 4.5f;
	float base_turn_speed = 3.0f;
	float drift_factor = 0.7f;
	float brake_strength = 8.0f;
	float handbrake_drift_force = 25.0f;

	// Nitro boost multipliers
	if (nitro_active)
	{
		acceleration *= 2.5f;
		max_speed_forward *= 1.8f;
	}

	// Get current velocity
	b2Vec2 velocity = vehicle->body->GetLinearVelocity();
	float speed = velocity.Length();

	// Calculate movement direction
	b2Vec2 forward_vector = vehicle->body->GetWorldVector(b2Vec2(0.0f, -1.0f));
	float forward_velocity = b2Dot(forward_vector, velocity);
	bool moving_forward = forward_velocity > 0.1f;
	bool moving_backward = forward_velocity < -0.1f;
	bool is_stopped = speed < 0.1f;

	// Acceleration control (W or up arrow)
	if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
	{
		b2Vec2 forward = vehicle->body->GetWorldVector(b2Vec2(0.0f, -1.0f));
		forward.x *= acceleration;
		forward.y *= acceleration;
		vehicle->body->ApplyForceToCenter(forward, true);
	}

	// Reverse and braking control (S or down arrow)
	if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
	{
		if (moving_forward && speed > 1.0f)
		{
			// If moving forward with significant speed, brake first
			b2Vec2 brake_force;
			brake_force.x = -velocity.x * vehicle->body->GetMass() * brake_strength;
			brake_force.y = -velocity.y * vehicle->body->GetMass() * brake_strength;
			vehicle->body->ApplyForceToCenter(brake_force, true);
		}
		else
		{
			// If stopped or moving slowly, apply reverse with full acceleration
			b2Vec2 backward = vehicle->body->GetWorldVector(b2Vec2(0.0f, 1.0f));
			backward.x *= acceleration;
			backward.y *= acceleration;
			vehicle->body->ApplyForceToCenter(backward, true);
		}
	}

	// Direction control (A/D or left/right arrows)
	bool turning_left = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
	bool turning_right = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
	bool is_turning = turning_left || turning_right;

	// Hand brake (space bar)
	bool handbrake_active = IsKeyDown(KEY_SPACE);

	if (handbrake_active)
	{
		if (is_turning && !is_stopped)
		{
			// DRIFT MODE: Handbrake + turn

			// Drastically reduce lateral grip to allow sliding
			b2Vec2 right_vector = vehicle->body->GetWorldVector(b2Vec2(1.0f, 0.0f));
			float lateral_velocity = b2Dot(right_vector, velocity);

			// Apply minimal lateral correction
			b2Vec2 drift_impulse;
			drift_impulse.x = -lateral_velocity * 0.3f * vehicle->body->GetMass() * right_vector.x;
			drift_impulse.y = -lateral_velocity * 0.3f * vehicle->body->GetMass() * right_vector.y;
			vehicle->body->ApplyLinearImpulseToCenter(drift_impulse, true);

			// Rear slides more than front for visual drift
			float drift_torque = 0.0f;
			if (turning_left)
			{
				drift_torque = -handbrake_drift_force * (speed / max_speed_forward);
			}
			else if (turning_right)
			{
				drift_torque = handbrake_drift_force * (speed / max_speed_forward);
			}

			vehicle->body->ApplyTorque(drift_torque, true);

			// Gradually reduce speed during drift
			b2Vec2 drift_brake;
			drift_brake.x = -velocity.x * vehicle->body->GetMass() * 2.0f;
			drift_brake.y = -velocity.y * vehicle->body->GetMass() * 2.0f;
			vehicle->body->ApplyForceToCenter(drift_brake, true);
		}
		else
		{
			// NORMAL BRAKE MODE: Handbrake without turning = gradual braking
			b2Vec2 brake_force;
			brake_force.x = -velocity.x * vehicle->body->GetMass() * 5.0f;
			brake_force.y = -velocity.y * vehicle->body->GetMass() * 5.0f;
			vehicle->body->ApplyForceToCenter(brake_force, true);
		}
	}
	else
	{
		// Normal steering without handbrake
		if (speed > 0.5f && is_turning)
		{
			// Calculate dynamic turn speed based on current speed
			float speed_ratio = moving_forward ? (speed / max_speed_forward) : (speed / max_speed_reverse);

			// Reduce turn rate at high speeds
			float turn_reduction = 1.0f - (speed_ratio * 0.5f);
			turn_reduction = fmaxf(turn_reduction, 0.4f);

			float dynamic_turn_speed = base_turn_speed * turn_reduction;

			if (turning_left)
			{
				vehicle->body->SetAngularVelocity(-dynamic_turn_speed);
			}
			else if (turning_right)
			{
				vehicle->body->SetAngularVelocity(dynamic_turn_speed);
			}
		}
		else if (!is_turning)
		{
			// Gradually reduce angular velocity when not turning
			vehicle->body->SetAngularVelocity(vehicle->body->GetAngularVelocity() * 0.9f);
		}

		// Anti-drift system (only when not in drift mode)
		b2Vec2 right_vector = vehicle->body->GetWorldVector(b2Vec2(1.0f, 0.0f));
		float lateral_velocity = b2Dot(right_vector, velocity);

		b2Vec2 impulse;
		impulse.x = -lateral_velocity * drift_factor * vehicle->body->GetMass() * right_vector.x;
		impulse.y = -lateral_velocity * drift_factor * vehicle->body->GetMass() * right_vector.y;
		vehicle->body->ApplyLinearImpulseToCenter(impulse, true);
	}

	// Max speed limit (different for forward and reverse)
	if (moving_forward && speed > max_speed_forward)
	{
		velocity.Normalize();
		velocity.x *= max_speed_forward;
		velocity.y *= max_speed_forward;
		vehicle->body->SetLinearVelocity(velocity);
	}
	else if (moving_backward && speed > max_speed_reverse)
	{
		velocity.Normalize();
		velocity.x *= max_speed_reverse;
		velocity.y *= max_speed_reverse;
		vehicle->body->SetLinearVelocity(velocity);
	}

	// Position and rotation of the vehicle
	int x, y;
	vehicle->GetPosition(x, y);
	float rotation = vehicle->GetRotation();

	// Update camera to follow vehicle
	App->renderer->UpdateCamera((float)x, (float)y, 0.1f);

	// Draw nitro visual effects
	DrawNitroEffects();

	// Draw vehicle
	Rectangle source = { 0, 0, (float)vehicle_texture.width, (float)vehicle_texture.height };
	App->renderer->Draw(vehicle_texture, x, y, &source, rotation,
		vehicle_texture.width / 2, vehicle_texture.height / 2);

	// Draw nitro bar (HUD)
	DrawNitroBar();

	// Debug info on screen (in game debug mode)
	if (App->physics->debug)
	{
		DrawText(TextFormat("Speed: %.1f", speed), 10, 135, 16, GREEN);
		DrawText(TextFormat("Position: (%d, %d)", x, y), 10, 155, 16, GREEN);
		DrawText(TextFormat("Rotation: %.1f", rotation), 10, 175, 16, GREEN);

		if (nitro_active)
		{
			DrawText("*** NITRO ACTIVE ***", 10, 195, 20, SKYBLUE);
		}
		else if (handbrake_active && is_turning)
		{
			DrawText("*** DRIFT MODE ***", 10, 195, 20, ORANGE);
		}
		else if (handbrake_active)
		{
			DrawText("BRAKING", 10, 195, 16, RED);
		}

		if (moving_forward)
		{
			DrawText("Direction: FORWARD", 10, 215, 16, BLUE);
		}
		else if (moving_backward)
		{
			DrawText("Direction: BACKWARD", 10, 215, 16, BLUE);
		}
	}

	return UPDATE_CONTINUE;
}