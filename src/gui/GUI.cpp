#include "GUI.h"

#include "Global.h"
#include "imgui.h"
#include "imgui_internal.h"

#include "Options.h"
#include "Version.h"
using namespace brd;

void SetupImGuiStyle() {
	// Hazy Dark style by kaitabuchi314 from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.6f;
	style.WindowPadding = ImVec2(5.5f, 8.3f);
	style.WindowRounding = 8.5f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(32.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;
	style.ChildRounding = 7.2f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 2.7f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(4.0f, 3.0f);
	style.FrameRounding = 2.4f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(8.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 14.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 10.0f;
	style.GrabRounding = 3.2f;
	style.TabRounding = 7.5f;
	style.TabBorderSize = 1.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.49803922f, 0.49803922f, 0.49803922f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05882353f, 0.05882353f, 0.05882353f, 0.94f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.078431375f, 0.078431375f, 0.078431375f, 0.94f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.42745098f, 0.42745098f, 0.49803922f, 0.5f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.13725491f, 0.17254902f, 0.22745098f, 0.54f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.21176471f, 0.25490198f, 0.3019608f, 0.4f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.043137256f, 0.047058824f, 0.047058824f, 0.67f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.039215688f, 0.039215688f, 0.039215688f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.078431375f, 0.08235294f, 0.09019608f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.51f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.13725491f, 0.13725491f, 0.13725491f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.019607844f, 0.019607844f, 0.019607844f, 0.53f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30980393f, 0.30980393f, 0.30980393f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40784314f, 0.40784314f, 0.40784314f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50980395f, 0.50980395f, 0.50980395f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.7176471f, 0.78431374f, 0.84313726f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.47843137f, 0.5254902f, 0.57254905f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2901961f, 0.31764707f, 0.3529412f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.14901961f, 0.16078432f, 0.1764706f, 0.4f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.13725491f, 0.14509805f, 0.15686275f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.078431375f, 0.08627451f, 0.09019608f, 1.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.19607843f, 0.21568628f, 0.23921569f, 0.31f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.16470589f, 0.1764706f, 0.19215687f, 0.8f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.07450981f, 0.08235294f, 0.09019608f, 1.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.42745098f, 0.42745098f, 0.49803922f, 0.5f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.23921569f, 0.3254902f, 0.42352942f, 0.78f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.27450982f, 0.38039216f, 0.49803922f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2901961f, 0.32941177f, 0.3764706f, 0.2f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.23921569f, 0.29803923f, 0.36862746f, 0.67f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.16470589f, 0.1764706f, 0.1882353f, 0.95f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.11764706f, 0.1254902f, 0.13333334f, 0.862f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.32941177f, 0.40784314f, 0.5019608f, 0.8f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.24313726f, 0.24705882f, 0.25490198f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667f, 0.101960786f, 0.14509805f, 0.9724f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13333334f, 0.25882354f, 0.42352942f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.60784316f, 0.60784316f, 0.60784316f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.42745098f, 0.34901962f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392f, 0.69803923f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882353f, 0.1882353f, 0.2f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.30980393f, 0.30980393f, 0.34901962f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.22745098f, 0.22745098f, 0.24705882f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 0.35f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
}

void initializeImGui(bool isDx12) {
	if (ImGui::GetCurrentContext()) {
		return;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	SetupImGuiStyle();

	HRSRC hResource = FindResource(Global::hModule, MAKEINTRESOURCE(101), "TTF");
	HGLOBAL hMemory = LoadResource(Global::hModule, hResource);
	void* pFontData = LockResource(hMemory);
	DWORD fileSize = SizeofResource(Global::hModule, hResource);

	ImFontConfig fontConfig;
	fontConfig.FontDataOwnedByAtlas = false;

	io.FontDefault = io.Fonts->AddFontFromMemoryTTF(pFontData, fileSize, 24.0f, &fontConfig);

	io.IniFilename = nullptr;
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	if (isDx12) {
		unsigned char *pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	}
}

inline bool IsChangingUIKey = false;
inline bool JustChangedKey = false;
inline bool IsChangingIsChangingReloadShadersKey = false;

void updateImGui() {
	static bool showDemo = false;
	static bool showModuleManager = false;
	static bool showCredits = false;
	static bool showRayTracingDebugger = false;
	static bool showDeferGUI = false;

	bool resetLayout = false;
	bool moduleManagerRequestFocus = false;
	bool creditsRequestFocus = false;

	updateKeys();
	updateOptions();

	ImGui::NewFrame();
	if (Options::showImGui) {
		auto &io = ImGui::GetIO();

		if(io.FontGlobalScale != Options::fontSize.get()) {
			io.FontGlobalScale = Options::fontSize.get();
		}

		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("BetterRenderDragon", Options::showImGui.ptr(),
						 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar)) {
			if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("View")) {
					if (ImGui::MenuItem("Open Module Manager")) {
						showModuleManager = true;
						moduleManagerRequestFocus = true;
					}
					if (ImGui::MenuItem("Open RayTracing Debugger")) {
						showRayTracingDebugger = true;
					}
					if (ImGui::MenuItem("Open Deferred Shading Debugger")) {
						showDeferGUI = true;
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Windows")) {
					ImGui::SliderFloat("Font size", &Options::fontSize.get(), 0.5f, 4.0f, "%.2f");
					ImGui::Separator();
					if (ImGui::MenuItem("Reset window position and size"))
						resetLayout = true;
					if (showModuleManager || showDemo)
						ImGui::Separator();
					if (showModuleManager && ImGui::MenuItem("Module Manager"))
						moduleManagerRequestFocus = true;;
					if (showCredits && ImGui::MenuItem("Changelog"))
						creditsRequestFocus = true;
					if (showDemo)
						ImGui::MenuItem("ImGui Demo Window");
					ImGui::EndMenu();
				}
				// if (ImGui::BeginMenu("Eject")) {
				//   if (ImGui::MenuItem("Eject (\"Removes BetterRenderDragon from the game.\")")) {
				//     Global::eject = true;
				//   }
				//   ImGui::EndMenu();
				// }
				if (ImGui::BeginMenu("Credits")) {
					if (ImGui::MenuItem("Credits")) {
						showCredits = true;
						creditsRequestFocus = true;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			ImGui::Text("BetterRenderDragon %s", BetterRDVersion);
			ImGui::NewLine();

			if (Options::performanceEnabled && ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
				static float framerate = io.Framerate;
				static float deltaTime = io.DeltaTime;
				static float updateTimer = 0.5f;
				updateTimer -= io.DeltaTime;
				if (updateTimer <= 0) {
					framerate = io.Framerate;
					deltaTime = io.DeltaTime;
					updateTimer = 0.5f;
				}
				ImGui::Indent();
				ImGui::Text("FPS: %.01f", framerate);
				ImGui::Text("Frame Time: %.01fms", deltaTime * 1000.0f);
				ImGui::Unindent();
										}
			if (Options::graphicsEnabled && ImGui::CollapsingHeader("Graphics", ImGuiTreeNodeFlags_DefaultOpen)) {
				if (!Options::vanilla2DeferredAvailable)
					ImGui::BeginDisabled();
				ImGui::Indent();
				ImGui::Checkbox("Force Enable Vibrant Visuals", Options::forceEnableVibrantVisuals.ptr());
				// ImGui::Checkbox("Disable RTX (Restart Required)", Options::disableRendererContextD3D12RTX.ptr());
				ImGui::Unindent();
				if (!Options::vanilla2DeferredAvailable)
					ImGui::EndDisabled();
			}

			if (Options::materialBinLoaderEnabled && ImGui::CollapsingHeader("MaterialBinLoader", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Indent();
				ImGui::Checkbox("Load Shaders from Resource Pack",
								Options::redirectShaders.ptr());
				if (Options::reloadShadersAvailable) {
					bool disable = Options::reloadShaders;
					if (disable)
						ImGui::BeginDisabled();
					if (ImGui::Button("Reload shaders")) {
						Options::reloadShaders = true;
					}
					if (disable)
						ImGui::EndDisabled();
				}
				ImGui::Unindent();
			}

			if (Options::settingsEnabled && ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Indent();
				if (ImGui::Button(IsChangingUIKey == false ? "Set UI Key" : "Cancel")) {
					IsChangingUIKey = !IsChangingUIKey;
				}
				if (IsChangingUIKey) {
					for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; i++) {
						ImGuiKey key = static_cast<ImGuiKey>(i);

						if (!ImGui::IsMouseKey(key) && ImGui::IsKeyDown(key)) {
							Options::uiKey = key;
							IsChangingUIKey = false;

							ImGui::GetIO().AddKeyEvent(key, false);
							JustChangedKey = true;
							break;
						}
					}
				}
				ImGui::SameLine();
				ImGui::Text("Current: %s", ImGui::GetKeyName(static_cast<ImGuiKey>(Options::uiKey.get())));

				if (ImGui::Button(IsChangingIsChangingReloadShadersKey == false ? "Set Reload Shaders Key" : "Cancel")) {
					IsChangingIsChangingReloadShadersKey = !IsChangingIsChangingReloadShadersKey;
				}
				if (IsChangingIsChangingReloadShadersKey) {
					for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; i++) {
						ImGuiKey key = static_cast<ImGuiKey>(i);

						if (!ImGui::IsMouseKey(key) && ImGui::IsKeyDown(key)) {
							Options::reloadShadersKey = key;
							IsChangingIsChangingReloadShadersKey = false;

							ImGui::GetIO().AddKeyEvent(key, false);
							break;
						}
					}
				}
				ImGui::SameLine();
				ImGui::Text("Current: %s", ImGui::GetKeyName(static_cast<ImGuiKey>(Options::reloadShadersKey.get())));
				ImGui::Unindent();
			}

			if (Options::customUniformsEnabled &&
				ImGui::CollapsingHeader("CustomUniforms", ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::Indent();
					ImGui::Unindent();
				}
			ImGui::NewLine();
		}
		ImGui::End();

		if (showModuleManager) {
			if (moduleManagerRequestFocus)
				ImGui::SetNextWindowFocus();
			if (ImGui::Begin("BetterRenderDragon - Module Manager", &showModuleManager)) {
				if (ImGui::BeginTable("modulesTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
					ImGui::TableSetupColumn("Module");
					ImGui::TableSetupColumn("Enabled");
					ImGui::TableHeadersRow();

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Performance");
					ImGui::TableSetColumnIndex(1);
					ImGui::Checkbox("##1", Options::performanceEnabled.ptr());

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Graphics");
					ImGui::TableSetColumnIndex(1);
					ImGui::Checkbox("##2", Options::graphicsEnabled.ptr());

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("MaterialBinLoader");
					ImGui::TableSetColumnIndex(1);
					ImGui::Checkbox("##3", Options::materialBinLoaderEnabled.ptr());

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Settings");
					ImGui::TableSetColumnIndex(1);
					ImGui::Checkbox("##4", Options::settingsEnabled.ptr());

					ImGui::EndTable();
				}
			}
			ImGui::End();
		}

		if (showRayTracingDebugger) {
			DrawRayTracingDebugWindow(&showRayTracingDebugger);
		}

		if (showDeferGUI) {
			DrawDeferredShadingParametersDebugWindow(gDeferredParams, &showDeferGUI);
		}

		if (showCredits) {
			if (creditsRequestFocus)
				ImGui::SetNextWindowFocus();
			if (ImGui::Begin("BetterRenderDragon - Credits", &showCredits)) {
				ImGui::Text("Source: https://github.com/QYCottage/BetterRenderDragon");
				ImGui::Separator();
				ImGui::Text("Original creators: https://github.com/ddf8196/BetterRenderDragon");
				ImGui::Text("Previous maintainer: https://github.com/dreamguxiang");
				ImGui::Text("Current maintainer: https://github.com/St0neHunter");
			}
			ImGui::End();
		}

		if (showDemo)
			ImGui::ShowDemoWindow(&showDemo);
	}
	ImGui::EndFrame();

	if (resetLayout) {
		ImGui::ClearWindowSettings("BetterRenderDragon");
		ImGui::ClearWindowSettings("BetterRenderDragon - Module Manager");
		ImGui::ClearWindowSettings("Dear ImGui Demo");
	}
}

void updateOptions() {
	static float saveTimer = 0.0f;

	if (Options::isDirty()) {
		saveTimer = 3.0f;
	}
	Options::record();

	// TODO: Put it on a separate thread
	if (saveTimer > 0.0f) {
		saveTimer -= ImGui::GetIO().DeltaTime;
		if (saveTimer <= 0.0f) {
			saveTimer = 0.0f;
 			Options::save();
		}
	}
}

static bool isValidImGuiKey(int key) {
	return key >= ImGuiKey_NamedKey_BEGIN && key < ImGuiKey_NamedKey_END;
}

void updateKeys() {
	static bool prevToggleImGui = false;

	bool toggleImGui = false;
	if (isValidImGuiKey(Options::uiKey.get())) {
		toggleImGui = ImGui::IsKeyPressed(static_cast<ImGuiKey>(Options::uiKey.get())) && !JustChangedKey;
	}
	if (!toggleImGui)
		JustChangedKey = false;
	if (toggleImGui && !prevToggleImGui)
		Options::showImGui = !Options::showImGui;
	prevToggleImGui = toggleImGui;

	if (Options::reloadShadersAvailable && !Options::reloadShaders &&
		isValidImGuiKey(Options::reloadShadersKey.get()) &&
		ImGui::IsKeyPressed(static_cast<ImGuiKey>(Options::reloadShadersKey.get()))) {
			Options::reloadShaders = true;
		}
}
