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
#include <psapi.h>

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

// Pull whatever the active resource packs provide.
// Returns how many material files were found (0 = pack has no shaders).
static int pullFromActivePack(bool verbose) {
  namespace fs = std::filesystem;

  if (!resourcePackManager || !ResourcePackManager_load) {
    if (verbose) Logger::log("pack manager not ready");
    return -1;
  }

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

  if (got.empty()) return 0;

  // Clean slate first so leftovers from a previous shader never linger.
  restoreVanilla();

  fs::path dest = materialsDir();
  std::error_code ec;
  fs::create_directories(dest, ec);

  int n = 0;
  for (auto &it : got) {
    fs::path target = dest / (it.name + ".material.bin");
    std::ofstream f(target, std::ios::binary | std::ios::trunc);
    if (f) {
      f.write(it.bytes.data(), (std::streamsize)it.bytes.size());
      f.close();
      if (verbose)
        Logger::log("wrote %s (%zu bytes)", it.name.c_str(), it.bytes.size());
      n++;
    }
  }
  Logger::log("applied %d materials from active packs", n);
  return n;
}

// Manual F8: apply now, and fall back to vanilla if the pack has no shaders.
static void applyNow() {
  int r = pullFromActivePack(true);
  if (r == 0) {
    Logger::log("no shader materials in active packs -> vanilla");
    restoreVanilla();
  }
  brd::Options::reloadShaders = true;
}

// Cheap probe: does the active pack stack currently expose ANY shader?
static bool packHasShaders() {
  if (!resourcePackManager || !ResourcePackManager_load) return false;
  for (const char *name : kMaterials) {
    std::string rel =
        std::string("renderer/materials/") + name + ".material.bin";
    std::string out;
    try {
      ResourceLocation location(rel);
      if (ResourcePackManager_load(resourcePackManager, location, out) &&
          !out.empty())
        return true;
    } catch (...) {
    }
  }
  return false;
}

static void watcherThread() {
  bool f8Was = false, f9Was = false;
  bool applied = false;
  int tick = 0;

  while (true) {
    // --- poll roughly once a second for shaders becoming available ---
    if (++tick >= 20) {
      tick = 0;
      bool avail = packHasShaders();

      if (avail && !applied) {
        Logger::log("shaders detected in active packs - applying");
        int r = pullFromActivePack(true);
        if (r > 0) {
          brd::Options::reloadShaders = true;
          applied = true;
          Logger::log("auto-applied %d materials", r);
        }
      } else if (!avail && applied) {
        // packs unmounted (left the world / turned the pack off)
        Logger::log("shaders no longer active - back to vanilla");
        restoreVanilla();
        brd::Options::reloadShaders = true;
        applied = false;
      }
    }

    // --- manual keys ---
    bool f8 = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
    bool f9 = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
    if (f8 && !f8Was) {
      Logger::log("F8 pressed");
      applyNow();
      applied = true;
    }
    if (f9 && !f9Was) {
      Logger::log("F9 pressed - force vanilla");
      restoreVanilla();
      brd::Options::reloadShaders = true;
      applied = false;
    }
    f8Was = f8;
    f9Was = f9;

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

// ---------------------------------------------------------------
// When loaded as version.dll we start with the process, long before the
// game's code is ready to scan. Waiting for a window is NOT enough: the
// window exists before the code we hook is mapped in. So poll for a
// signature we know exists and only continue once it resolves.
// ---------------------------------------------------------------
static uintptr_t scanFor(const char *signature) {
  HMODULE mod = GetModuleHandleA("Minecraft.Windows.exe");
  if (!mod) return 0;

  MODULEINFO mi{};
  if (!GetModuleInformation(GetCurrentProcess(), mod, &mi, sizeof(mi)))
    return 0;

  std::vector<uint16_t> pat;
  for (size_t i = 0; signature[i]; i++) {
    if (signature[i] == ' ') continue;
    if (signature[i] == '?') {
      pat.push_back(0xFF00);
      if (signature[i + 1] == '?') i++;
    } else {
      char b[3] = {signature[i], signature[i + 1], 0};
      i++;
      pat.push_back((uint16_t)strtoul(b, nullptr, 16));
    }
  }
  if (pat.empty()) return 0;

  uint8_t *base = (uint8_t *)mod;
  size_t size = mi.SizeOfImage;
  size_t n = pat.size();
  for (size_t i = 0; i + n < size; i++) {
    bool hit = true;
    for (size_t j = 0; j < n; j++) {
      if (pat[j] & 0xFF00) continue;
      if (base[i + j] != (uint8_t)pat[j]) { hit = false; break; }
    }
    if (hit) return (uintptr_t)(base + i);
  }
  return 0;
}

static bool waitForCode() {
  // ClientInstance::getResourcePackManager - only present once the game's
  // own code has been mapped and is scannable.
  const char *probe =
      "48 8B 89 ? ? ? ? 48 8B 01 48 8B 80 ? ? ? ? 48 8B 15 ? ? ? ? 48 FF E2 "
      "CC CC CC CC CC 48 8B 89 ? ? ? ? 48 8B 01 48 8B 80 ? ? ? ? 48 8B 15 ? "
      "? ? ? 48 FF E2 CC CC CC CC CC 56 48 83 EC ? 48 89 D6 48 8B 89 ? ? ? ? "
      "48 8B 01";

  for (int i = 0; i < 240; i++) {  // up to ~120 s
    if (scanFor(probe)) {
      Logger::log("game code ready after %d ms", i * 500);
      std::this_thread::sleep_for(std::chrono::seconds(1));
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  Logger::log("game code never became scannable (120s) - hooks will fail");
  return false;
}

void init() {
  std::filesystem::remove(Global::GetBRDRaomingPath() + "\\logs.txt");
  Logger::log("BetterRenderDragon %s", BetterRDVersion);
  brd::Options::init();
  brd::Options::load();

  waitForCode();

  MH_Initialize();
  initMCHooks();

  ensureBackup();
  // Always start from vanilla so a previous session never leaks through.
  restoreVanilla();

  Logger::log("auto-apply on world load | F8 = re-apply | F9 = vanilla");
  std::thread(watcherThread).detach();
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
