#include <cstdlib>

#include "GLFWWindowSystem.h"

int main(int, char**)
{
	{
		GLFWWindowSystem glfwWindow;
		GLFWWindowData* windowData = glfwWindow.CreateWindow();
		glfwWindow.DeleteWindow();
	}

	return EXIT_SUCCESS;
}