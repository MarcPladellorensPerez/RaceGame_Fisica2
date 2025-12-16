#include "AIVehicle.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModulePhysics.h" 
#include <cmath>
#include <algorithm>

class RayCastCallback : public b2RayCastCallback {
public:
    bool hit;
    float fraction;
    bool isDynamic;

    RayCastCallback() : hit(false), fraction(1.0f), isDynamic(false) {}

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
        if (fixture->IsSensor()) return -1.0f;

        this->hit = true;
        this->fraction = fraction;
        if (fixture->GetBody()->GetType() == b2_dynamicBody) {
            this->isDynamic = true;
        }
        return fraction;
    }
};

AIVehicle::AIVehicle() : body(nullptr), active(false), current_waypoint_id(-1),
is_maneuvering(false), maneuver_timer(0), turn_direction(0),
width(0), height(0), sensor_length(10.0f),
wall_detected_center(false), wall_detected_left(false), wall_detected_right(false),
is_car_center(false), is_car_left(false), is_car_right(false),
dist_fraction_center(1.0f), waypoint_timer(0.0f),
waypoint_offset(0, 0), currentTarget(0, 0), texture({ 0 }) {
}

AIVehicle::~AIVehicle() {}

void AIVehicle::Init(b2World* world, b2Vec2 position, Texture2D tex, int start_waypoint_id) {
    texture = tex;
    width = (float)texture.width;
    height = (float)texture.height;
    current_waypoint_id = start_waypoint_id;
    active = true;
    is_maneuvering = false;
    waypoint_timer = 0.0f;

    float rx = ((rand() % 100) / 30.0f) - 1.5f;
    float ry = ((rand() % 100) / 30.0f) - 1.5f;
    waypoint_offset.Set(rx, ry);

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = position;
    bodyDef.linearDamping = 2.5f;
    bodyDef.angularDamping = 5.0f;

    body = world->CreateBody(&bodyDef);

    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(PIXELS_TO_METERS(width) * 0.45f, PIXELS_TO_METERS(height) * 0.45f);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &dynamicBox;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.3f;
    fixtureDef.restitution = 0.1f;

    body->CreateFixture(&fixtureDef);
}

void AIVehicle::RaycastSensors() {
    if (!body) return;

    b2World* world = body->GetWorld();
    b2Vec2 pos = body->GetPosition();
    b2Vec2 forward = body->GetWorldVector(b2Vec2(0.0f, -1.0f));
    b2Vec2 right = body->GetWorldVector(b2Vec2(1.0f, 0.0f));

    b2Vec2 p1 = pos + 1.2f * forward;

    b2Vec2 p2_center = p1 + sensor_length * forward;
    b2Vec2 p2_left = p1 + (sensor_length * 0.7f) * (forward - 0.5f * right);
    b2Vec2 p2_right = p1 + (sensor_length * 0.7f) * (forward + 0.5f * right);

    RayCastCallback cb;

    cb.hit = false; cb.fraction = 1.0f; cb.isDynamic = false;
    world->RayCast(&cb, p1, p2_center);
    wall_detected_center = cb.hit;
    is_car_center = cb.isDynamic;
    dist_fraction_center = cb.fraction;

    cb.hit = false; cb.isDynamic = false;
    world->RayCast(&cb, p1, p2_left);
    wall_detected_left = cb.hit;
    is_car_left = cb.isDynamic;

    cb.hit = false; cb.isDynamic = false;
    world->RayCast(&cb, p1, p2_right);
    wall_detected_right = cb.hit;
    is_car_right = cb.isDynamic;
}

