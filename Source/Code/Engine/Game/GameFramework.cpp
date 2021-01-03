#include "GameFramework.h"

void GameFramework::InitFramework()
{
	world.LoadWorld();
}

void GameFramework::ShutdownFramework()
{
	world.UnLoadWorld();
}

void GameFramework::TickFramework(float DeltaTime)
{

}