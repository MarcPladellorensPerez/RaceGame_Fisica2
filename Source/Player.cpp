#include "Globals.h"
#include "Application.h"
#include "Player.h"
#include "ModuleRender.h"

#pragma warning(push)
#pragma warning(disable : 26495)
#include "ModulePhysics.h"
#pragma warning(pop)

ModulePlayer::ModulePlayer(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	vehicle = nullptr;
	vehicle_texture = { 0 };
}

ModulePlayer::~ModulePlayer()
{
}

bool ModulePlayer::Start()
{
	LOG("ModulePlayer: Iniciando...");

	vehicle_texture = LoadTexture("Assets/Textures/Cars/car1.png");

	if (vehicle_texture.id == 0)
	{
		LOG("ERROR al cargar la textura del vehiculo");
		return false;
	}

	LOG("ModulePlayer: Textura cargada correctamente (%dx%d)", vehicle_texture.width, vehicle_texture.height);

	// Create vehicle body at origin position
	int start_x = 640;
	int start_y = 360;

	// Vehicle collider
	vehicle = App->physics->CreateRectangle(start_x, start_y,
		vehicle_texture.width,
		vehicle_texture.height,
		PhysBodyType::DYNAMIC);

	if (vehicle == nullptr || vehicle->body == nullptr)
	{
		LOG("ERROR: No se pudo crear el cuerpo fisico del vehiculo!");
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

	LOG("ModulePlayer: Vehiculo inicializado correctamente");

	return true;
}

bool ModulePlayer::CleanUp()
{
	UnloadTexture(vehicle_texture);
	return true;
}

update_status ModulePlayer::Update()
{
	if (vehicle == nullptr || vehicle->body == nullptr)
	{
		return UPDATE_CONTINUE;
	}

	// Movement parameters
	float acceleration = 25.0f;
	float max_speed_forward = 6.0f;
	float max_speed_reverse = 4.5f;
	float base_turn_speed = 3.0f;
	float drift_factor = 0.7f;
	float brake_strength = 8.0f;
	float handbrake_drift_force = 25.0f; 

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

	// Draw vehicle
	Rectangle source = { 0, 0, (float)vehicle_texture.width, (float)vehicle_texture.height };
	App->renderer->Draw(vehicle_texture, x, y, &source, rotation,
		vehicle_texture.width / 2, vehicle_texture.height / 2);

	// Debug info on screen (in game debug mode)
	if (App->physics->debug)
	{
		DrawText(TextFormat("Velocidad: %.1f", speed), 10, 135, 16, GREEN);
		DrawText(TextFormat("Posicion: (%d, %d)", x, y), 10, 155, 16, GREEN);
		DrawText(TextFormat("Rotacion: %.1f", rotation), 10, 175, 16, GREEN);

		if (handbrake_active && is_turning)
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