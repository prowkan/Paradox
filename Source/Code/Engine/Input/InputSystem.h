#pragma once

class InputSystem
{
	public:

		void InitSystem();
		void ShutdownSystem();
		void TickSystem(float DeltaTime);

	private:

		POINT PreviousCursorPosition, CurrentCursorPosition;
};