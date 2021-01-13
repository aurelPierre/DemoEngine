#pragma once

#include <GLFW/glfw3.h>

#include <unordered_map>

#include <Wrappers/glm.h>

enum class KEY_CODE
{
	A,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
	I,
	J,
	K,
	L,
	M,
	N,
	O,
	P,
	Q,
	R,
	S,
	T,
	U,
	V,
	W,
	X,
	Y,
	Z,
	COUNT
};

enum class MOUSE_CODE
{
	LEFT,
	RIGHT,
	MIDDLE,
	EXTRA_1,
	EXTRA_2,
	EXTRA_3,
	EXTRA_4,
	EXTRA_5,
	COUNT
};

struct GLFWWindowData
{
	GLFWwindow* _window			= NULL;
	uint32_t	_width			= 1280;
	uint32_t	_height			= 720;

	bool		_shouldUpdate	= false;

	std::unordered_map<KEY_CODE, int>	_keyMap;
	std::unordered_map<int, bool>		_keyDownMap;

	std::unordered_map<MOUSE_CODE, int>	_mouseMap;
	std::unordered_map<int, bool>		_mouseDownMap;

	Vec2 _mousePos;

	bool IsKeyDown(const KEY_CODE kKey) { return _keyDownMap[_keyMap[kKey]]; }
	bool IsMouseDown(const MOUSE_CODE kKey) { return _mouseDownMap[_mouseMap[kKey]]; }
};

class GLFWWindowSystem
{
	GLFWWindowData	_windowData;

public:
	GLFWWindowData*	CreateWindow();
	void			DeleteWindow();

	bool			UpdateInput();
};