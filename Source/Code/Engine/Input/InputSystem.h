#pragma once

class InputSystem
{
	public:

		void InitSystem();
		void ShutdownSystem();
		void TickSystem(float DeltaTime);

		void ProcessRawMouseInput(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	private:

		//POINT PreviousCursorPosition, CurrentCursorPosition;
		LONG MouseDeltaX, MouseDeltaY;
};