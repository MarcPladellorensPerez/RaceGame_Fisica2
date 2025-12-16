#include "Globals.h"
#include "Application.h"

#include "raylib.h"

#include <stdlib.h>

enum main_states
{
	MAIN_CREATION,
	MAIN_START,
	MAIN_UPDATE,
	MAIN_FINISH,
	MAIN_EXIT
};

Application* App = NULL;

int main(int argc, char** argv)
{
	LOG("Iniciant joc '%s'...", TITLE);

	int main_return = EXIT_FAILURE;
	main_states state = MAIN_CREATION;

	while (state != MAIN_EXIT)
	{
		switch (state)
		{
		case MAIN_CREATION:

			LOG("-------------- Creacio de l'Aplicacio --------------");
			App = new Application();
			state = MAIN_START;
			break;

		case MAIN_START:

			LOG("-------------- Inicialitzacio de l'Aplicacio --------------");
			if (App->Init() == false)
			{
				LOG("L'inicialitzacio de l'aplicacio ha fallat");
				state = MAIN_EXIT;
			}
			else
			{
				state = MAIN_UPDATE;
				LOG("-------------- Actualitzacio de l'Aplicacio --------------");
			}

			break;

		case MAIN_UPDATE:
		{
			int update_return = App->Update();

			if (update_return == UPDATE_ERROR)
			{
				LOG("L'actualitzacio de l'aplicacio ha fallat");
				state = MAIN_EXIT;
			}

			if (update_return == UPDATE_STOP)
				state = MAIN_FINISH;
		}
		break;

		case MAIN_FINISH:

			LOG("-------------- Neteja de l'Aplicacio --------------");
			if (App->CleanUp() == false)
			{
				LOG("La neteja de l'aplicacio ha fallat");
			}
			else
				main_return = EXIT_SUCCESS;

			state = MAIN_EXIT;

			break;

		}
	}

	delete App;
	LOG("Sortint del joc '%s'...\n", TITLE);
	return main_return;
}