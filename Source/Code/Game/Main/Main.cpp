#include "Main.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Application::StartApplication(u"Paradox", hInstance);
	Application::RunMainLoop();
	Application::StopApplication();
	return 0;
}