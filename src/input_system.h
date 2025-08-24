#pragma once

#include <types.h>

namespace Visor
{
	class InputSystem
	{
	public:
		enum class Key
		{
			Q,
			W,
			E,
			R,
			T,
			S,
			COUNT
		};

		enum class MouseButton
		{
			LEFT,
			RIGHT,
			MIDDLE,
			COUNT
		};

	public:
		void update();
		
		void setKeyPressed(Key key, b8 pressed);
		b8 isKeyPressed(Key key);
		void setMouseButtonPressed(MouseButton mouseButton, b8 pressed);
		b8 isMouseButtonPressed(MouseButton mouseButton);
		void setMousePosition(f32 x, f32 y);
		void getMousePosition(f32& x, f32& y);
		void getPreviousMousePosition(f32& x, f32& y);
		
		static void start();
		static void terminate();
		static InputSystem& getInstance();

	private:
		struct KeyboardState
		{
		public:
			b8 _keyStates[(ui32)Key::COUNT];
		};

		struct MouseState
		{
		public:
			f32 x;
			f32 y;
			b8 buttonStates[(ui32)MouseButton::COUNT];
		};

	private:
		InputSystem();

	private:
		KeyboardState _keyboardState;
		MouseState _previousMouseState;
		MouseState _mouseState;
	};
}