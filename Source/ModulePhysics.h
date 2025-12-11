#pragma once

#include "Module.h"
#include "Globals.h"

#include "box2d\box2d.h"

// Module --------------------------------------
class ModulePhysics : public Module, public b2ContactListener
{
public:
	ModulePhysics(Application* app, bool start_enabled = true);
	~ModulePhysics();

	bool Start();
	update_status PreUpdate();
	update_status PostUpdate();
	bool CleanUp();

	// Gestió de col·lisions de Box2D
	void BeginContact(b2Contact* contact);

	b2World* world;

private:
	bool debug;
	b2MouseJoint* mouse_joint;
	b2Body* body_clicked;
};