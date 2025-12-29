#pragma once

#include "Globals.h"
#include "raylib.h"
#include <vector>
#include <string>

struct Character {
    std::string name;
    Texture2D texture;
    Texture2D car_texture;
    float current_offset_y;  
    float target_offset_y;  
};

class CharacterSelect {
public:
    CharacterSelect();
    ~CharacterSelect();

    void Init();
    void CleanUp();
    void Update(float dt);
    void Draw();

    int GetSelectedCharacterIndex() const { return selected_index; }
    Texture2D GetSelectedCarTexture() const;
    bool IsConfirmed() const { return confirmed; }
    void Reset();

private:
    std::vector<Character> characters;
    Texture2D background_texture;

    int selected_index;
    bool confirmed;

    float hover_offset;     
    float animation_speed;  
    float scale_factor;     

   
    int start_x;
    int start_y;
    int spacing_x;
    int spacing_y;
    int characters_per_row;

    // Effects
    float glow_pulse;        
    float selection_alpha;  

    void LoadCharacters();
    void UpdateAnimations(float dt);
    void HandleInput();
    void DrawCharacter(const Character& character, int index);
    void DrawSelectionFrame(int index);
};
