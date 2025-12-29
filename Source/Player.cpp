#include "Globals.h"
#include "Application.h"
#include "Player.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h" 
#include <stdlib.h>
#include <cmath>

#pragma warning(push)
#pragma warning(disable : 26495)
#include "ModulePhysics.h"
#pragma warning(pop)

ModulePlayer::ModulePlayer(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	vehicle = nullptr;
	vehicle_texture = { 0 };

	nitro_active = false;
	nitro_duration = NITRO_MAX_DURATION;
	nitro_timer = 0.0f;
	nitro_cooldown = NITRO_COOLDOWN_TIME;
	nitro_cooldown_timer = 0.0f;
	nitro_particle_timer = 0.0f;

	// Initialize sound IDs
	sfx_engine = 0;
	sfx_crash = 0;
	sfx_nitro = 0;
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

	// Temporal starting position
	int start_x = 0;
	int start_y = 0;

	vehicle = App->physics->CreateRectangle(start_x, start_y,
		vehicle_texture.width,
		vehicle_texture.height,
		PhysBodyType::DYNAMIC);

	if (vehicle == nullptr || vehicle->body == nullptr)
	{
		LOG("ERROR: Could not create vehicle physics body!");
		return false;
	}

	// Configure vehicle physics
	vehicle->body->SetAngularDamping(3.0f);
	vehicle->body->SetLinearDamping(1.5f);
	vehicle->body->SetFixedRotation(false);

	b2Fixture* fixture = vehicle->body->GetFixtureList();
	if (fixture)
	{
		fixture->SetDensity(1.2f);
		fixture->SetFriction(0.3f);
		fixture->SetRestitution(0.1f);
		vehicle->body->ResetMassData();
	}

	// Assign listener for collisions
	vehicle->listener = this;

	// Load sound effects
	LOG("Loading player sounds...");
	sfx_engine = App->audio->LoadFx("Assets/Audio/car/engine_loop.wav");
	sfx_crash = App->audio->LoadFx("Assets/Audio/car/crash.wav");
	sfx_nitro = App->audio->LoadFx("Assets/Audio/car/nitro.wav");

	return true;
}

void ModulePlayer::SetPosition(float x, float y, float rotation_degrees)
{
	if (vehicle && vehicle->body)
	{
		// Convert degrees to radians for Box2D
		float angle_radians = rotation_degrees * DEG_TO_RAD;

		vehicle->body->SetLinearVelocity(b2Vec2(0, 0));
		vehicle->body->SetAngularVelocity(0);
		vehicle->body->SetTransform(b2Vec2(PIXELS_TO_METERS(x), PIXELS_TO_METERS(y)), angle_radians);
	}
}

// Reset nitro system to initial state (fully charged)
void ModulePlayer::ResetNitro()
{
	nitro_active = false;
	nitro_duration = NITRO_MAX_DURATION;
	nitro_timer = 0.0f;
	nitro_cooldown_timer = 0.0f;
	nitro_particle_timer = 0.0f;
	nitro_particles.clear();

	LOG("ModulePlayer: Nitro system reset to full charge");
}

bool ModulePlayer::CleanUp()
{
	UnloadTexture(vehicle_texture);
	nitro_particles.clear();
	return true;
}

void ModulePlayer::UpdateNitro(float dt)
{
	// Handle cooldown period
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
			// Gradually refill the bar during cooldown
			nitro_duration = NITRO_MAX_DURATION * (1.0f - (nitro_cooldown_timer / NITRO_COOLDOWN_TIME));
		}
	}

	// Activate nitro when N is pressed
	if (IsKeyPressed(KEY_N) && nitro_duration >= NITRO_MAX_DURATION && nitro_cooldown_timer <= 0.0f)
	{
		nitro_active = true;
		nitro_timer = 0.0f;

		// Play nitro sound
		App->audio->PlayFx(sfx_nitro);
	}

	// Update nitro timer and deactivate when duration is reached
	if (nitro_active)
	{
		nitro_timer += dt;
		if (nitro_timer >= NITRO_MAX_DURATION)
		{
			nitro_active = false;
			nitro_timer = 0.0f;
			nitro_duration = 0.0f;
			nitro_cooldown_timer = NITRO_COOLDOWN_TIME;
		}
	}
}

