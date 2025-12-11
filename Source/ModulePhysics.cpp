#include "Globals.h"
#include "Application.h"
#include "ModulePhysics.h"
#include "ModuleRender.h"
#include "ModuleWindow.h"

#include "raylib.h"
#include "raymath.h"

#include <cmath>

ModulePhysics::ModulePhysics(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	world = nullptr;
	mouse_joint = nullptr;
	debug = false;
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
	world->Step(1.0f / 60.0f, 6, 2);

	for (b2Contact* c = world->GetContactList(); c; c = c->GetNext())
	{
		if (c->GetFixtureA()->IsSensor() && c->IsTouching())
		{
			PhysBody* pb1 = (PhysBody*)c->GetFixtureA()->GetBody()->GetUserData().pointer;
			PhysBody* pb2 = (PhysBody*)c->GetFixtureB()->GetBody()->GetUserData().pointer;
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

	if (!debug)
	{
		return UPDATE_CONTINUE;
	}

	int shapes_drawn = 0;

	for (size_t i = 0; i < bodies.size(); ++i)
	{
		PhysBody* body = bodies[i];
		int x, y;
		body->GetPosition(x, y);

		if (body->body->GetType() == b2_dynamicBody)
		{
			b2Fixture* fixture = body->body->GetFixtureList();
			while (fixture != nullptr)
			{
				switch (fixture->GetShape()->GetType())
				{
				case b2Shape::e_circle:
				{
					b2CircleShape* shape = (b2CircleShape*)fixture->GetShape();
					DrawCircleLines(x, y, METERS_TO_PIXELS(shape->m_radius), RED);
					shapes_drawn++;
				}
				break;

				case b2Shape::e_polygon:
				{
					b2PolygonShape* shape = (b2PolygonShape*)fixture->GetShape();
					int count = shape->m_count;
					b2Vec2 prev = body->body->GetWorldPoint(shape->m_vertices[count - 1]);
					for (int j = 0; j < count; ++j)
					{
						b2Vec2 v = body->body->GetWorldPoint(shape->m_vertices[j]);
						DrawLine(METERS_TO_PIXELS(prev.x), METERS_TO_PIXELS(prev.y),
							METERS_TO_PIXELS(v.x), METERS_TO_PIXELS(v.y), RED);
						prev = v;
					}
					shapes_drawn++;
				}
				break;

				case b2Shape::e_chain:
				{
					b2ChainShape* shape = (b2ChainShape*)fixture->GetShape();
					int count = shape->m_count;
					b2Vec2 prev = body->body->GetWorldPoint(shape->m_vertices[0]);
					for (int j = 1; j < count; ++j)
					{
						b2Vec2 v = body->body->GetWorldPoint(shape->m_vertices[j]);
						DrawLine(METERS_TO_PIXELS(prev.x), METERS_TO_PIXELS(prev.y),
							METERS_TO_PIXELS(v.x), METERS_TO_PIXELS(v.y), GREEN);
						prev = v;
					}
					shapes_drawn++;
				}
				break;

				default:
					break;
				}
				fixture = fixture->GetNext();
			}
		}
	}

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
	{
		Vector2 mouse_pos = GetMousePosition();
		b2Vec2 p = { PIXELS_TO_METERS(mouse_pos.x), PIXELS_TO_METERS(mouse_pos.y) };

		LOG("ModulePhysics: Click del ratoli a (%.1f %.1f)", mouse_pos.x, mouse_pos.y);

		if (mouse_joint == nullptr)
		{
			bool body_found = false;
			for (size_t i = 0; i < bodies.size(); ++i)
			{
				PhysBody* body = bodies[i];
				if (body->Contains((int)mouse_pos.x, (int)mouse_pos.y))
				{
					LOG("ModulePhysics: Cos fisic seleccionat! Creant mouse joint");

					b2MouseJointDef def;
					def.bodyA = ground;
					def.bodyB = body->body;
					def.target = p;
					def.damping = 0.5f;
					def.stiffness = 20.0f;
					def.maxForce = 100.0f * body->body->GetMass();

					mouse_joint = (b2MouseJoint*)world->CreateJoint(&def);
					body_found = true;
					break;
				}
			}

			if (!body_found)
			{
				LOG("ModulePhysics: Cap cos fisic trobat en aquesta posicio");
			}
		}
	}

	if (mouse_joint && IsMouseButtonDown(MOUSE_LEFT_BUTTON))
	{
		Vector2 mouse_pos = GetMousePosition();
		b2Vec2 mouse_position = { PIXELS_TO_METERS(mouse_pos.x), PIXELS_TO_METERS(mouse_pos.y) };
		mouse_joint->SetTarget(mouse_position);

		b2Vec2 anchor = mouse_joint->GetAnchorB();
		DrawLine(METERS_TO_PIXELS(anchor.x), METERS_TO_PIXELS(anchor.y),
			(int)mouse_pos.x, (int)mouse_pos.y, RED);
	}

	if (mouse_joint && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
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
	LOG("ModulePhysics: Nombre de cossos a eliminar: %d", (int)bodies.size());

	for (size_t i = 0; i < bodies.size(); ++i)
	{
		delete bodies[i];
	}
	bodies.clear();

	delete world;

	LOG("ModulePhysics: Mon de fisica destruida correctament");
	return true;
}

void ModulePhysics::BeginContact(b2Contact* contact)
{
	PhysBody* physA = (PhysBody*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
	PhysBody* physB = (PhysBody*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;

	if (physA && physA->listener != nullptr)
	{
		physA->listener->OnCollision(physA, physB);
	}

	if (physB && physB->listener != nullptr)
	{
		physB->listener->OnCollision(physB, physA);
	}
}

PhysBody* ModulePhysics::CreateCircle(int x, int y, int radius, PhysBodyType type)
{
	b2BodyDef body_def;
	body_def.position.Set(PIXELS_TO_METERS(x), PIXELS_TO_METERS(y));

	switch (type)
	{
	case PhysBodyType::DYNAMIC:
		body_def.type = b2_dynamicBody;
		break;
	case PhysBodyType::STATIC:
		body_def.type = b2_staticBody;
		break;
	case PhysBodyType::KINEMATIC:
		body_def.type = b2_kinematicBody;
		break;
	}

	b2Body* body = world->CreateBody(&body_def);

	b2CircleShape shape;
	shape.m_radius = PIXELS_TO_METERS(radius);

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
	body_def.position.Set(PIXELS_TO_METERS(x), PIXELS_TO_METERS(y));

	switch (type)
	{
	case PhysBodyType::DYNAMIC:
		body_def.type = b2_dynamicBody;
		break;
	case PhysBodyType::STATIC:
		body_def.type = b2_staticBody;
		break;
	case PhysBodyType::KINEMATIC:
		body_def.type = b2_kinematicBody;
		break;
	}

	b2Body* body = world->CreateBody(&body_def);

	b2PolygonShape box;
	box.SetAsBox(PIXELS_TO_METERS(width) * 0.5f, PIXELS_TO_METERS(height) * 0.5f);

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
	body_def.position.Set(PIXELS_TO_METERS(x), PIXELS_TO_METERS(y));
	body_def.type = b2_staticBody;

	b2Body* body = world->CreateBody(&body_def);

	b2PolygonShape box;
	box.SetAsBox(PIXELS_TO_METERS(width) * 0.5f, PIXELS_TO_METERS(height) * 0.5f);

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
	b2BodyDef body_def;
	body_def.position.Set(PIXELS_TO_METERS(x), PIXELS_TO_METERS(y));

	switch (type)
	{
	case PhysBodyType::DYNAMIC:
		body_def.type = b2_dynamicBody;
		break;
	case PhysBodyType::STATIC:
		body_def.type = b2_staticBody;
		break;
	case PhysBodyType::KINEMATIC:
		body_def.type = b2_kinematicBody;
		break;
	}

	b2Body* body = world->CreateBody(&body_def);

	b2ChainShape shape;
	b2Vec2* p = new b2Vec2[size / 2];

	for (int i = 0; i < size / 2; ++i)
	{
		p[i].x = PIXELS_TO_METERS(points[i * 2 + 0]);
		p[i].y = PIXELS_TO_METERS(points[i * 2 + 1]);
	}

	shape.CreateLoop(p, size / 2);

	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;

	body->CreateFixture(&fixture_def);

	delete[] p;

	PhysBody* pbody = new PhysBody();
	pbody->body = body;
	body->GetUserData().pointer = reinterpret_cast<uintptr_t>(pbody);
	pbody->width = 0;
	pbody->height = 0;

	bodies.push_back(pbody);

	LOG("ModulePhysics: Cadena creada a (%d %d) amb %d punts", x, y, size / 2);

	return pbody;
}

PhysBody::PhysBody() : listener(nullptr), body(nullptr)
{
}

PhysBody::~PhysBody()
{
}

void PhysBody::GetPosition(int& x, int& y) const
{
	b2Vec2 pos = body->GetPosition();
	x = METERS_TO_PIXELS(pos.x);
	y = METERS_TO_PIXELS(pos.y);
}

float PhysBody::GetRotation() const
{
	return body->GetAngle() * RAD_TO_DEG;
}

bool PhysBody::Contains(int x, int y) const
{
	b2Vec2 p(PIXELS_TO_METERS(x), PIXELS_TO_METERS(y));

	const b2Fixture* fixture = body->GetFixtureList();

	while (fixture != nullptr)
	{
		if (fixture->GetShape()->TestPoint(body->GetTransform(), p) == true)
		{
			return true;
		}
		fixture = fixture->GetNext();
	}

	return false;
}

int PhysBody::RayCast(int x1, int y1, int x2, int y2, float& normal_x, float& normal_y) const
{
	int ret = -1;

	b2RayCastInput input;
	b2RayCastOutput output;

	input.p1.Set(PIXELS_TO_METERS(x1), PIXELS_TO_METERS(y1));
	input.p2.Set(PIXELS_TO_METERS(x2), PIXELS_TO_METERS(y2));
	input.maxFraction = 1.0f;

	const b2Fixture* fixture = body->GetFixtureList();

	while (fixture != nullptr)
	{
		if (fixture->GetShape()->RayCast(&output, input, body->GetTransform(), 0) == true)
		{
			float fx = (float)(x2 - x1);
			float fy = (float)(y2 - y1);
			float dist = sqrtf((fx * fx) + (fy * fy));

			normal_x = output.normal.x;
			normal_y = output.normal.y;

			return (int)(output.fraction * dist);
		}
		fixture = fixture->GetNext();
	}

	return ret;
}