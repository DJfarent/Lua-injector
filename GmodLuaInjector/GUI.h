#pragma once
#include <Windows.h>
#include <d3dx9.h>
#include <d3d9.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h"
#include "Kiero/kiero.h"
#include "Executor.h"
#include "Utils.h"
#pragma comment(lib,"d3d9.lib")

#if _WIN64
#define GWL_WNDPROC GWLP_WNDPROC
#pragma comment(lib,"libMinHook-x64.lib")
#else
#pragma comment(lib,"libMinHook-x86.lib")
#endif

typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
IMGUI_IMPL_API LRESULT  ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

EndScene oEndScene = NULL;
WNDPROC oWndProc;
static HWND window = NULL;
char buf[65535];
namespace fs = std::filesystem;
static bool openMenu = false;

std::string doFileStuff()
{
	static int selection_mask = (1 << 2);
	int node_clicked = -1;
	static std::string output = "";
	ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, ImGui::GetFontSize());
	std::function<void(const fs::path&, unsigned int, unsigned int&)> functor = [&](const fs::path& path, unsigned int depth, unsigned int& idx) {
		for (auto&& p : fs::directory_iterator(path)) {
			if (fs::is_directory(p.path())) {
				using namespace std::string_literals;
				std::string s = p.path().filename().string().c_str();
				if (ImGui::TreeNodeEx((void*)(intptr_t)idx, ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick, "%s", s.c_str())) {
					functor(p, depth + 1, ++idx);
					ImGui::TreePop();
				}
			}
			else {
				ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ((selection_mask & (1 << idx)) ? ImGuiTreeNodeFlags_Selected : 0);
				if (depth > 0) {
					ImGui::Indent();
				}
				ImGui::TreeNodeEx((void*)(intptr_t)idx, node_flags, "%s", p.path().filename().string().c_str());
				if (ImGui::IsItemClicked() || ImGui::IsItemFocused())
				{
					node_clicked = idx;
					output = p.path().string();
				}
				if (depth > 0) {
					ImGui::Unindent();
				}
			}
			++idx;
		}
		depth -= 1;
	};
	unsigned int idx = 0u;
	functor(fs::path("C:\\GaztoofScriptHook"), 0u, idx);
	if (node_clicked != -1) {
		selection_mask = (1 << node_clicked);
	}
	ImGui::PopStyleVar();
	return output;
}

extern std::string lastFileName;
extern std::string selectedFilePath;

