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

static void reloadKeyThread() {
  bool wasDown = false;
  while (true) {
    bool isDown = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
    if (isDown && !wasDown) {
      brd::Options::reloadShaders = true;
      Logger::log("F8 pressed - reload requested");
    }
    wasDown = isDown;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

void init() {
  std::filesystem::remove(Global::GetBRDRaomingPath() + "\\logs.txt");
  Logger::log("BetterRenderDragon %s", BetterRDVersion);
  brd::Options::init();
  brd::Options::load();

  MH_Initialize();
  initMCHooks();
  Logger::log("F8 = reload shaders");
  std::thread(reloadKeyThread).detach();
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
