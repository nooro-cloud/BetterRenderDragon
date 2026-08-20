#include <windows.h>
#include <wrl.h>

#include "MCPatches.h"
#include "gui/Options.h"
#include "imgui/ImGuiHooks.h"

#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <thread>

#include "Global.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "MinHook.h"
#include "Version.h"
#include "api/Logger.h"
#include "api/memory/HookAPI.hpp"

void initMCHooks();

void init() {
  std::filesystem::remove(Global::GetBRDRaomingPath() + "\\logs.txt");
  Logger::log("BetterRenderDragon %s", BetterRDVersion);
  brd::Options::init();
  brd::Options::load();

  MH_Initialize();
  initMCPatches();
  initMCHooks();
  std::this_thread::sleep_for(std::chrono::seconds(1));
  initImGuiHooks();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  switch (ul_reason_for_call) {
  case DLL_PROCESS_ATTACH: {
    Global::hModule = hModule;
    DisableThreadLibraryCalls(hModule);
    CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)init, nullptr, 0, nullptr);
    break;
  }
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
    break;
  case DLL_PROCESS_DETACH:
    break;
  }
  return TRUE;
}
