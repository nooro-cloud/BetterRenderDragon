#include <windows.h>
#include <wrl.h>

#include "MCPatches.h"
#include "gui/Options.h"
#include "imgui/ImGuiHooks.h"

#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <thread>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "Global.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "MinHook.h"
#include "Version.h"
#include "api/Logger.h"
#include "api/memory/HookAPI.hpp"
#include "mc/deps/core/resource/ResourceLocation.h"

void initMCHooks();

// provided by MCHooks.cpp
extern void *resourcePackManager;
typedef bool (*PFN_ResourcePackManager_load)(void *This,
                                             const ResourceLocation &location,
                                             std::string &resourceStream);
extern PFN_ResourcePackManager_load ResourcePackManager_load;

static const char *kMaterials[] = {
    "RenderChunk",   "Actor",         "Sky",        "Clouds",
    "Stars",         "SunMoon",       "Weather",    "EndSky",
    "EndPortal",     "Particle",      "LegacyCubemap"};

static void copyShaderFromPacks() {
  namespace fs = std::filesystem;

  if (!resourcePackManager || !ResourcePackManager_load) {
    Logger::log("pack manager not ready - load a world first");
    return;
  }

  char buf[MAX_PATH];
  GetModuleFileNameA(NULL, buf, MAX_PATH);
  fs::path dest =
      fs::path(buf).parent_path() / "data" / "renderer" / "materials";

  std::error_code ec;
  fs::create_directories(dest, ec);

  int n = 0;
  for (const char *name : kMaterials) {
    std::string rel =
        std::string("renderer/materials/") + name + ".material.bin";

    std::string out;
    bool ok = false;
    try {
      ResourceLocation location(rel);
      ok = ResourcePackManager_load(resourcePackManager, location, out);
    } catch (...) {
      ok = false;
    }

    if (ok && !out.empty()) {
      fs::path target = dest / (std::string(name) + ".material.bin");
      std::ofstream f(target, std::ios::binary | std::ios::trunc);
      if (f) {
        f.write(out.data(), (std::streamsize)out.size());
        f.close();
        Logger::log("wrote %s (%zu bytes)", name, out.size());
        n++;
      } else {
        Logger::log("could not write %s", target.string().c_str());
      }
    }
  }

  Logger::log("pulled %d materials from active packs", n);
}

static void reloadKeyThread() {
  bool wasDown = false;
  while (true) {
    bool isDown = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
    if (isDown && !wasDown) {
      Logger::log("F8 pressed");
      copyShaderFromPacks();
      brd::Options::reloadShaders = true;
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
  Logger::log("F8 = load shaders from active resource pack");
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
