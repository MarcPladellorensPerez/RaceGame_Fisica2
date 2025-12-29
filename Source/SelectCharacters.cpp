#include "SelectCharacters.h"
#include <cmath>

CharacterSelect::CharacterSelect()
    : background_texture({ 0 }), selected_index(0), confirmed(false),
    hover_offset(-30.0f), animation_speed(8.0f), scale_factor(0.18f),
    start_x(160), start_y(180), spacing_x(260), spacing_y(300),
    characters_per_row(4), glow_pulse(0.0f), selection_alpha(1.0f) {
}

CharacterSelect::~CharacterSelect() {
}

void CharacterSelect::Init() {
    background_texture = LoadTexture("Assets/Textures/UI/backgroundSelectCharacter.png");

    LoadCharacters();

    // Character
    if (!characters.empty()) {
        characters[selected_index].target_offset_y = hover_offset;
        characters[selected_index].current_offset_y = hover_offset;
    }

    LOG("CharacterSelect: Initialized with %d characters", (int)characters.size());
}

void CharacterSelect::LoadCharacters() {
    // List of character and their cars
    struct CharacterData {
        const char* name;
        int car_id;
    };

    const CharacterData character_data[] = {
        {"Aina", 2},      
        {"Alex", 3},      
        {"Christian", 4}, 
        {"Ismael", 5},    
        {"Jonay", 6},     
        {"Jordi", 7},     
        {"Lucia", 8},     
        {"Marc", 1}       
    };

   
    for (int i = 0; i < 8; ++i) {
        Character character;
        character.name = character_data[i].name;

        
        char char_path[256];
        sprintf_s(char_path, "Assets/Textures/Characters/%s.png", character_data[i].name);
        character.texture = LoadTexture(char_path);

        char car_path[256];
        sprintf_s(car_path, "Assets/Textures/Cars/car%d.png", character_data[i].car_id);
        character.car_texture = LoadTexture(car_path);

        character.current_offset_y = 0.0f;
        character.target_offset_y = 0.0f;

        characters.push_back(character);
    }
}

void CharacterSelect::CleanUp() {
    if (background_texture.id != 0) {
        UnloadTexture(background_texture);
        background_texture = { 0 };
    }

    for (auto& character : characters) {
        if (character.texture.id != 0) {
            UnloadTexture(character.texture);
        }
        if (character.car_texture.id != 0) {
            UnloadTexture(character.car_texture);
        }
    }

    characters.clear();
}

void CharacterSelect::Update(float dt) {
    HandleInput();
    UpdateAnimations(dt);

    glow_pulse += dt * 3.0f;
    if (glow_pulse > 6.28f) glow_pulse = 0.0f;

    selection_alpha = 0.7f + sinf(glow_pulse) * 0.3f;
}

void CharacterSelect::HandleInput() {
    if (confirmed) return;

    int previous_index = selected_index;

    // Keys Navigating
    if (IsKeyPressed(KEY_RIGHT)) {
        selected_index++;
        if (selected_index >= (int)characters.size()) {
            selected_index = 0;
        }
    }
    else if (IsKeyPressed(KEY_LEFT)) {
        selected_index--;
        if (selected_index < 0) {
            selected_index = (int)characters.size() - 1;
        }
    }
    else if (IsKeyPressed(KEY_DOWN)) {
        selected_index += characters_per_row;
        if (selected_index >= (int)characters.size()) {
            selected_index = selected_index % characters_per_row;
        }
    }
    else if (IsKeyPressed(KEY_UP)) {
        selected_index -= characters_per_row;
        if (selected_index < 0) {
            selected_index = (int)characters.size() - characters_per_row + (selected_index % characters_per_row);
            if (selected_index < 0) selected_index = 0;
        }
    }

    
    if (previous_index != selected_index) {
        characters[previous_index].target_offset_y = 0.0f;
        characters[selected_index].target_offset_y = hover_offset;

        LOG("Selected character: %s (index %d)",
            characters[selected_index].name.c_str(), selected_index);
    }

    // Confirm selection with enter o space
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        confirmed = true;
        LOG("Character confirmed: %s", characters[selected_index].name.c_str());
    }
}

void CharacterSelect::UpdateAnimations(float dt) {
    for (auto& character : characters) {
        float diff = character.target_offset_y - character.current_offset_y;
        character.current_offset_y += diff * animation_speed * dt;

        if (fabs(diff) < 0.5f) {
            character.current_offset_y = character.target_offset_y;
        }
    }
}

void CharacterSelect::Draw() {
    if (background_texture.id != 0) {
        float scale_x = (float)SCREEN_WIDTH / background_texture.width;
        float scale_y = (float)SCREEN_HEIGHT / background_texture.height;
        float scale = fmaxf(scale_x, scale_y);

        int bg_width = (int)(background_texture.width * scale);
        int bg_height = (int)(background_texture.height * scale);
        int bg_x = (SCREEN_WIDTH - bg_width) / 2;
        int bg_y = (SCREEN_HEIGHT - bg_height) / 2;

        Rectangle source = { 0, 0, (float)background_texture.width, (float)background_texture.height };
        Rectangle dest = { (float)bg_x, (float)bg_y, (float)bg_width, (float)bg_height };
        Vector2 origin = { 0, 0 };

        DrawTexturePro(background_texture, source, dest, origin, 0.0f, WHITE);
    }
    else {
        DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
            Color{ 20, 20, 40, 255 }, Color{ 10, 10, 20, 255 });
    }

    // Title
    const char* title = "SELECT YOUR CHARACTER";
    int title_width = MeasureText(title, 50);
    DrawText(title, (SCREEN_WIDTH - title_width) / 2, 40, 50, GOLD);

    DrawLineEx({ 100, 110 }, { (float)SCREEN_WIDTH - 100, 110 }, 3.0f, GOLD);

    // Draw all the characters
    for (int i = 0; i < (int)characters.size(); ++i) {
        DrawCharacter(characters[i], i);
    }

    DrawSelectionFrame(selected_index);

    const char* instruction1 = "Use ARROW KEYS to select";
    const char* instruction2 = "Press ENTER to confirm";
    int instr1_width = MeasureText(instruction1, 16);
    int instr2_width = MeasureText(instruction2, 16);

    DrawText(instruction1, (SCREEN_WIDTH - instr1_width) / 2, 125, 16, LIGHTGRAY);
    DrawText(instruction2, (SCREEN_WIDTH - instr2_width) / 2, 145, 16, LIGHTGRAY);


    const char* selected_name = characters[selected_index].name.c_str();
    int name_width = MeasureText(selected_name, 30);
    DrawText(selected_name, (SCREEN_WIDTH - name_width) / 2, SCREEN_HEIGHT - 80, 40, YELLOW);
}