void ModulePlayer::DrawNitroBar()
{
	int bar_x = SCREEN_WIDTH - 250;
	int bar_y = 30;
	int bar_width = 200;
	int bar_height = 30;
	int border_thickness = 3;

	// Draw bar background
	DrawRectangle(bar_x, bar_y, bar_width, bar_height, ColorAlpha(BLACK, 0.6f));
	DrawRectangleLinesEx({ (float)bar_x, (float)bar_y, (float)bar_width, (float)bar_height },
		(float)border_thickness, WHITE);

	float fill_percentage = 0.0f;
	Color fill_color;

	// Determine bar state and color
	if (nitro_active)
	{
		fill_percentage = 1.0f - (nitro_timer / NITRO_MAX_DURATION);
		fill_color = Color{ 0, 150, 255, 255 };
	}
	else if (nitro_cooldown_timer > 0.0f)
	{
		fill_percentage = nitro_duration / NITRO_MAX_DURATION;
		fill_color = Color{ 100, 100, 150, 255 };
	}
	else
	{
		fill_percentage = 1.0f;
		fill_color = Color{ 0, 200, 255, 255 };
	}

	// Draw fill bar
	int fill_width = (int)((bar_width - border_thickness * 2) * fill_percentage);
	if (fill_width > 0)
	{
		DrawRectangle(bar_x + border_thickness, bar_y + border_thickness,
			fill_width, bar_height - border_thickness * 2, fill_color);

		// Draw glow effect when ready
		if (fill_percentage >= 1.0f && !nitro_active)
		{
			DrawRectangleLinesEx({ (float)bar_x - 2, (float)bar_y - 2,
				(float)bar_width + 4, (float)bar_height + 4 }, 2.0f,
				ColorAlpha(fill_color, 0.5f));
		}
	}

	// Draw text labels
	DrawText("NITRO", bar_x + 10, bar_y + 7, 16, WHITE);

	if (nitro_active)
	{
		DrawText("BOOST!", bar_x + bar_width - 70, bar_y + 7, 16, YELLOW);
	}
	else if (nitro_cooldown_timer > 0.0f)
	{
		char cooldown_text[32];
		sprintf_s(cooldown_text, "%.1fs", nitro_cooldown_timer);
		DrawText(cooldown_text, bar_x + bar_width - 50, bar_y + 7, 16, LIGHTGRAY);
	}
	else
	{
		DrawText("READY", bar_x + bar_width - 60, bar_y + 7, 16, GREEN);
	}

	// Draw instruction text
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

	// Calculate rear position for particle emission
	float rear_offset = vehicle_texture.height * 0.4f;
	float rear_x = x - sinf(rotation) * rear_offset;
	float rear_y = y + cosf(rotation) * rear_offset;

	// Generate new particles
	nitro_particle_timer += GetFrameTime();
	if (nitro_particle_timer >= 0.02f)
	{
		nitro_particle_timer = 0.0f;

		for (int i = 0; i < 3; i++)
		{
			NitroParticle particle;
			float offset_x = ((rand() % 20) - 10) * 0.5f;
			float offset_y = ((rand() % 20) - 10) * 0.5f;

			particle.position.x = rear_x + offset_x;
			particle.position.y = rear_y + offset_y;

			float speed = 100.0f + (rand() % 50);
			particle.velocity.x = sinf(rotation) * speed + ((rand() % 40) - 20);
			particle.velocity.y = -cosf(rotation) * speed + ((rand() % 40) - 20);

			particle.max_lifetime = 0.5f + (rand() % 100) / 200.0f;
			particle.lifetime = particle.max_lifetime;

			// Random color variants
			int color_variant = rand() % 3;
			if (color_variant == 0) particle.color = Color{ 0, 150, 255, 255 };
			else if (color_variant == 1) particle.color = Color{ 100, 200, 255, 255 };
			else particle.color = Color{ 255, 255, 255, 255 };

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
			nitro_particles.erase(nitro_particles.begin() + i);
			continue;
		}

		// Update particle position
		p.position.x += p.velocity.x * dt;
		p.position.y += p.velocity.y * dt;

		// Calculate alpha based on lifetime
		float alpha = p.lifetime / p.max_lifetime;
		Color draw_color = p.color;
		draw_color.a = (unsigned char)(255 * alpha);

		// Draw particle with growing size
		float size = 8.0f * (1.0f + (1.0f - alpha) * 2.0f);
		float screen_x = p.position.x + App->renderer->camera_x;
		float screen_y = p.position.y + App->renderer->camera_y;

		DrawCircle((int)screen_x, (int)screen_y, size + 4, ColorAlpha(draw_color, alpha * 0.3f));
		DrawCircle((int)screen_x, (int)screen_y, size, draw_color);

		i++;
	}

	// Draw main nitro glow at vehicle rear
	DrawCircle((int)(rear_x + App->renderer->camera_x),
		(int)(rear_y + App->renderer->camera_y),
		15.0f, ColorAlpha(Color{ 0, 150, 255, 255 }, 0.6f));
	DrawCircle((int)(rear_x + App->renderer->camera_x),
		(int)(rear_y + App->renderer->camera_y),
		10.0f, ColorAlpha(Color{ 100, 200, 255, 255 }, 0.8f));
}

