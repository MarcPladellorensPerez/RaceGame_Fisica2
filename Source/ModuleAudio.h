#pragma once
#include "Module.h"
#include "raylib.h"

#define MAX_FX_SOUNDS 64

class ModuleAudio : public Module
{
public:
	ModuleAudio(Application* app, bool start_enabled = true);
	~ModuleAudio();

	bool Init();
	bool CleanUp();

	// Play a music file
	bool PlayMusic(const char* path, float fade_time = 1.0f);

	// Load a WAV in memory
	unsigned int LoadFx(const char* path);

	// Play a previously loaded WAV
	bool PlayFx(unsigned int fx, int repeat = 0);

	void SetFxPitch(unsigned int id, float pitch);
	void SetFxVolume(unsigned int id, float volume);
	bool IsFxPlaying(unsigned int id);
	void StopFx(unsigned int id);

public:
	Music music;
	Sound fx[MAX_FX_SOUNDS];
	unsigned int fx_count;
};