void CharacterSelect::DrawCharacter(const Character& character, int index) {
    if (character.texture.id == 0) return;

    int row = index / characters_per_row;
    int col = index % characters_per_row;

    int base_x = start_x + col * spacing_x;
    int base_y = start_y + row * spacing_y;

    int final_y = base_y + (int)character.current_offset_y;

    int char_width = (int)(character.texture.width * scale_factor);
    int char_height = (int)(character.texture.height * scale_factor);

    int draw_x = base_x + (spacing_x - char_width) / 2;
    int draw_y = final_y;

    Rectangle source = { 0, 0, (float)character.texture.width, (float)character.texture.height };
    Rectangle dest = { (float)draw_x, (float)draw_y, (float)char_width, (float)char_height };
    Vector2 origin = { 0, 0 };

    Color tint = (index == selected_index) ? WHITE : ColorAlpha(WHITE, 0.7f);

    if (index == selected_index) {
        DrawRectangle(draw_x + 5, draw_y + char_height - 10, char_width, 15,
            ColorAlpha(BLACK, 0.5f));
    }

    DrawTexturePro(character.texture, source, dest, origin, 0.0f, tint);

    const char* name = character.name.c_str();
    int name_width = MeasureText(name, 20);
    int name_x = base_x + (spacing_x - name_width) / 2;
    int name_y = base_y + (int)(char_height * 1.1f);

    Color name_color = (index == selected_index) ? YELLOW : LIGHTGRAY;
    DrawText(name, name_x, name_y, 20, name_color);
}

void CharacterSelect::DrawSelectionFrame(int index) {
    int row = index / characters_per_row;
    int col = index % characters_per_row;

    int base_x = start_x + col * spacing_x;
    int base_y = start_y + row * spacing_y;
    int final_y = base_y + (int)characters[index].current_offset_y;

    int char_width = (int)(characters[index].texture.width * scale_factor);
    int char_height = (int)(characters[index].texture.height * scale_factor);

    int draw_x = base_x + (spacing_x - char_width) / 2;

    int border_thickness = 4;
    int padding = 10;

    Rectangle frame = {
        (float)(draw_x - padding),
        (float)(final_y - padding),
        (float)(char_width + padding * 2),
        (float)(char_height + padding * 2)
    };

    DrawRectangleLinesEx(frame, (float)(border_thickness + 2),
        ColorAlpha(GOLD, selection_alpha * 0.3f));

    DrawRectangleLinesEx(frame, (float)border_thickness,
        ColorAlpha(GOLD, selection_alpha));

    int corner_size = 20;
    DrawLineEx({ frame.x, frame.y }, { frame.x + corner_size, frame.y },
        (float)border_thickness, GOLD);
    DrawLineEx({ frame.x, frame.y }, { frame.x, frame.y + corner_size },
        (float)border_thickness, GOLD);

    DrawLineEx({ frame.x + frame.width - corner_size, frame.y },
        { frame.x + frame.width, frame.y }, (float)border_thickness, GOLD);
    DrawLineEx({ frame.x + frame.width, frame.y },
        { frame.x + frame.width, frame.y + corner_size }, (float)border_thickness, GOLD);

    DrawLineEx({ frame.x, frame.y + frame.height - corner_size },
        { frame.x, frame.y + frame.height }, (float)border_thickness, GOLD);
    DrawLineEx({ frame.x, frame.y + frame.height },
        { frame.x + corner_size, frame.y + frame.height }, (float)border_thickness, GOLD);

    DrawLineEx({ frame.x + frame.width - corner_size, frame.y + frame.height },
        { frame.x + frame.width, frame.y + frame.height }, (float)border_thickness, GOLD);
    DrawLineEx({ frame.x + frame.width, frame.y + frame.height - corner_size },
        { frame.x + frame.width, frame.y + frame.height }, (float)border_thickness, GOLD);
}

Texture2D CharacterSelect::GetSelectedCarTexture() const {
    if (selected_index >= 0 && selected_index < (int)characters.size()) {
        return characters[selected_index].car_texture;
    }
    return { 0 };
}

void CharacterSelect::Reset() {
    confirmed = false;
    selected_index = 0;
    glow_pulse = 0.0f;

    for (size_t i = 0; i < characters.size(); ++i) {
        characters[i].current_offset_y = 0.0f;
        characters[i].target_offset_y = 0.0f;
    }

    if (!characters.empty()) {
        characters[0].target_offset_y = hover_offset;
        characters[0].current_offset_y = hover_offset;
    }
}