update_status ModulePlayer::Update()
{
	if (vehicle == nullptr || vehicle->body == nullptr) return UPDATE_CONTINUE;

	// Check if menu is shown, if so, don't update player
	if (App->scene_intro->show_menu)
	{
		// Keep vehicle stopped while in menu
		vehicle->body->SetLinearVelocity(b2Vec2(0, 0));
		vehicle->body->SetAngularVelocity(0);
		// Stop engine sound if inside menu
		if (App->audio->IsFxPlaying(sfx_engine)) {
			App->audio->SetFxVolume(sfx_engine, 0.0f);
		}
		return UPDATE_CONTINUE;
	}

	float dt = GetFrameTime();
	UpdateNitro(dt);

	// Vehicle physics parameters
	float acceleration = 25.0f;
	float max_speed_forward = 6.0f;
	float max_speed_reverse = 4.5f;
	float base_turn_speed = 3.0f;
	float drift_factor = 0.7f;
	float brake_strength = 8.0f;
	float handbrake_drift_force = 25.0f;

	// Apply nitro boost
	if (nitro_active)
	{
		acceleration *= 2.5f;
		max_speed_forward *= 1.8f;
	}

	b2Vec2 velocity = vehicle->body->GetLinearVelocity();
	float speed = velocity.Length();

	if (!App->audio->IsFxPlaying(sfx_engine)) {
		App->audio->PlayFx(sfx_engine, -1);
	}

	// Adjust pitch and volume based on speed
	float pitch = 0.6f + (speed * 0.08f);
	if (pitch > 2.0f) pitch = 2.0f;
	App->audio->SetFxPitch(sfx_engine, pitch);
	App->audio->SetFxVolume(sfx_engine, 0.5f);


	b2Vec2 forward_vector = vehicle->body->GetWorldVector(b2Vec2(0.0f, -1.0f));
	float forward_velocity = b2Dot(forward_vector, velocity);
	bool moving_forward = forward_velocity > 0.1f;
	bool moving_backward = forward_velocity < -0.1f;
	bool is_stopped = speed < 0.1f;

	// Forward acceleration
	if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
	{
		b2Vec2 forward = vehicle->body->GetWorldVector(b2Vec2(0.0f, -1.0f));
		forward.x *= acceleration;
		forward.y *= acceleration;
		vehicle->body->ApplyForceToCenter(forward, true);
	}

	// Reverse / Brake
	if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
	{
		if (moving_forward && speed > 1.0f)
		{
			// Apply braking force
			b2Vec2 brake_force;
			brake_force.x = -velocity.x * vehicle->body->GetMass() * brake_strength;
			brake_force.y = -velocity.y * vehicle->body->GetMass() * brake_strength;
			vehicle->body->ApplyForceToCenter(brake_force, true);
		}
		else
		{
			// Apply reverse force
			b2Vec2 backward = vehicle->body->GetWorldVector(b2Vec2(0.0f, 1.0f));
			backward.x *= acceleration;
			backward.y *= acceleration;
			vehicle->body->ApplyForceToCenter(backward, true);
		}
	}

	bool turning_left = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
	bool turning_right = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
	bool is_turning = turning_left || turning_right;
	bool handbrake_active = IsKeyDown(KEY_SPACE);

	if (handbrake_active)
	{
		if (is_turning && !is_stopped)
		{
			// Apply drift physics
			b2Vec2 right_vector = vehicle->body->GetWorldVector(b2Vec2(1.0f, 0.0f));
			float lateral_velocity = b2Dot(right_vector, velocity);

			b2Vec2 drift_impulse;
			drift_impulse.x = -lateral_velocity * 0.3f * vehicle->body->GetMass() * right_vector.x;
			drift_impulse.y = -lateral_velocity * 0.3f * vehicle->body->GetMass() * right_vector.y;
			vehicle->body->ApplyLinearImpulseToCenter(drift_impulse, true);

			// Apply drift torque
			float drift_torque = 0.0f;
			if (turning_left) drift_torque = -handbrake_drift_force * (speed / max_speed_forward);
			else if (turning_right) drift_torque = handbrake_drift_force * (speed / max_speed_forward);

			vehicle->body->ApplyTorque(drift_torque, true);

			// Apply drift brake
			b2Vec2 drift_brake;
			drift_brake.x = -velocity.x * vehicle->body->GetMass() * 2.0f;
			drift_brake.y = -velocity.y * vehicle->body->GetMass() * 2.0f;
			vehicle->body->ApplyForceToCenter(drift_brake, true);
		}
		else
		{
			// Simple handbrake when not turning
			b2Vec2 brake_force;
			brake_force.x = -velocity.x * vehicle->body->GetMass() * 5.0f;
			brake_force.y = -velocity.y * vehicle->body->GetMass() * 5.0f;
			vehicle->body->ApplyForceToCenter(brake_force, true);
		}
	}
	else
	{
		// Normal steering
		if (speed > 0.5f && is_turning)
		{
			float speed_ratio = moving_forward ? (speed / max_speed_forward) : (speed / max_speed_reverse);
			float turn_reduction = 1.0f - (speed_ratio * 0.5f);
			turn_reduction = fmaxf(turn_reduction, 0.4f);

			float dynamic_turn_speed = base_turn_speed * turn_reduction;

			if (turning_left) vehicle->body->SetAngularVelocity(-dynamic_turn_speed);
			else if (turning_right) vehicle->body->SetAngularVelocity(dynamic_turn_speed);
		}
		else if (!is_turning)
		{
			vehicle->body->SetAngularVelocity(vehicle->body->GetAngularVelocity() * 0.9f);
		}

		// Apply lateral friction for realistic car physics
		b2Vec2 right_vector = vehicle->body->GetWorldVector(b2Vec2(1.0f, 0.0f));
		float lateral_velocity = b2Dot(right_vector, velocity);

		b2Vec2 impulse;
		impulse.x = -lateral_velocity * drift_factor * vehicle->body->GetMass() * right_vector.x;
		impulse.y = -lateral_velocity * drift_factor * vehicle->body->GetMass() * right_vector.y;
		vehicle->body->ApplyLinearImpulseToCenter(impulse, true);
	}

	// Limit forward speed
	if (moving_forward && speed > max_speed_forward)
	{
		velocity.Normalize();
		velocity.x *= max_speed_forward;
		velocity.y *= max_speed_forward;
		vehicle->body->SetLinearVelocity(velocity);
	}
	// Limit reverse speed
	else if (moving_backward && speed > max_speed_reverse)
	{
		velocity.Normalize();
		velocity.x *= max_speed_reverse;
		velocity.y *= max_speed_reverse;
		vehicle->body->SetLinearVelocity(velocity);
	}

	// Update camera to follow player
	int x, y;
	vehicle->GetPosition(x, y);
	float rotation = vehicle->GetRotation();

	App->renderer->UpdateCamera((float)x, (float)y, 0.1f);

	// Draw nitro effects
	DrawNitroEffects();

	// Draw vehicle
	Rectangle source = { 0, 0, (float)vehicle_texture.width, (float)vehicle_texture.height };
	App->renderer->Draw(vehicle_texture, x, y, &source, rotation,
		(int)(vehicle_texture.width / 2.0f), (int)(vehicle_texture.height / 2.0f));

	// Draw UI
	DrawNitroBar();

	// Debug info
	if (App->physics->debug)
	{
		DrawText(TextFormat("Speed: %.1f", speed), 10, 135, 16, GREEN);
		DrawText(TextFormat("Position: (%d, %d)", x, y), 10, 155, 16, GREEN);

		if (nitro_active) DrawText("*** NITRO ACTIVE ***", 10, 195, 20, SKYBLUE);
		else if (handbrake_active && is_turning) DrawText("*** DRIFT MODE ***", 10, 195, 20, ORANGE);
		else if (handbrake_active) DrawText("BRAKING", 10, 195, 16, RED);
	}

	return UPDATE_CONTINUE;
}

void ModulePlayer::OnCollision(PhysBody* bodyA, PhysBody* bodyB)
{
	// Detect strong collisions for sound
	if (bodyA == vehicle && vehicle != nullptr && vehicle->body != nullptr)
	{
		float impact_speed = vehicle->body->GetLinearVelocity().Length();

		// Minimum threshold to play crash sound
		if (impact_speed > 2.0f)
		{
			float volume = impact_speed / 8.0f;
			if (volume > 1.0f) volume = 1.0f;
			if (volume < 0.2f) volume = 0.2f;

			App->audio->SetFxVolume(sfx_crash, volume);
			App->audio->SetFxPitch(sfx_crash, 0.8f + ((rand() % 40) / 100.0f));
			App->audio->PlayFx(sfx_crash);
		}
	}
}