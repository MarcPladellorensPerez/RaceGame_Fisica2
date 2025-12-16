#include "Globals.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "ModuleRender.h"
#include "ModuleWindow.h"

#include "raylib.h"
#include "raymath.h"

#include <cmath>

PhysBody::PhysBody() : body(nullptr), listener(nullptr), width(0), height(0) {}

PhysBody::~PhysBody() {}

void PhysBody::GetPosition(int& x, int& y) const
{
	if (body)
	{
		b2Vec2 pos = body->GetPosition();
		x = METERS_TO_PIXELS(pos.x);
		y = METERS_TO_PIXELS(pos.y);
	}
}

float PhysBody::GetRotation() const
{
	if (body)
		return RAD_TO_DEG * body->GetAngle();
	return 0.0f;
}

bool PhysBody::Contains(int x, int y) const
{
	if (body)
	{
		b2Vec2 p(PIXELS_TO_METERS((float)x), PIXELS_TO_METERS((float)y));
		const b2Fixture* fixture = body->GetFixtureList();
		while (fixture != nullptr)
		{
			if (fixture->GetShape()->TestPoint(body->GetTransform(), p) == true)
				return true;
			fixture = fixture->GetNext();
		}
	}
	return false;
}

int PhysBody::RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const
{
	return -1;
}

ModulePhysics::ModulePhysics(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	world = nullptr;
	mouse_joint = nullptr;
	mouse_body = nullptr;
	ground = nullptr;
	debug = false;

	accumulator = 0.0f;

	LOG("ModulePhysics: Constructor cridat");
}

ModulePhysics::~ModulePhysics()
{
	LOG("ModulePhysics: Destructor cridat");
}

bool ModulePhysics::Start()
{
	LOG("ModulePhysics: Creant mon de fisica 2D");

	b2Vec2 gravity(GRAVITY_X, GRAVITY_Y);
	world = new b2World(gravity);
	world->SetContactListener(this);

	b2BodyDef bd;
	ground = world->CreateBody(&bd);

	LOG("ModulePhysics: Mon de fisica creat correctament");
	LOG("ModulePhysics: Gravetat configurada a X=%.2f Y=%.2f", GRAVITY_X, GRAVITY_Y);
	LOG("ModulePhysics: Prem F1 per activar mode debug");

	return true;
}

update_status ModulePhysics::PreUpdate()
{
	// Make sure cars speed it's the same for all computers
	float frameTime = GetFrameTime();

	// Avoid big jumps if freezed
	if (frameTime > 0.25f) frameTime = 0.25f;

	accumulator += frameTime;

	const float FIXED_STEP = 1.0f / 60.0f;

	while (accumulator >= FIXED_STEP)
	{
		world->Step(FIXED_STEP, 8, 3);
		accumulator -= FIXED_STEP;
	}

	// Process collisions
	for (b2Contact* c = world->GetContactList(); c; c = c->GetNext())
	{
		if (c->GetFixtureA()->IsSensor() && c->IsTouching())
		{
			PhysBody* pb1 = reinterpret_cast<PhysBody*>(c->GetFixtureA()->GetBody()->GetUserData().pointer);
			PhysBody* pb2 = reinterpret_cast<PhysBody*>(c->GetFixtureB()->GetBody()->GetUserData().pointer);

			if (pb1 && pb2 && pb1->listener)
			{
				pb1->listener->OnCollision(pb1, pb2);
			}
		}
	}

	return UPDATE_CONTINUE;
}

