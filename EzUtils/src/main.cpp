#include "GLFWWindowSystem.h"
#include "ImGuiSystem.h"
#include "LogSystem.h"
#include "Utils.h"

#include "Swapchain.h"

int main(int, char**)
{
	GLFWWindowSystem		glfwWindow;
	ImGuiSystem				imGui;
	GLFWWindowData*			windowData	= glfwWindow.CreateWindow();

	Context context = CreateContext();
	Device device = CreateDevice(context);
	LogicalDevice logicalDevice = CreateLogicalDevice(context, device);
	Surface surface = CreateSurface(context, logicalDevice, device, windowData);
	Swapchain swapchain = CreateSwapchain(context, logicalDevice, device, surface, windowData);

	imGui.Init(windowData, context, device, logicalDevice, swapchain);


		/****************/
		/***** LOOP *****/
		/****************/
		while (!glfwWindow.UpdateInput() ) // TODO create window abstraction
		{
			// Clear
			imGui.StartFrame();

			// Update


			// Draw
			ez::LogSystem::Draw();
			ez::ProfileSystem::Draw();
			if (!Draw(logicalDevice, swapchain))
			{
				ResizeSwapchain(context, logicalDevice, device, surface, windowData, swapchain);
				continue;
			}

			Render(logicalDevice, swapchain);
			if(!Present(logicalDevice, swapchain))
				ResizeSwapchain(context, logicalDevice, device, surface, windowData, swapchain);

			imGui.EndFrame();
		}

		/*****************/
		/***** CLEAR *****/
		/*****************/
		imGui.Clear();

		DestroySwapchain(context, logicalDevice, swapchain);
		DestroySurface(context, surface);
		DestroyLogicalDevice(context, logicalDevice);
		DestroyContext(context);

		glfwWindow.DeleteWindow();

		return EXIT_SUCCESS;

}