#pragma once

#include <Containers/Delegate.h>

class InputSystem
{
	public:

		void InitSystem();
		void ShutdownSystem();
		void TickSystem(float DeltaTime);

		void ToggleOcclusionBuffer();
		void ToggleBoundingBoxes();

	private:

		POINT PreviousCursorPosition, CurrentCursorPosition;

		enum class KeyState { Pressed, Released };

		KeyState KeyStates[255];
		Delegate<void> OnPressedKeyBindings[255];
		Delegate<void> OnReleasedKeyBindings[255];
};