update_status ModulePhysics::PostUpdate()
{
	if (IsKeyPressed(KEY_F1))
	{
		debug = !debug;
		if (debug)
		{
			LOG("ModulePhysics: Mode DEBUG ACTIVAT");
		}
		else
		{
			LOG("ModulePhysics: Mode DEBUG DESACTIVAT");
		}
	}

	if (!debug) return UPDATE_CONTINUE;

	int shapes_drawn = 0;
	float cam_x = App->renderer->camera_x;
	float cam_y = App->renderer->camera_y;

	// Draw physics bodies
	for (b2Body* b = world->GetBodyList(); b; b = b->GetNext())
	{
		for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext())
		{
			switch (f->GetType())
			{
			case b2Shape::e_circle:
			{
				b2CircleShape* shape = (b2CircleShape*)f->GetShape();
				b2Vec2 pos = f->GetBody()->GetPosition();
				DrawCircleLines(
					(int)(METERS_TO_PIXELS(pos.x) + cam_x),
					(int)(METERS_TO_PIXELS(pos.y) + cam_y),
					(float)METERS_TO_PIXELS(shape->m_radius),
					RED);
				shapes_drawn++;
			}
			break;

			case b2Shape::e_polygon:
			{
				b2PolygonShape* shape = (b2PolygonShape*)f->GetShape();
				int count = shape->m_count;
				b2Vec2 prev = b->GetWorldPoint(shape->m_vertices[count - 1]);

				for (int j = 0; j < count; ++j)
				{
					b2Vec2 v = b->GetWorldPoint(shape->m_vertices[j]);
					DrawLine(
						(int)(METERS_TO_PIXELS(prev.x) + cam_x),
						(int)(METERS_TO_PIXELS(prev.y) + cam_y),
						(int)(METERS_TO_PIXELS(v.x) + cam_x),
						(int)(METERS_TO_PIXELS(v.y) + cam_y),
						RED);
					prev = v;
				}
				shapes_drawn++;
			}
			break;

			case b2Shape::e_chain:
			{
				b2ChainShape* shape = (b2ChainShape*)f->GetShape();
				int count = shape->m_count;

				for (int j = 0; j < count - 1; ++j)
				{
					b2Vec2 v1 = b->GetWorldPoint(shape->m_vertices[j]);
					b2Vec2 v2 = b->GetWorldPoint(shape->m_vertices[j + 1]);

					DrawLineEx(
						Vector2{ (float)(METERS_TO_PIXELS(v1.x) + cam_x), (float)(METERS_TO_PIXELS(v1.y) + cam_y) },
						Vector2{ (float)(METERS_TO_PIXELS(v2.x) + cam_x), (float)(METERS_TO_PIXELS(v2.y) + cam_y) },
						3.0f,
						GREEN);
				}

				// Close the loop if necessary
				if (count > 0)
				{
					b2Vec2 v_last = b->GetWorldPoint(shape->m_vertices[count - 1]);
					b2Vec2 v_first = b->GetWorldPoint(shape->m_vertices[0]);

					DrawLineEx(
						Vector2{ (float)(METERS_TO_PIXELS(v_last.x) + cam_x), (float)(METERS_TO_PIXELS(v_last.y) + cam_y) },
						Vector2{ (float)(METERS_TO_PIXELS(v_first.x) + cam_x), (float)(METERS_TO_PIXELS(v_first.y) + cam_y) },
						3.0f,
						GREEN);
				}
				shapes_drawn++;
			}
			break;

			default: break;
			}
		}
	}

	// Mouse joint (debug)
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
	{
		Vector2 mouse_pos = GetMousePosition();
		float world_x = mouse_pos.x - cam_x;
		float world_y = mouse_pos.y - cam_y;
		b2Vec2 p = b2Vec2(PIXELS_TO_METERS(world_x), PIXELS_TO_METERS(world_y));

		if (mouse_joint == nullptr)
		{
			for (b2Body* b = world->GetBodyList(); b; b = b->GetNext())
			{
				if (b->GetType() == b2_dynamicBody)
				{
					for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext())
					{
						if (f->TestPoint(p))
						{
							LOG("ModulePhysics: Cos fisic seleccionat! Creant mouse joint");
							b2MouseJointDef def;
							def.bodyA = ground;
							def.bodyB = b;
							def.target = p;
							def.damping = 0.5f;
							def.stiffness = 20.0f;
							def.maxForce = 1000.0f * b->GetMass();

							mouse_joint = (b2MouseJoint*)world->CreateJoint(&def);
							break;
						}
					}
				}
				if (mouse_joint) break;
			}
		}
	}

	if (mouse_joint && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
	{
		Vector2 mouse_pos = GetMousePosition();
		float world_x = mouse_pos.x - cam_x;
		float world_y = mouse_pos.y - cam_y;
		b2Vec2 mouse_position = b2Vec2(PIXELS_TO_METERS(world_x), PIXELS_TO_METERS(world_y));

		mouse_joint->SetTarget(mouse_position);

		b2Vec2 anchor = mouse_joint->GetAnchorB();
		DrawLine(
			(int)(METERS_TO_PIXELS(anchor.x) + cam_x),
			(int)(METERS_TO_PIXELS(anchor.y) + cam_y),
			(int)mouse_pos.x, (int)mouse_pos.y,
			RED);
	}
	else if (mouse_joint && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
	{
		LOG("ModulePhysics: Mouse joint eliminat");
		world->DestroyJoint(mouse_joint);
		mouse_joint = nullptr;
	}

	return UPDATE_CONTINUE;
}

bool ModulePhysics::CleanUp()
{
	LOG("ModulePhysics: Destruint mon de fisica");
	for (size_t i = 0; i < bodies.size(); ++i)
	{
		delete bodies[i];
	}
	bodies.clear();

	if (world)
	{
		delete world;
		world = nullptr;
	}
	LOG("ModulePhysics: Mon de fisica destruida correctament");
	return true;
}

void ModulePhysics::BeginContact(b2Contact* contact)
{
	PhysBody* physA = reinterpret_cast<PhysBody*>(contact->GetFixtureA()->GetBody()->GetUserData().pointer);
	PhysBody* physB = reinterpret_cast<PhysBody*>(contact->GetFixtureB()->GetBody()->GetUserData().pointer);

	if (physA && physA->listener != nullptr)
		physA->listener->OnCollision(physA, physB);

	if (physB && physB->listener != nullptr)
		physB->listener->OnCollision(physB, physA);
}

PhysBody* ModulePhysics::CreateCircle(int x, int y, int radius, PhysBodyType type)
{
	b2BodyDef body_def;
	body_def.position.Set(PIXELS_TO_METERS((float)x), PIXELS_TO_METERS((float)y));

	switch (type)
	{
	case PhysBodyType::DYNAMIC: body_def.type = b2_dynamicBody; break;
	case PhysBodyType::STATIC: body_def.type = b2_staticBody; break;
	case PhysBodyType::KINEMATIC: body_def.type = b2_kinematicBody; break;
	}

	b2Body* body = world->CreateBody(&body_def);

	b2CircleShape shape;
	shape.m_radius = PIXELS_TO_METERS((float)radius);

	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;
	fixture_def.density = 1.0f;

	body->CreateFixture(&fixture_def);

	PhysBody* pbody = new PhysBody();
	pbody->body = body;
	body->GetUserData().pointer = reinterpret_cast<uintptr_t>(pbody);
	pbody->width = radius * 2;
	pbody->height = radius * 2;

	bodies.push_back(pbody);
	LOG("ModulePhysics: Cercle creat a (%d %d) amb radi %d", x, y, radius);
	return pbody;
}

PhysBody* ModulePhysics::CreateRectangle(int x, int y, int width, int height, PhysBodyType type)
{
	b2BodyDef body_def;
	body_def.position.Set(PIXELS_TO_METERS((float)x), PIXELS_TO_METERS((float)y));

	switch (type)
	{
	case PhysBodyType::DYNAMIC: body_def.type = b2_dynamicBody; break;
	case PhysBodyType::STATIC: body_def.type = b2_staticBody; break;
	case PhysBodyType::KINEMATIC: body_def.type = b2_kinematicBody; break;
	}

	b2Body* body = world->CreateBody(&body_def);

	b2PolygonShape box;
	box.SetAsBox(PIXELS_TO_METERS((float)width) * 0.5f, PIXELS_TO_METERS((float)height) * 0.5f);

	b2FixtureDef fixture_def;
	fixture_def.shape = &box;
	fixture_def.density = 1.0f;

	body->CreateFixture(&fixture_def);

	PhysBody* pbody = new PhysBody();
	pbody->body = body;
	body->GetUserData().pointer = reinterpret_cast<uintptr_t>(pbody);
	pbody->width = width;
	pbody->height = height;

	bodies.push_back(pbody);
	LOG("ModulePhysics: Rectangle creat a (%d %d) amb dimensions %dx%d", x, y, width, height);
	return pbody;
}

PhysBody* ModulePhysics::CreateRectangleSensor(int x, int y, int width, int height)
{
	b2BodyDef body_def;
	body_def.position.Set(PIXELS_TO_METERS((float)x), PIXELS_TO_METERS((float)y));
	body_def.type = b2_staticBody;

	b2Body* body = world->CreateBody(&body_def);

	b2PolygonShape box;
	box.SetAsBox(PIXELS_TO_METERS((float)width) * 0.5f, PIXELS_TO_METERS((float)height) * 0.5f);

	b2FixtureDef fixture_def;
	fixture_def.shape = &box;
	fixture_def.density = 1.0f;
	fixture_def.isSensor = true;

	body->CreateFixture(&fixture_def);

	PhysBody* pbody = new PhysBody();
	pbody->body = body;
	body->GetUserData().pointer = reinterpret_cast<uintptr_t>(pbody);
	pbody->width = width;
	pbody->height = height;

	bodies.push_back(pbody);
	LOG("ModulePhysics: Sensor rectangle creat a (%d %d) amb dimensions %dx%d", x, y, width, height);
	return pbody;
}

PhysBody* ModulePhysics::CreateChain(int x, int y, const int* points, int size, PhysBodyType type)
{
	if (points == nullptr || size < 6)
	{
		LOG("ERROR: CreateChain cridat amb punts nulls o tamany incorrecte");
		return nullptr;
	}

	b2BodyDef body_def;
	body_def.position.Set(PIXELS_TO_METERS((float)x), PIXELS_TO_METERS((float)y));

	switch (type)
	{
	case PhysBodyType::DYNAMIC: body_def.type = b2_dynamicBody; break;
	case PhysBodyType::STATIC: body_def.type = b2_staticBody; break;
	case PhysBodyType::KINEMATIC: body_def.type = b2_kinematicBody; break;
	}

	b2Body* body = world->CreateBody(&body_def);

	b2ChainShape shape;
	int num_vertices = size / 2;
	b2Vec2* p = new b2Vec2[num_vertices];

	for (int i = 0; i < num_vertices; ++i)
	{
		p[i].x = PIXELS_TO_METERS((float)points[i * 2 + 0]);
		p[i].y = PIXELS_TO_METERS((float)points[i * 2 + 1]);
	}

	shape.CreateLoop(p, num_vertices);

	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;
	fixture_def.friction = 0.5f;
	fixture_def.restitution = 0.0f;

	body->CreateFixture(&fixture_def);

	delete[] p;

	PhysBody* pbody = new PhysBody();
	pbody->body = body;
	body->GetUserData().pointer = reinterpret_cast<uintptr_t>(pbody);

	bodies.push_back(pbody);
	LOG("ModulePhysics: Cadena creada a (%d %d) amb %d punts", x, y, num_vertices);
	return pbody;
}