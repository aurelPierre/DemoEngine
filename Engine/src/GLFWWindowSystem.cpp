#include "GLFWWindowSystem.h"

#include "LogSystem.h"

#include <iostream>
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort

static void glfw_error_callback(int error, const char* description)
{
	LOG(ez::ERROR, std::string("Glfw Error ") + std::to_string(error) + ':' + description)
}

static void glfw_framebuffer_callback(GLFWwindow* window, int width, int height)
{
	GLFWWindowData* windowData = (GLFWWindowData*)glfwGetWindowUserPointer(window);

	windowData->_width = width;
	windowData->_height = height;
	windowData->_shouldUpdate = true;
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	GLFWWindowData* windowData = (GLFWWindowData*)glfwGetWindowUserPointer(window);

	if (action == GLFW_PRESS)
		windowData->_keyDownMap[key] = true;
	else if (action == GLFW_RELEASE)
		windowData->_keyDownMap[key] = false;
}

static void glfw_cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	GLFWWindowData* windowData = (GLFWWindowData*)glfwGetWindowUserPointer(window);

	windowData->_mousePos = Vec2{ xpos, ypos };
}

static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	GLFWWindowData* windowData = (GLFWWindowData*)glfwGetWindowUserPointer(window);

	if (action == GLFW_PRESS)
		windowData->_mouseDownMap[button] = true;
	else if (action == GLFW_RELEASE)
		windowData->_mouseDownMap[button] = false;
}

GLFWWindowData*	GLFWWindowSystem::CreateWindow()
{
	glfwSetErrorCallback(glfw_error_callback);

	if (!glfwInit())
		return nullptr;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	_windowData._window = glfwCreateWindow(_windowData._width, _windowData._height, "Dear ImGui GLFW+Vulkan example", NULL, NULL);
	
	glfwSetWindowUserPointer(_windowData._window, this);
	glfwSetFramebufferSizeCallback(_windowData._window, glfw_framebuffer_callback);

	_windowData._mouseMap[MOUSE_CODE::LEFT]		= GLFW_MOUSE_BUTTON_1;
	_windowData._mouseMap[MOUSE_CODE::RIGHT]	= GLFW_MOUSE_BUTTON_2;
	_windowData._mouseMap[MOUSE_CODE::MIDDLE]	= GLFW_MOUSE_BUTTON_3;
	_windowData._mouseMap[MOUSE_CODE::EXTRA_1]	= GLFW_MOUSE_BUTTON_4;
	_windowData._mouseMap[MOUSE_CODE::EXTRA_2]	= GLFW_MOUSE_BUTTON_5;
	_windowData._mouseMap[MOUSE_CODE::EXTRA_3]	= GLFW_MOUSE_BUTTON_6;
	_windowData._mouseMap[MOUSE_CODE::EXTRA_4]	= GLFW_MOUSE_BUTTON_7;
	_windowData._mouseMap[MOUSE_CODE::EXTRA_5]	= GLFW_MOUSE_BUTTON_8;

	glfwSetCursorPosCallback(_windowData._window, glfw_cursor_position_callback);
	glfwSetMouseButtonCallback(_windowData._window, glfw_mouse_button_callback);

	_windowData._keyMap[KEY_CODE::A] = GLFW_KEY_A;
	_windowData._keyMap[KEY_CODE::B] = GLFW_KEY_B;
	_windowData._keyMap[KEY_CODE::C] = GLFW_KEY_C;
	_windowData._keyMap[KEY_CODE::D] = GLFW_KEY_D;
	_windowData._keyMap[KEY_CODE::E] = GLFW_KEY_E;
	_windowData._keyMap[KEY_CODE::F] = GLFW_KEY_F;
	_windowData._keyMap[KEY_CODE::G] = GLFW_KEY_G;
	_windowData._keyMap[KEY_CODE::H] = GLFW_KEY_H;
	_windowData._keyMap[KEY_CODE::I] = GLFW_KEY_I;
	_windowData._keyMap[KEY_CODE::J] = GLFW_KEY_J;
	_windowData._keyMap[KEY_CODE::K] = GLFW_KEY_K;
	_windowData._keyMap[KEY_CODE::L] = GLFW_KEY_L;
	_windowData._keyMap[KEY_CODE::M] = GLFW_KEY_M;
	_windowData._keyMap[KEY_CODE::N] = GLFW_KEY_N;
	_windowData._keyMap[KEY_CODE::O] = GLFW_KEY_O;
	_windowData._keyMap[KEY_CODE::P] = GLFW_KEY_P;
	_windowData._keyMap[KEY_CODE::Q] = GLFW_KEY_Q;
	_windowData._keyMap[KEY_CODE::R] = GLFW_KEY_R;
	_windowData._keyMap[KEY_CODE::S] = GLFW_KEY_S;
	_windowData._keyMap[KEY_CODE::T] = GLFW_KEY_T;
	_windowData._keyMap[KEY_CODE::U] = GLFW_KEY_U;
	_windowData._keyMap[KEY_CODE::V] = GLFW_KEY_V;
	_windowData._keyMap[KEY_CODE::W] = GLFW_KEY_W;
	_windowData._keyMap[KEY_CODE::X] = GLFW_KEY_X;
	_windowData._keyMap[KEY_CODE::Y] = GLFW_KEY_Y;
	_windowData._keyMap[KEY_CODE::Z] = GLFW_KEY_Z;

	glfwSetKeyCallback(_windowData._window, glfw_key_callback);

	return &_windowData;
}

void GLFWWindowSystem::DeleteWindow()
{
	glfwDestroyWindow(_windowData._window);
	glfwTerminate();
}

bool GLFWWindowSystem::UpdateInput()
{
	if (glfwWindowShouldClose(_windowData._window))
		return true;

	_windowData._shouldUpdate = false;
	glfwPollEvents();

	return false;
}