#pragma once

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>

struct GLFWWindowData
{
	GLFWwindow* _window			= NULL;
	uint32_t	_width			= 1280;
	uint32_t	_height			= 720;

	bool		_shouldUpdate	= false;
};

class GLFWWindowSystem
{
	GLFWWindowData	_windowData;

public:
	GLFWWindowData*	CreateWindow();
	void			DeleteWindow();

	bool			UpdateInput();
};