long __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	static std::string selectedFilePathTemp = "";
	static bool initialized = false;
	
	if(!initialized)
	{
		std::cout << "[GUI] Initializing ImGui context..." << std::endl;
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;

		ImGui::StyleColorsClassic();
		auto vStyle = &ImGui::GetStyle();
		vStyle->WindowPadding = ImVec2(15, 15);
		vStyle->WindowRounding = 5.0f;
		vStyle->FramePadding = ImVec2(5, 5);
		vStyle->FrameRounding = 4.0f;
		vStyle->ItemSpacing = ImVec2(12, 8);
		vStyle->ItemInnerSpacing = ImVec2(8, 6);
		vStyle->IndentSpacing = 25.0f;
		vStyle->ScrollbarSize = 15.0f;
		vStyle->ScrollbarRounding = 9.0f;
		vStyle->GrabMinSize = 5.0f;
		vStyle->GrabRounding = 3.0f;
		vStyle->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
		vStyle->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		vStyle->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		vStyle->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		vStyle->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
		vStyle->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		vStyle->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		vStyle->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		vStyle->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		vStyle->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		vStyle->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
		vStyle->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		vStyle->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		vStyle->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		vStyle->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		vStyle->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		vStyle->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		vStyle->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		vStyle->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		vStyle->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		vStyle->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		vStyle->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		vStyle->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		vStyle->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		vStyle->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		vStyle->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		vStyle->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		vStyle->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		vStyle->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		vStyle->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		vStyle->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		vStyle->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		vStyle->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		vStyle->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
		vStyle->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);
		vStyle->WindowTitleAlign.x = 0.50f;
		vStyle->FrameRounding = 2.0f;
		
		std::cout << "[GUI] Initializing Win32..." << std::endl;
		ImGui_ImplWin32_Init(window);
		std::cout << "[GUI] Initializing D3D9..." << std::endl;
		ImGui_ImplDX9_Init(pDevice);
		std::cout << "[GUI] ImGui initialization complete!" << std::endl;
		initialized = true;
	};

	bool tempState = GetAsyncKeyState(VK_INSERT) & 0x8000;
	static bool lastState = false;
	if (tempState && !lastState) {
		openMenu = !openMenu;
		std::cout << "[GUI] Menu toggled: " << (openMenu ? "ON" : "OFF") << std::endl;
	}
	lastState = tempState;
	
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (openMenu)
	{
		ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
		ImGui::Begin("Garry's Mod Scripthook/Executor");
		static int i = 0;
		if (ImGui::Button("Executor", ImVec2(100, 20)))i = 0;
		ImGui::SameLine();
		if (ImGui::Button("Scripts", ImVec2(100, 20)))i = 1;
		switch (i)
		{
		case 0:
		{
			ImGui::InputTextMultiline("##script", buf, IM_ARRAYSIZE(buf), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_AllowTabInput);
			if (selectedFilePath != selectedFilePathTemp)
			{
				if (selectedFilePath.length())
				{
					std::ifstream stream(selectedFilePath);
					std::string fileContent = std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
					strcpy(buf, fileContent.c_str());
				}
			}
			selectedFilePathTemp = selectedFilePath;
			if (ImGui::Button(("Load Script")))
				std::thread(openFileDialog).detach();

			ImGui::SameLine();
			if (ImGui::Button(("Execute")))
				Execute("", std::string(buf));
			break;
		}
		case 1:
		{
			static std::string filePath;
			std::string temp = doFileStuff();
			static std::string fileContent = "";
			if (filePath != temp)
			{
				std::ifstream stream(temp);
				fileContent = std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
			}
			filePath = temp;
			ImGui::InputTextMultiline("##scripts", (char*)fileContent.c_str(), fileContent.length(), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_ReadOnly);
			break;
		}
		default:
			break;
		}
		ImGui::End();
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return oEndScene(pDevice);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (openMenu && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

void InitializeGUI() {
	ZeroMemory(buf, 65535);
	
	std::cout << "[GUI] Starting Kiero initialization..." << std::endl;
	int attempts = 0;
	while (true)
	{
		attempts++;
		std::cout << "[GUI] Kiero init attempt #" << attempts << "..." << std::endl;
		
		auto status = kiero::init(kiero::RenderType::D3D9);
		if (status == kiero::Status::Success)
		{
			std::cout << "[GUI] Kiero initialized successfully!" << std::endl;
			
			std::cout << "[GUI] Binding EndScene hook..." << std::endl;
			auto bindStatus = kiero::bind(42, (void**)&oEndScene, hkEndScene);
			if (bindStatus == kiero::Status::Success) {
				std::cout << "[GUI] EndScene hook bound successfully!" << std::endl;
			} else {
				std::cout << "[GUI] ERROR: Failed to bind EndScene hook!" << std::endl;
			}

			std::cout << "[GUI] Finding Garry's Mod window..." << std::endl;
			window = FindWindow(NULL, L"Garry's Mod (x64)");
			if (window) {
				std::cout << "[GUI] Window found! Handle: " << window << std::endl;
				oWndProc = (WNDPROC)SetWindowLongPtrA(window, GWL_WNDPROC, (LONG_PTR)WndProc);
				std::cout << "[GUI] Window procedure hooked!" << std::endl;
			} else {
				std::cout << "[GUI] ERROR: Could not find Garry's Mod window!" << std::endl;
			}
			break;
		}
		else {
			std::cout << "[GUI] Kiero init failed, retrying..." << std::endl;
			Sleep(100);
			if (attempts > 50) {
				std::cout << "[GUI] ERROR: Kiero init failed after 50 attempts!" << std::endl;
				break;
			}
		}
	}
}