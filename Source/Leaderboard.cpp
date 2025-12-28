#include "Leaderboard.h"
#include "ModulePhysics.h"
#include <cmath>

Leaderboard::Leaderboard()
    : background_texture({ 0 }), is_visible(true),
    board_x(20), board_y(20), board_width(250), board_height(400) {
}

Leaderboard::~Leaderboard() {
}

void Leaderboard::Init() {
    background_texture = LoadTexture("Assets/Textures/UI/tablero_fondo.png");

    if (background_texture.id == 0) {
        LOG("WARNING: No se pudo cargar tablero_fondo.png, usando fondo por defecto");
    }
    else {
        board_width = background_texture.width;
        board_height = background_texture.height;
    }

    sorted_racers.clear();
}

void Leaderboard::CleanUp() {
    if (background_texture.id != 0) {
        UnloadTexture(background_texture);
        background_texture = { 0 };
    }
    sorted_racers.clear();
}

float Leaderboard::CalculateTotalProgress(const RacerInfo& racer) {

    float progress = (float)racer.current_waypoint * 1000.0f;

    progress -= racer.distance_to_next_waypoint;

    return progress;
}

void Leaderboard::UpdatePositions(const std::vector<RacerInfo>& racers) {
    sorted_racers = racers;

    for (auto& racer : sorted_racers) {
        racer.total_progress = CalculateTotalProgress(racer);
    }
    std::sort(sorted_racers.begin(), sorted_racers.end(),
        [](const RacerInfo& a, const RacerInfo& b) {
            return a.total_progress > b.total_progress;
        });
    for (size_t i = 0; i < sorted_racers.size(); ++i) {
        sorted_racers[i].position = (int)i + 1;
    }
}

void Leaderboard::Draw() {
    if (!is_visible) return;

    if (background_texture.id != 0) {
        DrawTexture(background_texture, board_x, board_y, WHITE);
    }

    DrawLineEx({ (float)board_x + 10, (float)board_y + 50 },
        { (float)board_x + board_width - 10, (float)board_y + 50 },
        2.0f, WHITE);

    int start_y = board_y + 65;
    int line_height = 35;

    for (size_t i = 0; i < sorted_racers.size() && i < 10; ++i) {
        const RacerInfo& racer = sorted_racers[i];
        int y_pos = start_y + (int)i * line_height;

        Color position_color = WHITE;
        Color name_color = WHITE;

        if (racer.is_player) {
            position_color = GREEN;
            name_color = LIME;

            DrawRectangle(board_x + 5, y_pos - 5, board_width - 10, line_height - 5,
                ColorAlpha(GREEN, 0.2f));
        }
        else {
            if (i == 0) position_color = GOLD;      // 1º Position
            else if (i == 1) position_color = LIGHTGRAY; // 2º Position
            else if (i == 2) position_color = ORANGE;    // 3º Position
            else position_color = WHITE;
        }

        char pos_text[8];
        sprintf_s(pos_text, "%d.", racer.position);
        DrawText(pos_text, board_x + 20, y_pos, 22, position_color);

        const char* name_display = racer.name.c_str();
        DrawText(name_display, board_x + 60, y_pos, 20, name_color);

        if (racer.is_player) {
            DrawText("[P]", board_x + board_width - 50, y_pos, 18, SKYBLUE);
        }
        else {
            DrawText("[AI]", board_x + board_width - 50, y_pos, 16, GRAY);
        }
    }

    DrawText("Press [TAB] to hide", board_x + 10,
        board_y + board_height - 25, 14, DARKGRAY);
}