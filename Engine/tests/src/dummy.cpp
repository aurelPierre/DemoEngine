#include <cstdlib>

#include "VkRenderer/Core.h"
#include "GLFWWindowSystem.h"

int main(int, char**)
{
	{
		GLFWWindowSystem glfwWindow;
		GLFWWindowData* windowData = glfwWindow.CreateWindow();
		ASSERT(windowData != nullptr, "window data is nullptr")
		glfwWindow.DeleteWindow();
	}

	return EXIT_SUCCESS;
}