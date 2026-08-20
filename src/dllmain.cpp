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
static void copyShaderFromPacks() {
  namespace fs = std::filesystem;
  const char *appData = std::getenv("APPDATA");
  if (!appData) { Logger::log("no APPDATA"); return; }

  fs::path packs = fs::path(appData) / "Minecraft Education Edition" /
      "games" / "com.mojang" / "resource_packs";
  if (!fs::exists(packs)) {
    Logger::log("no resource_packs folder: %s", packs.string().c_str());
    return;
  }

  fs::path best;
  fs::file_time_type bestTime{};
  std::error_code ec;
  for (auto &e : fs::directory_iterator(packs, ec)) {
    if (!e.is_directory()) continue;
    fs::path m = e.path() / "renderer" / "materials";
    if (!fs::exists(m)) continue;
    Logger::log("pack with materials: %s", e.path().filename().string().c_str());
    auto t = fs::last_write_time(e.path(), ec);
    if (best.empty() || t > bestTime) { best = m; bestTime = t; }
  }
  if (best.empty()) { Logger::log("no activated pack has renderer/materials"); return; }

  char buf[MAX_PATH];
  GetModuleFileNameA(NULL, buf, MAX_PATH);
  fs::path dest = fs::path(buf).parent_path() / "data" / "renderer" / "materials";
  Logger::log("copy from: %s", best.string().c_str());
  Logger::log("copy to  : %s", dest.string().c_str());

  int n = 0;
  for (auto &f : fs::directory_iterator(best, ec)) {
    if (f.path().extension() != ".bin") continue;
    fs::copy_file(f.path(), dest / f.path().filename(),
                  fs::copy_options::overwrite_existing, ec);
    if (ec) Logger::log("FAILED %s : %s", f.path().filename().string().c_str(),
                        ec.message().c_str());
    else n++;
  }
  Logger::log("copied %d material files", n);
}
static void reloadKeyThread() {
  bool wasDown = false;
  while (true) {
    bool isDown = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
    if (isDown && !wasDown) {
      copyShaderFromPacks();
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
