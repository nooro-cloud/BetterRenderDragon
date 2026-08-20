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
#include <vector>

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
    "RenderChunk", "Actor",     "Sky",       "Clouds",
    "Stars",       "SunMoon",   "Weather",   "EndSky",
    "EndPortal",   "Particle",  "LegacyCubemap"};

static std::filesystem::path materialsDir() {
  namespace fs = std::filesystem;
  char buf[MAX_PATH];
  GetModuleFileNameA(NULL, buf, MAX_PATH);
  return fs::path(buf).parent_path() / "data" / "renderer" / "materials";
}

static std::filesystem::path backupDir() {
  return std::filesystem::path(Global::GetBRDRaomingPath()) / "vanilla_backup";
}

// Save a pristine copy of the game's own materials, once.
// Only runs if the backup folder does not already exist.
static void ensureBackup() {
  namespace fs = std::filesystem;
  std::error_code ec;
  fs::path src = materialsDir();
  fs::path bak = backupDir();

  if (fs::exists(bak)) {
    int have = 0;
    for (auto &f : fs::directory_iterator(bak, ec)) {
      (void)f;
      have++;
    }
    Logger::log("vanilla backup present (%d files)", have);
    return;
  }

  fs::create_directories(bak, ec);
  int n = 0;
  for (auto &f : fs::directory_iterator(src, ec)) {
    if (f.path().extension() != ".bin") continue;
    fs::copy_file(f.path(), bak / f.path().filename(),
                  fs::copy_options::overwrite_existing, ec);
    if (!ec) n++;
  }
  Logger::log("CREATED vanilla backup: %d files -> %s", n,
              bak.string().c_str());
  Logger::log("  (if the game was modded when this ran, delete that folder");
  Logger::log("   and re-inject with vanilla materials in place)");
}

static int restoreVanilla() {
  namespace fs = std::filesystem;
  std::error_code ec;
  fs::path bak = backupDir();
  fs::path dst = materialsDir();
  if (!fs::exists(bak)) {
    Logger::log("no vanilla backup to restore");
    return 0;
  }
  int n = 0;
  for (auto &f : fs::directory_iterator(bak, ec)) {
    if (f.path().extension() != ".bin") continue;
    fs::copy_file(f.path(), dst / f.path().filename(),
                  fs::copy_options::overwrite_existing, ec);
    if (!ec) n++;
  }
  Logger::log("restored %d vanilla materials", n);
  return n;
}

static void loadFromActivePack() {
  namespace fs = std::filesystem;

  if (!resourcePackManager || !ResourcePackManager_load) {
    Logger::log("pack manager not ready - load a world first");
    return;
  }

  fs::path dest = materialsDir();
  std::error_code ec;
  fs::create_directories(dest, ec);

  // Collect first, write second: if the pack has no shaders we must not
  // leave a half-applied mix on disk.
  struct Item { std::string name, bytes; };
  std::vector<Item> got;

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
    if (ok && !out.empty()) got.push_back({name, std::move(out)});
  }

  if (got.empty()) {
    Logger::log("no shader materials in active packs -> vanilla");
    restoreVanilla();
    return;
  }

  // Start from a clean vanilla base so leftovers from a previous shader
  // never linger.
  restoreVanilla();

  int n = 0;
  for (auto &it : got) {
    fs::path target = dest / (it.name + ".material.bin");
    std::ofstream f(target, std::ios::binary | std::ios::trunc);
    if (f) {
      f.write(it.bytes.data(), (std::streamsize)it.bytes.size());
      f.close();
      Logger::log("wrote %s (%zu bytes)", it.name.c_str(), it.bytes.size());
      n++;
    }
  }
  Logger::log("applied %d materials from active packs", n);
}

static void reloadKeyThread() {
  bool f8Was = false, f9Was = false;
  while (true) {
    bool f8 = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
    bool f9 = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
    if (f8 && !f8Was) {
      Logger::log("F8 pressed");
      loadFromActivePack();
      brd::Options::reloadShaders = true;
    }
    if (f9 && !f9Was) {
      Logger::log("F9 pressed - force vanilla");
      restoreVanilla();
      brd::Options::reloadShaders = true;
    }
    f8Was = f8;
    f9Was = f9;
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

  ensureBackup();
  // Always start from vanilla so a previous session never leaks through.
  restoreVanilla();

  Logger::log("F8 = apply active resource pack   F9 = vanilla");
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
    // Game is closing: put the vanilla files back.
    restoreVanilla();
    break;
  }
  return TRUE;
}
