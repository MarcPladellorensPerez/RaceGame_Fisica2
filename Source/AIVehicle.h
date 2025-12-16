#pragma once
#include "Globals.h"
#include "p2Point.h"
#include "raylib.h"
#include <vector>

#pragma warning(push)
#pragma warning(disable : 26495)
#include "box2d/box2d.h"
#pragma warning(pop)

// Forward declaration
struct Waypoint;

class AIVehicle {
public:
    AIVehicle();
    ~AIVehicle();

    void Init(b2World* world, b2Vec2 position, Texture2D texture, int start_waypoint_id);
    void Update(float dt, const std::vector<Waypoint>& waypoints);
    void Draw(bool debug);

    b2Body* body;
    bool active;
    int current_waypoint_id;

private:
    void RaycastSensors();

    Texture2D texture;
    float width, height;

    // Sensors
    float sensor_length;
    bool wall_detected_center;
    bool wall_detected_left;
    bool wall_detected_right;

	// Identify other cars
    bool is_car_center;
    bool is_car_left;
    bool is_car_right;

    float dist_fraction_center;

	// Logic for short maneuvers
    bool is_maneuvering;
    float maneuver_timer;
    float turn_direction;

	// Navigation
    float waypoint_timer;
    b2Vec2 waypoint_offset;
    b2Vec2 currentTarget;
};