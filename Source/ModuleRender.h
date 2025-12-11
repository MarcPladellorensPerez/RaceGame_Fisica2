#pragma once
#include "Module.h"
#include "Globals.h"

#include "raylib.h"

#include <limits.h>

class ModuleRender : public Module
{
public:
	ModuleRender(Application* app, bool start_enabled = true);
	~ModuleRender();

	bool Init();
	update_status PreUpdate();
	update_status Update();
	update_status PostUpdate();
	bool CleanUp();

	void SetBackgroundColor(Color color);
	bool Draw(Texture2D texture, int x, int y, const Rectangle* section = NULL, double angle = 0, int pivot_x = 0, int pivot_y = 0) const;

	void SetCameraPosition(float x, float y);
	void CenterCameraOn(float x, float y);
	void UpdateCamera(float target_x, float target_y, float smoothness = 0.1f);

public:

	Color background;
	float camera_x;
	float camera_y;
};