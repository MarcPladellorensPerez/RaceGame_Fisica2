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
	return true;
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
		DrawText("DEBUG MODE ACTIVATED", 10, 40, 20, RED);
		DrawText("Click to drag objects!", 10, 65, 16, DARKGRAY);
		DrawText("Press F1 to disable Debug Mode", 10, 85, 16, DARKGRAY);
		DrawText(TextFormat("Camera: (%.0f, %.0f)", camera_x, camera_y), 10, 110, 16, BLUE);
	}
	else
	{
		DrawText("Press F1 to activate Debug Mode", 10, 40, 16, DARKGRAY);
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
	if (texture.id == 0)
	{
		return false;
	}

	Rectangle source_rect;
	if (section != NULL)
	{
		source_rect = *section;
	}
	else
	{
		source_rect = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
	}

	// Calculate screen position with camera
	Vector2 screen_pos;
	screen_pos.x = (float)x + camera_x;
	screen_pos.y = (float)y + camera_y;

	// Rotation origin
	Vector2 origin = { (float)pivot_x, (float)pivot_y };

	Rectangle dest_rect;
	dest_rect.x = screen_pos.x;
	dest_rect.y = screen_pos.y;
	dest_rect.width = source_rect.width;
	dest_rect.height = source_rect.height;

	// Rotate texture
	DrawTexturePro(texture, source_rect, dest_rect, origin, (float)angle, WHITE);

	return true;
}