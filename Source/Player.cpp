#include "Globals.h"
#include "Application.h"
#include "Player.h"
#include "ModuleRender.h"

// Ignoramos warnings de Box2D para que no fallen al compilar
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
	vehicle_texture = LoadTexture("Assets/Textures/Cars/car1.png");

	vehicle = App->physics->CreateRectangle(640, 360, vehicle_texture.width, vehicle_texture.height, PhysBodyType::DYNAMIC);

	return true;
}

bool ModulePlayer::CleanUp()
{
	UnloadTexture(vehicle_texture);
	return true;
}

update_status ModulePlayer::Update()
{
	float speed = 2000.0f;
	float turn_speed = 30.0f;

	if (vehicle != nullptr) {
		if (IsKeyDown(KEY_UP))
		{
			b2Vec2 force = vehicle->body->GetWorldVector(b2Vec2(0, -speed));
			vehicle->body->ApplyForceToCenter(force, true);
		}
		if (IsKeyDown(KEY_DOWN))
		{
			b2Vec2 force = vehicle->body->GetWorldVector(b2Vec2(0, speed));
			vehicle->body->ApplyForceToCenter(force, true);
		}
		if (IsKeyDown(KEY_LEFT))
		{
			vehicle->body->SetAngularVelocity(-turn_speed * DEG_TO_RAD);
		}
		else if (IsKeyDown(KEY_RIGHT))
		{
			vehicle->body->SetAngularVelocity(turn_speed * DEG_TO_RAD);
		}
		else
		{
			vehicle->body->SetAngularVelocity(0);
		}

		b2Vec2 velocity = vehicle->body->GetLinearVelocity();
		b2Vec2 forward = vehicle->body->GetWorldVector(b2Vec2(0, 1));
		b2Vec2 right = vehicle->body->GetWorldVector(b2Vec2(1, 0));

		float lateral_velocity = b2Dot(right, velocity);
		b2Vec2 impulse = -lateral_velocity * vehicle->body->GetMass() * right;
		vehicle->body->ApplyLinearImpulse(impulse, vehicle->body->GetWorldCenter(), true);

		vehicle->body->SetLinearDamping(2.0f);
		vehicle->body->SetAngularDamping(2.0f);

		int x, y;
		vehicle->GetPosition(x, y);

		App->renderer->CenterCameraOn(x, y);

		Rectangle source = { 0, 0, (float)vehicle_texture.width, (float)vehicle_texture.height };
		App->renderer->Draw(vehicle_texture, x, y, &source, vehicle->GetRotation(), vehicle_texture.width / 2, vehicle_texture.height / 2);
	}

	return UPDATE_CONTINUE;
}