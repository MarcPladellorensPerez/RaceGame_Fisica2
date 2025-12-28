#pragma once

#include "Globals.h"
#include "raylib.h"
#include <vector>
#include <string>
#include <algorithm>

#pragma warning(push)
#pragma warning(disable : 26495)
#include "box2d/box2d.h"
#pragma warning(pop)

struct RacerInfo {
    std::string name;
    bool is_player;
    int position;
    b2Body* body;
    int current_waypoint;
    float distance_to_next_waypoint;
    float total_progress; 
};

class Leaderboard {
public:
    Leaderboard();
    ~Leaderboard();

    void Init();
    void CleanUp();

    void UpdatePositions(const std::vector<RacerInfo>& racers);

    // Draw leaderboard on screen
    void Draw();

    void SetVisible(bool visible) { is_visible = visible; }
    bool IsVisible() const { return is_visible; }

private:
    Texture2D background_texture;
    std::vector<RacerInfo> sorted_racers;
    bool is_visible;

    // Position leaderboard on screen
    int board_x;
    int board_y;
    int board_width;
    int board_height;

    float CalculateTotalProgress(const RacerInfo& racer);
};