#pragma once

#include "imgui.h"
#include "imgui_internal.h"

namespace ez
{
	class Editor
	{
	public:
		static void Init()
		{
			ImGuiID dockspace_id = ImGui::GetID("dockspace");
			ImGuiViewport* viewport = ImGui::GetMainViewport();

			ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout

			ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // Add empty node
			ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

			ImGuiID dock_main_id = dockspace_id; // This variable will track the document node, however we are not using it here as we aren't docking anything into it.
			ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
			ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.20f, nullptr, &dock_main_id);
			ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.20f, nullptr, &dock_main_id);

			//ImGui::DockBuilderDockWindow(SystemLog::ID.c_str(), dock_id_bottom);
			//ImGui::DockBuilderDockWindow(FormManager::PROPERTY_EDITOR_ID.c_str(), dock_id_right);
			//ImGui::DockBuilderDockWindow(FormManager::PROJECT_EXPLORER_ID.c_str(), dock_id_left);
			//ImGui::DockBuilderDockWindow("Extra", dock_id_prop);

			ImGui::DockBuilderDockWindow("dummyLeft", dock_id_left);
			ImGui::DockBuilderDockWindow("dummyMain", dock_main_id);
			ImGui::DockBuilderDockWindow("dummyRight", dock_id_right);
			ImGui::DockBuilderDockWindow("dummyBot", dock_id_bottom);
			ImGui::DockBuilderFinish(dockspace_id);
		}

		static void Draw()
		{
			bool open = true;

			ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("EZ_Editor", &open, window_flags);
			ImGui::PopStyleVar(3);

			ImGui::BeginMenuBar();

			ImGui::MenuItem("Demo", "f", &open);

			ImGui::EndMenuBar();

			if (ImGui::DockBuilderGetNode(ImGui::GetID("dockspace")) == nullptr)
			{
				Init();
			}

			ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, { 35, 65, 90, 255 });
			ImGuiID dockspace_id = ImGui::GetID("dockspace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), 0);
			ImGui::PopStyleColor();
			ImGui::End();
		}
	};
}