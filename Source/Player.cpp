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

	// Vehicle fisics properties
	vehicle->body->SetAngularDamping(8.0f);
	vehicle->body->SetLinearDamping(3.0f);
	vehicle->body->SetFixedRotation(false);

	b2Fixture* fixture = vehicle->body->GetFixtureList();
	if (fixture)
	{
		fixture->SetDensity(1.0f);
		fixture->SetFriction(0.5f);
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
	float acceleration = 50.0f;
	float max_speed = 8.0f;
	float turn_speed = 2.5f;
	float drift_factor = 0.9f;

	// Acceleration control (W/S or up/down arrows)
	if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
	{
		b2Vec2 forward = vehicle->body->GetWorldVector(b2Vec2(0.0f, -1.0f));
		forward.x *= acceleration;
		forward.y *= acceleration;
		vehicle->body->ApplyForceToCenter(forward, true);
	}
	else if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
	{
		b2Vec2 backward = vehicle->body->GetWorldVector(b2Vec2(0.0f, 1.0f));
		backward.x *= acceleration * 0.7f;
		backward.y *= acceleration * 0.7f;
		vehicle->body->ApplyForceToCenter(backward, true);
	}

	// Directrion control (A/D or left/right arrows)
	b2Vec2 velocity = vehicle->body->GetLinearVelocity();
	float speed = velocity.Length();

	// Rotate only if moving
	if (speed > 1.0f)
	{
		if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
		{
			float turn = -turn_speed * (speed / max_speed);
			vehicle->body->SetAngularVelocity(turn);
		}
		else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
		{
			float turn = turn_speed * (speed / max_speed);
			vehicle->body->SetAngularVelocity(turn);
		}
		else
		{
			vehicle->body->SetAngularVelocity(vehicle->body->GetAngularVelocity() * 0.95f);
		}
	}
	else
	{
		vehicle->body->SetAngularVelocity(0);
	}

	// Drifting effect
	b2Vec2 right_vector = vehicle->body->GetWorldVector(b2Vec2(1.0f, 0.0f));
	float lateral_velocity = b2Dot(right_vector, velocity);

	b2Vec2 impulse;
	impulse.x = -lateral_velocity * drift_factor * vehicle->body->GetMass() * right_vector.x;
	impulse.y = -lateral_velocity * drift_factor * vehicle->body->GetMass() * right_vector.y;
	vehicle->body->ApplyLinearImpulseToCenter(impulse, true);

	// Max speed limit
	if (speed > max_speed)
	{
		velocity.Normalize();
		velocity.x *= max_speed;
		velocity.y *= max_speed;
		vehicle->body->SetLinearVelocity(velocity);
	}

	// Hand brake (space bar)
	if (IsKeyDown(KEY_SPACE))
	{
		b2Vec2 brake_force;
		brake_force.x = -velocity.x * vehicle->body->GetMass() * 15.0f;
		brake_force.y = -velocity.y * vehicle->body->GetMass() * 15.0f;
		vehicle->body->ApplyForceToCenter(brake_force, true);
	}

	// Position and rotiation of the vehicle
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
	}

	return UPDATE_CONTINUE;
}