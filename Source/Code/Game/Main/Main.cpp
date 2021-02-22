// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "Main.h"

#include "../../Engine/Containers/String.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Application::StartApplication(u"Paradox", hInstance);
	Application::RunMainLoop();
	Application::StopApplication();
	return 0;
}