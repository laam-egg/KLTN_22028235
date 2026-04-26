#include <windows.h>
#include "pmd_ui/app.h"

using namespace pmd_ui;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
	App app;
	return app.Run(hInstance, nCmdShow);
}

