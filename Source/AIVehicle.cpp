#include "AIVehicle.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModulePhysics.h" 
#include "Player.h" 
#include <cmath>
#include <algorithm>

// Raycast callback that ignores sensors and the car itself
class RayCastCallback : public b2RayCastCallback {
public:
    bool hit;
    float fraction;
    bool isDynamic;
    b2Body* selfBody;

    RayCastCallback(b2Body* self) : hit(false), fraction(1.0f), isDynamic(false), selfBody(self) {}

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
        // Ignore sensors and our own collider
        if (fixture->IsSensor() || fixture->GetBody() == selfBody) return -1.0f;

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
width(0), height(0), sensor_length(3.0f),
wall_detected_center(false), wall_detected_left(false), wall_detected_right(false),
is_car_center(false), is_car_left(false), is_car_right(false),
dist_fraction_center(1.0f), waypoint_timer(0.0f),
waypoint_offset(0, 0), currentTarget(0, 0), texture({ 0 }), drive_time(0.0f), behavior_mode(0) {
}

AIVehicle::~AIVehicle() {}

void AIVehicle::Init(b2World* world, b2Vec2 position, Texture2D tex, int start_waypoint_id, float rotation_degrees) {
    texture = tex;
    width = (float)texture.width;
    height = (float)texture.height;
    current_waypoint_id = start_waypoint_id;
    active = true;
    is_maneuvering = false;
    waypoint_timer = 0.0f;
    drive_time = 0.0f;

    // Assign random personality
    int rand_behavior = rand() % 100;
    if (rand_behavior < 50) behavior_mode = 0; // Neutral
    else if (rand_behavior < 80) behavior_mode = 1; // Aggressive
    else behavior_mode = 2; // Fearful

    float rx = ((rand() % 100) / 30.0f) - 1.5f;
    float ry = ((rand() % 100) / 30.0f) - 1.5f;
    waypoint_offset.Set(rx, ry);

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = position;
    bodyDef.linearDamping = 0.5f;
    bodyDef.angle = rotation_degrees * DEG_TO_RAD;
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

    b2Vec2 p1 = pos;

    b2Vec2 p2_center = p1 + sensor_length * forward;
    b2Vec2 p2_left = p1 + (sensor_length * 0.8f) * (forward - 0.5f * right);
    b2Vec2 p2_right = p1 + (sensor_length * 0.8f) * (forward + 0.5f * right);

    RayCastCallback cb(body);

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

    if (drive_time < 3.0f) {
        if (is_car_center) wall_detected_center = false;
        if (is_car_left) wall_detected_left = false;
        if (is_car_right) wall_detected_right = false;
    }
}

void AIVehicle::Update(float dt, const std::vector<Waypoint>& waypoints) {
    if (!active || !body) return;

    drive_time += dt;
    RaycastSensors();
    float mass = body->GetMass();
    float speed = body->GetLinearVelocity().Length();

    // Maneuver logic (unstuck)
    if (is_maneuvering) {
        maneuver_timer += dt;

        b2Vec2 backward = body->GetWorldVector(b2Vec2(0.0f, 1.0f));
        body->ApplyForceToCenter(4.0f * mass * backward, true);

        if (speed > 0.1f) {
            body->SetAngularVelocity(2.0f * turn_direction);
        }

        if (maneuver_timer > 1.2f) {
            is_maneuvering = false;
            body->SetAngularVelocity(0);
            b2Vec2 forward = body->GetWorldVector(b2Vec2(0.0f, -1.0f));
            body->ApplyLinearImpulseToCenter(1.0f * mass * forward, true);
        }
        return;
    }

    bool hittingWall = wall_detected_center && !is_car_center;
    bool reallyClose = dist_fraction_center < 0.15f;

    if (drive_time > 4.0f && hittingWall && reallyClose && speed < 0.5f) {
        is_maneuvering = true;
        maneuver_timer = 0.0f;
        if (wall_detected_left) turn_direction = -1.0f;
        else if (wall_detected_right) turn_direction = 1.0f;
        else turn_direction = (rand() % 2 == 0) ? 1.0f : -1.0f;
        return;
    }

    // Waypoint search
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
    bool stuckOnRoute = waypoint_timer > 5.0f;

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

    // Player interaction
    b2Vec2 playerInteraction(0.0f, 0.0f);
    float interactionFactor = 0.0f;

    if (App->player && App->player->vehicle && App->player->vehicle->body) {
        b2Vec2 playerPos = App->player->vehicle->body->GetPosition();
        b2Vec2 toPlayer = playerPos - body->GetPosition();
        float distToPlayer = toPlayer.Length();

        // If the player is close (less than 15 meters)
        if (distToPlayer < 15.0f && drive_time > 3.0f) {
            toPlayer.Normalize();

            if (behavior_mode == 1) { // Agressive
                // Try to crash: turn slightly towards the player
                playerInteraction = toPlayer;
                interactionFactor = 0.6f; // Aggression factor
            }
            else if (behavior_mode == 2) { // Fearul
                // Try to avoid: turn away from the player
                playerInteraction = -1.0f * toPlayer;
                interactionFactor = 0.8f; // Panic factor
            }
        }
    }

    b2Vec2 avoidDir(0.0f, 0.0f);
    float avoidFactor = 0.0f;

    if (wall_detected_center) {
        float multiplier = is_car_center ? 2.0f : 5.0f;
        avoidFactor += multiplier * (1.0f - dist_fraction_center);

        b2Vec2 right = body->GetWorldVector(b2Vec2(1.0f, 0.0f));

        if (wall_detected_left && !wall_detected_right) {
            avoidDir += body->GetWorldVector(b2Vec2(1.2f, 0.3f));
        }
        else if (wall_detected_right && !wall_detected_left) {
            avoidDir += body->GetWorldVector(b2Vec2(-1.2f, 0.3f));
        }
        else {
            if (b2Dot(desiredDir, right) > 0) {
                avoidDir += body->GetWorldVector(b2Vec2(1.0f, 0.5f));
            }
            else {
                avoidDir += body->GetWorldVector(b2Vec2(-1.0f, 0.5f));
            }
        }
    }

    if (wall_detected_left && !is_car_left) {
        avoidFactor += 1.5f;
        avoidDir += body->GetWorldVector(b2Vec2(0.8f, 0.2f));
    }
    if (wall_detected_right && !is_car_right) {
        avoidFactor += 1.5f;
        avoidDir += body->GetWorldVector(b2Vec2(-0.8f, 0.2f));
    }

    // Combine: Desired direction + Wall Avoidance + Player Interaction
    b2Vec2 finalDir = desiredDir;

    // Apply player influence
    if (interactionFactor > 0.0f) {
        finalDir = finalDir + (interactionFactor * playerInteraction);
    }

    if (avoidFactor > 0.0f) {
        avoidDir.Normalize();
        finalDir = desiredDir + (avoidFactor * avoidDir);
    }
    finalDir.Normalize();

    // Smooth and realistic turning
    float desiredAngle = atan2f(finalDir.y, finalDir.x) + (b2_pi / 2.0f);
    float currentAngle = body->GetAngle();
    float angleDiff = desiredAngle - currentAngle;

    while (angleDiff <= -b2_pi) angleDiff += 2 * b2_pi;
    while (angleDiff > b2_pi) angleDiff -= 2 * b2_pi;

    float steerFactor = 0.0f;
    if (speed > 0.5f) {
        steerFactor = std::min(speed / 5.0f, 1.0f);
    }

    if (fabs(angleDiff) < 0.05f) {
        body->SetAngularVelocity(0);
    }
    else {
        float turnSpeed = 5.0f;
        float clampedDiff = std::max(-1.0f, std::min(1.0f, angleDiff));
        body->SetAngularVelocity(clampedDiff * turnSpeed * steerFactor);
    }

    // Acceleration
    float maxSpeed = 9.0f;
    float acceleration = 6.0f;

    // Aggressive ones run a bit faster when chasing you
    if (behavior_mode == 1 && interactionFactor > 0.0f) maxSpeed = 10.5f;

    if (fabs(angleDiff) > 0.8f) acceleration *= 0.5f;
    else if (wall_detected_center && !is_car_center && dist_fraction_center < 0.5f) {
        acceleration *= 0.4f;
        maxSpeed = 6.0f;
    }

    if (speed < maxSpeed) {
        b2Vec2 forward = body->GetWorldVector(b2Vec2(0.0f, -1.0f));
        b2Vec2 force = forward;
        force.x *= acceleration * mass;
        force.y *= acceleration * mass;
        body->ApplyForceToCenter(force, true);
    }

    b2Vec2 rightNormal = body->GetWorldVector(b2Vec2(1.0f, 0.0f));
    b2Vec2 vel = body->GetLinearVelocity();
    float lateralSpeed = b2Dot(rightNormal, vel);
    body->ApplyLinearImpulseToCenter(-1.0f * mass * lateralSpeed * rightNormal, true);
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
    if (debug) {
        // Colors according to state and behavior for debugging
        if (is_maneuvering) color = RED;
        else if (behavior_mode == 1) color = { 255, 100, 100, 255 }; // Reddish (Aggressive)
        else if (behavior_mode == 2) color = { 100, 100, 255, 255 }; // Bluish (Fearful)
        else if (wall_detected_center && !is_car_center && dist_fraction_center < 0.3f) color = ORANGE;
    }

    DrawTexturePro(texture, source, dest, origin, angle, color);

    if (debug) {
        DrawLine((int)(METERS_TO_PIXELS(pos.x) + camX), (int)(METERS_TO_PIXELS(pos.y) + camY),
            (int)(METERS_TO_PIXELS(currentTarget.x) + camX), (int)(METERS_TO_PIXELS(currentTarget.y) + camY), BLUE);

        DrawCircleLines((int)(METERS_TO_PIXELS(currentTarget.x) + camX), (int)(METERS_TO_PIXELS(currentTarget.y) + camY), 5.0f, BLUE);

        b2Vec2 forward = body->GetWorldVector(b2Vec2(0.0f, -1.0f));
        b2Vec2 right = body->GetWorldVector(b2Vec2(1.0f, 0.0f));

        b2Vec2 p1 = pos;
        b2Vec2 p2_center = p1 + sensor_length * forward;
        b2Vec2 p2_left = p1 + (sensor_length * 0.8f) * (forward - 0.5f * right);
        b2Vec2 p2_right = p1 + (sensor_length * 0.8f) * (forward + 0.5f * right);

        Color cColor = GREEN;
        if (wall_detected_center) cColor = is_car_center ? YELLOW : RED;
        DrawLine((int)(METERS_TO_PIXELS(p1.x) + camX), (int)(METERS_TO_PIXELS(p1.y) + camY),
            (int)(METERS_TO_PIXELS(p2_center.x) + camX), (int)(METERS_TO_PIXELS(p2_center.y) + camY), cColor);

        DrawLine((int)(METERS_TO_PIXELS(p1.x) + camX), (int)(METERS_TO_PIXELS(p1.y) + camY),
            (int)(METERS_TO_PIXELS(p2_left.x) + camX), (int)(METERS_TO_PIXELS(p2_left.y) + camY), wall_detected_left ? RED : GREEN);

        DrawLine((int)(METERS_TO_PIXELS(p1.x) + camX), (int)(METERS_TO_PIXELS(p1.y) + camY),
            (int)(METERS_TO_PIXELS(p2_right.x) + camX), (int)(METERS_TO_PIXELS(p2_right.y) + camY), wall_detected_right ? RED : GREEN);

        // Debug text over the car
        const char* behaviorText = "N";
        if (behavior_mode == 1) behaviorText = "AGR";
        if (behavior_mode == 2) behaviorText = "FEAR";
        DrawText(behaviorText, (int)(METERS_TO_PIXELS(pos.x) + camX), (int)(METERS_TO_PIXELS(pos.y) + camY), 10, WHITE);
    }
}