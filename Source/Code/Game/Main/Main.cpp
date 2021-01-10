#include "Main.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Application::StartApplication((const wchar_t*)u"Paradox", hInstance);
	Application::RunMainLoop();
	Application::StopApplication();
	return 0;
}