#include "input_system.h"

#include <cassert>
#include <cstring>

namespace Visor
{
	static InputSystem* pInstance = nullptr;

	void InputSystem::update()
	{
		assert(pInstance != nullptr);
		_previousMouseState = _mouseState;
	}

	void InputSystem::setKeyPressed(Key key, b8 pressed)
	{
		_keyboardState._keyStates[(ui32)key] = pressed;
	}

	b8 InputSystem::isKeyPressed(Key key)
	{
		return _keyboardState._keyStates[(ui32)key];
	}

	void InputSystem::setMouseButtonPressed(MouseButton mouseButton, b8 pressed)
	{
		_mouseState.buttonStates[(ui32)mouseButton] = pressed;
	}

	b8 InputSystem::isMouseButtonPressed(MouseButton mouseButton)
	{
		return _mouseState.buttonStates[(ui32)mouseButton];
	}

	void InputSystem::setMousePosition(f32 x, f32 y)
	{
		_mouseState.x = x;
		_mouseState.y = y;
	}

	void InputSystem::getMousePosition(f32& x, f32& y)
	{
		x = _mouseState.x;
		y = _mouseState.y;
	}

	void InputSystem::getPreviousMousePosition(f32& x, f32& y)
	{
		x = _previousMouseState.x;
		y = _previousMouseState.y;
	}
	
	void InputSystem::start()
	{
		assert(pInstance == nullptr);
		pInstance = new InputSystem();
	}

	void InputSystem::terminate()
	{
		assert(pInstance != nullptr);
		delete pInstance;
		pInstance = nullptr;
	}

	InputSystem& InputSystem::getInstance()
	{
		assert(pInstance != nullptr);
		return *pInstance;
	}

	InputSystem::InputSystem()
	{
		std::memset(&_keyboardState, 0, sizeof(KeyboardState));
		std::memset(&_previousMouseState, 0, sizeof(MouseState));
		std::memset(&_mouseState, 0, sizeof(MouseState));
	}
}