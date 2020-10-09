#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include "Utils.h"

#include <string>

template<typename T>
void DrawPopup(std::string name, const T& obj, const bool isModal = false)
{
	if (isModal)
	{
		if (ImGui::BeginPopupModal(name.c_str()))
		{
			DrawEditor(obj);

			if(ImGui::Button("Close"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}
	else
	{
		if (ImGui::BeginPopup(name.c_str()))
		{
			DrawEditor(obj);

			if (ImGui::Button("Close"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}
}

template<typename T>
void DrawMenu(std::string name, const T& obj)
{
	if (ImGui::BeginMenu(name.c_str()))
	{
		DrawEditor(obj);
		ImGui::EndMenu();
	}
}

template<typename T>
void DrawWindow(std::string name, const T& obj, const bool canDock = false)
{
	ImGui::Begin(name.c_str(), nullptr, canDock ? 0 : ImGuiWindowFlags_NoDocking);
	DrawEditor(obj);
	ImGui::End();
}

template<typename T>
inline void DrawEditor(const T& obj)
{
	ImGui::Text((std::string("DrawEditor for ") + typeid(T).name() + " is undefined.").c_str());
}