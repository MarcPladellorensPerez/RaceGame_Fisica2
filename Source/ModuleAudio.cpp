#include "Globals.h"
#include "Application.h"
#include "ModuleAudio.h"

#include "raylib.h"

ModuleAudio::ModuleAudio(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	fx_count = 0;
	music = Music{ 0 };

	// Clear the sound array
	for (int i = 0; i < MAX_FX_SOUNDS; i++)
	{
		fx[i] = Sound{ 0 };
	}
}

// Destructor
ModuleAudio::~ModuleAudio()
{
}

// Called before render is available
bool ModuleAudio::Init()
{
	LOG("Loading Audio Mixer");
	bool ret = true;

	LOG("Loading raylib audio system");

	InitAudioDevice();

	return ret;
}

// Called before quitting
bool ModuleAudio::CleanUp()
{
	LOG("Freeing sound FX, closing Mixer and Audio subsystem");

	// Unload sounds
	for (unsigned int i = 0; i < fx_count; i++)
	{
		UnloadSound(fx[i]);
	}

	// Unload music
	if (IsMusicReady(music))
	{
		StopMusicStream(music);
		UnloadMusicStream(music);
	}

	CloseAudioDevice();

	return true;
}

// Play a music file
bool ModuleAudio::PlayMusic(const char* path, float fade_time)
{
	if (IsEnabled() == false)
		return false;

	bool ret = true;

	StopMusicStream(music);
	music = LoadMusicStream(path);

	PlayMusicStream(music);

	LOG("Successfully playing %s", path);

	return ret;
}

// Load WAV
unsigned int ModuleAudio::LoadFx(const char* path)
{
	if (IsEnabled() == false)
		return 0;

	unsigned int ret = 0;

	Sound sound = LoadSound(path);

	if (sound.stream.buffer == NULL)
	{
		LOG("Cannot load sound: %s", path);
	}
	else
	{
		if (fx_count < MAX_FX_SOUNDS)
		{
			fx[fx_count++] = sound;
			ret = fx_count;
		}
		else
		{
			LOG("Cannot load sound %s: FX array is full", path);
		}
	}

	return ret;
}

// Play WAV
bool ModuleAudio::PlayFx(unsigned int id, int repeat)
{
	if (IsEnabled() == false)
	{
		return false;
	}

	bool ret = false;

	// Check if ID is valid
	if (id > 0 && id <= fx_count)
	{
		PlaySound(fx[id - 1]);
		ret = true;
	}

	return ret;
}

// Change the pitch of a sound effect
void ModuleAudio::SetFxPitch(unsigned int id, float pitch)
{
	if (id > 0 && id <= fx_count)
	{
		SetSoundPitch(fx[id - 1], pitch);
	}
}

// Change the volume of a specific effect
void ModuleAudio::SetFxVolume(unsigned int id, float volume)
{
	if (id > 0 && id <= fx_count)
	{
		SetSoundVolume(fx[id - 1], volume);
	}
}

// Check if an effect is playing 
bool ModuleAudio::IsFxPlaying(unsigned int id)
{
	if (id > 0 && id <= fx_count)
	{
		return IsSoundPlaying(fx[id - 1]);
	}
	return false;
}

// Stop a sound effect immediately
void ModuleAudio::StopFx(unsigned int id)
{
	if (id > 0 && id <= fx_count)
	{
		StopSound(fx[id - 1]);
	}
}