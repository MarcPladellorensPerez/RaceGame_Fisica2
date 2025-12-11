#include "Globals.h"
#include "Application.h"
#include "ModuleWindow.h"
#include "ModuleRender.h"
#include "ModulePhysics.h"
#include <math.h>

ModuleRender::ModuleRender(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	background = RAYWHITE;
	camera_x = 0.0f;
	camera_y = 0.0f;
}

ModuleRender::~ModuleRender()
{
}

bool ModuleRender::Init()
{
	LOG("ModuleRender: Creant context de render");
	bool ret = true;

	return ret;
}

update_status ModuleRender::PreUpdate()
{
	return UPDATE_CONTINUE;
}

update_status ModuleRender::Update()
{
	ClearBackground(background);
	BeginDrawing();
	return UPDATE_CONTINUE;
}

update_status ModuleRender::PostUpdate()
{
	DrawFPS(10, 10);

	if (App->physics->debug)
	{
		DrawText("MODE DEBUG ACTIVAT", 10, 40, 20, RED);
		DrawText("Fes clic i arrossega els objectes!", 10, 65, 16, DARKGRAY);
		DrawText("Prem F1 per desactivar", 10, 85, 16, DARKGRAY);

		DrawText(TextFormat("Camera: (%.0f, %.0f)", camera_x, camera_y), 10, 110, 16, BLUE);
	}
	else
	{
		DrawText("Prem F1 per activar mode DEBUG", 10, 40, 16, DARKGRAY);
	}

	EndDrawing();

	return UPDATE_CONTINUE;
}

bool ModuleRender::CleanUp()
{
	LOG("ModuleRender: Netejant render");
	return true;
}

void ModuleRender::SetBackgroundColor(Color color)
{
	background = color;
}

void ModuleRender::SetCameraPosition(float x, float y)
{
	camera_x = x;
	camera_y = y;
	LOG("ModuleRender: Camera posicionada a (%.2f, %.2f)", camera_x, camera_y);
}

void ModuleRender::CenterCameraOn(float x, float y)
{
	camera_x = (SCREEN_WIDTH / 2.0f) - x;
	camera_y = (SCREEN_HEIGHT / 2.0f) - y;
}

void ModuleRender::UpdateCamera(float target_x, float target_y, float smoothness)
{
	float desired_x = (SCREEN_WIDTH / 2.0f) - target_x;
	float desired_y = (SCREEN_HEIGHT / 2.0f) - target_y;

	camera_x += (desired_x - camera_x) * smoothness;
	camera_y += (desired_y - camera_y) * smoothness;
}

bool ModuleRender::Draw(Texture2D texture, int x, int y, const Rectangle* section, double angle, int pivot_x, int pivot_y) const
{
	bool ret = true;

	float scale = 1.0f;
	Vector2 position = { (float)x, (float)y };
	Rectangle rect = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };

	if (section != NULL)
	{
		rect = *section;
	}

	position.x = (float)(x - pivot_x) * scale + camera_x;
	position.y = (float)(y - pivot_y) * scale + camera_y;

	rect.width *= scale;
	rect.height *= scale;

	DrawTextureRec(texture, rect, position, WHITE);

	return ret;
}