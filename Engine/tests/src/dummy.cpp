#include <cstdlib>

#include "Core.h"
#include "GLFWWindowSystem.h"

#include "VkRenderer/Context.h"
#include "VkRenderer/Swapchain.h"

int main(int, char**)
{
	ez::LogSystem::_standardOutput = true;

	{
		GLFWWindowSystem glfwWindow;
		GLFWWindowData* windowData = glfwWindow.CreateWindow();
		ASSERT(windowData != nullptr, "window data is nullptr")

		Context context;
		Device device;
		LogicalDevice logicalDevice(device);
		Surface surface(windowData);
		Swapchain swapchain(surface, windowData);

		vkDeviceWaitIdle(logicalDevice._device);

		glfwWindow.DeleteWindow();
	}

	return EXIT_SUCCESS;
}