void AIVehicle::Update(float dt, const std::vector<Waypoint>& waypoints) {
    if (!active || !body) return;

    RaycastSensors();

	// Maneuvering jam logic
    if (is_maneuvering) {
        maneuver_timer += dt;

        b2Vec2 backward = body->GetWorldVector(b2Vec2(0.0f, 1.0f));

        body->ApplyForceToCenter(2.0f * backward, true);

        body->SetAngularVelocity(0.5f * turn_direction);

        if (maneuver_timer > 0.8f) {
            is_maneuvering = false;
            body->SetLinearVelocity(b2Vec2(0, 0));
        }
        return;
    }

    float speed = body->GetLinearVelocity().Length();

    bool hittingWall = wall_detected_center && !is_car_center;
    bool reallyClose = dist_fraction_center < 0.1f;

    if (hittingWall && reallyClose && speed < 0.2f) {
        is_maneuvering = true;
        maneuver_timer = 0.0f;
        if (wall_detected_left) turn_direction = -1.0f;
        else if (wall_detected_right) turn_direction = 1.0f;
        else turn_direction = (rand() % 2 == 0) ? 1.0f : -1.0f;
    }

    b2Vec2 targetPos(0, 0);
    bool found = false;
    for (const auto& wp : waypoints) {
        if (wp.id == current_waypoint_id) {
            targetPos = wp.position + waypoint_offset;
            currentTarget = targetPos;
            found = true;
            break;
        }
    }

    if (!found) return;

    waypoint_timer += dt;
    float distToTarget = (targetPos - body->GetPosition()).Length();
    bool reached = distToTarget < 8.0f;
    bool stuckOnRoute = waypoint_timer > 8.0f;

    if (reached || stuckOnRoute) {
        for (const auto& wp : waypoints) {
            if (wp.id == current_waypoint_id) {
                if (!wp.next_ids.empty()) {
                    current_waypoint_id = wp.next_ids[rand() % wp.next_ids.size()];
                    waypoint_timer = 0.0f;
                    float rx = ((rand() % 100) / 30.0f) - 1.5f;
                    float ry = ((rand() % 100) / 30.0f) - 1.5f;
                    waypoint_offset.Set(rx, ry);
                }
                else {
                    current_waypoint_id = 0;
                }
                break;
            }
        }
    }

    b2Vec2 desiredDir = targetPos - body->GetPosition();
    desiredDir.Normalize();

    b2Vec2 avoidDir(0.0f, 0.0f);
    float avoidFactor = 0.0f;

    if (wall_detected_center) {
        if (!is_car_center) {
            avoidFactor += 4.0f * (1.0f - dist_fraction_center);
            b2Vec2 right = body->GetWorldVector(b2Vec2(1.0f, 0.0f));
            if (b2Dot(desiredDir, right) > 0) avoidDir += body->GetWorldVector(b2Vec2(1.0f, 0.5f));
            else avoidDir += body->GetWorldVector(b2Vec2(-1.0f, 0.5f));
        }
        else {
            avoidFactor += 0.5f;
            avoidDir += body->GetWorldVector(b2Vec2(1.0f, 0.0f));
        }
    }

    if (wall_detected_left && !is_car_left) {
        avoidFactor += 2.0f;
        avoidDir += body->GetWorldVector(b2Vec2(0.8f, 0.2f));
    }
    if (wall_detected_right && !is_car_right) {
        avoidFactor += 2.0f;
        avoidDir += body->GetWorldVector(b2Vec2(-0.8f, 0.2f));
    }

    b2Vec2 finalDir = desiredDir;
    if (avoidFactor > 0.0f) {
        avoidDir.Normalize();
        finalDir = desiredDir + (avoidFactor * avoidDir);
    }
    finalDir.Normalize();

    float desiredAngle = atan2f(finalDir.y, finalDir.x) + (b2_pi / 2.0f);
    float currentAngle = body->GetAngle();
    float angleDiff = desiredAngle - currentAngle;

    while (angleDiff <= -b2_pi) angleDiff += 2 * b2_pi;
    while (angleDiff > b2_pi) angleDiff -= 2 * b2_pi;

    body->SetAngularVelocity(angleDiff * 2.0f);

    float maxSpeed = 5.0f;
    float acceleration = 5.0f;

    if (fabs(angleDiff) > 0.6f) acceleration *= 0.5f;
    else if (wall_detected_center && !is_car_center && dist_fraction_center < 0.4f) {
        acceleration *= 0.1f;
        maxSpeed = 2.0f;
    }
    else if (wall_detected_center && is_car_center) acceleration *= 0.9f;

    if (speed < maxSpeed) {
        b2Vec2 forward = body->GetWorldVector(b2Vec2(0.0f, -1.0f));
        b2Vec2 force = forward;
        force.x *= acceleration;
        force.y *= acceleration;
        body->ApplyForceToCenter(force, true);
    }

    b2Vec2 rightNormal = body->GetWorldVector(b2Vec2(1.0f, 0.0f));
    b2Vec2 vel = body->GetLinearVelocity();
    float lateralSpeed = b2Dot(rightNormal, vel);
    body->ApplyLinearImpulseToCenter(-2.0f * body->GetMass() * lateralSpeed * rightNormal, true);
}

void AIVehicle::Draw(bool debug) {
    if (!active || !body) return;

    b2Vec2 pos = body->GetPosition();
    float angle = body->GetAngle() * RAD_TO_DEG;

    Rectangle source = { 0, 0, width, height };
    float camX = App->renderer->camera_x;
    float camY = App->renderer->camera_y;

    Rectangle dest = { (float)METERS_TO_PIXELS(pos.x) + camX, (float)METERS_TO_PIXELS(pos.y) + camY, width, height };
    Vector2 origin = { width / 2.0f, height / 2.0f };

    Color color = WHITE;
    if (is_maneuvering) color = RED;
    else if (wall_detected_center && !is_car_center && dist_fraction_center < 0.3f) color = ORANGE;

    DrawTexturePro(texture, source, dest, origin, angle, color);

    if (debug) {
        DrawLine((int)(METERS_TO_PIXELS(pos.x) + camX), (int)(METERS_TO_PIXELS(pos.y) + camY),
            (int)(METERS_TO_PIXELS(currentTarget.x) + camX), (int)(METERS_TO_PIXELS(currentTarget.y) + camY), BLUE);

        DrawCircleLines((int)(METERS_TO_PIXELS(currentTarget.x) + camX), (int)(METERS_TO_PIXELS(currentTarget.y) + camY), 5.0f, BLUE);

        b2Vec2 forward = body->GetWorldVector(b2Vec2(0.0f, -1.0f));
        b2Vec2 p1 = pos + 1.2f * forward;
        b2Vec2 p2 = p1 + sensor_length * forward;
        Color cColor = GREEN;
        if (wall_detected_center) cColor = is_car_center ? YELLOW : RED;

        DrawLine((int)(METERS_TO_PIXELS(p1.x) + camX), (int)(METERS_TO_PIXELS(p1.y) + camY),
            (int)(METERS_TO_PIXELS(p2.x) + camX), (int)(METERS_TO_PIXELS(p2.y) + camY), cColor);
    }
}