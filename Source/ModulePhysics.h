#pragma once

#include "Module.h"
#include "Globals.h"

#include "box2d/box2d.h"

#include <vector>

#define GRAVITY_X 0.0f
#define GRAVITY_Y 0.0f

#define PIXELS_PER_METER 50.0f
#define METERS_PER_PIXEL 0.02f

#define METERS_TO_PIXELS(m) ((int) floor(PIXELS_PER_METER * m))
#define PIXELS_TO_METERS(p)  ((float) METERS_PER_PIXEL * p)

#define RAD_TO_DEG 57.29577951308232f
#define DEG_TO_RAD 0.01745329251994f

enum class PhysBodyType
{
	STATIC,
	DYNAMIC,
	KINEMATIC
};

class PhysBody
{
public:
	PhysBody();
	~PhysBody();

	void GetPosition(int& x, int& y) const;
	float GetRotation() const;
	bool Contains(int x, int y) const;
	int RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const;

public:
	int width = 0;
	int height = 0;
	b2Body* body = nullptr;
	Module* listener = nullptr;
};

class ModulePhysics : public Module, public b2ContactListener
{
public:
	ModulePhysics(Application* app, bool start_enabled = true);
	~ModulePhysics();

	bool Start();
	update_status PreUpdate();
	update_status PostUpdate();
	bool CleanUp();

	PhysBody* CreateCircle(int x, int y, int radius, PhysBodyType type = PhysBodyType::DYNAMIC);
	PhysBody* CreateRectangle(int x, int y, int width, int height, PhysBodyType type = PhysBodyType::DYNAMIC);
	PhysBody* CreateRectangleSensor(int x, int y, int width, int height);
	PhysBody* CreateChain(int x, int y, const int* points, int size, PhysBodyType type = PhysBodyType::STATIC);

	void BeginContact(b2Contact* contact) override;

	b2World* GetWorld() const { return world; }

	bool debug;

private:
	b2World* world = nullptr;
	b2Body* ground = nullptr;
	b2MouseJoint* mouse_joint = nullptr;
	b2Body* mouse_body = nullptr;

	std::vector<PhysBody*> bodies;
};