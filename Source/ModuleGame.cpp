#include "Globals.h"
#include "Application.h"
#include "ModuleRender.h"
#include "ModuleGame.h"
#include "ModuleAudio.h"
#include "ModulePhysics.h"

ModuleGame::ModuleGame(Application* app, bool start_enabled) : Module(app, start_enabled)
{
	LOG("ModuleGame: Constructor cridat");
}

ModuleGame::~ModuleGame()
{
	LOG("ModuleGame: Destructor cridat");
}

bool ModuleGame::Start()
{
	LOG("ModuleGame: Carregant escena de prova");
	bool ret = true;

	LOG("ModuleGame: Creant objectes de prova per validar fisica");

	App->physics->CreateCircle(640, 200, 25, PhysBodyType::DYNAMIC);
	LOG("ModuleGame: Cercle dinamic creat al centre");

	App->physics->CreateRectangle(640, 300, 50, 50, PhysBodyType::DYNAMIC);
	LOG("ModuleGame: Rectangle dinamic creat");

	App->physics->CreateRectangle(640, 600, 800, 40, PhysBodyType::STATIC);
	LOG("ModuleGame: Plataforma estatica creada");

	int ground_points[] = {
		0, 500,
		200, 520,
		400, 500,
		600, 520,
		800, 500,
		1000, 520,
		1200, 500
	};
	App->physics->CreateChain(0, 0, ground_points, 14, PhysBodyType::STATIC);
	LOG("ModuleGame: Cadena de terra creada");

	App->renderer->CenterCameraOn(640, 360);
	LOG("ModuleGame: Camera centrada");

	LOG("ModuleGame: Escena de prova carregada correctament");
	LOG("ModuleGame: Prem F1 per veure les col·lisions en mode debug");
	LOG("ModuleGame: En mode debug fes clic i arrossega els objectes");

	return ret;
}

bool ModuleGame::CleanUp()
{
	LOG("ModuleGame: Descarregant escena");

	return true;
}

update_status ModuleGame::Update()
{
	return UPDATE_CONTINUE;
}