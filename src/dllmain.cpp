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

void initMCHooks();

static std::string readTextFile(const std::filesystem::path &p) {
  std::ifstream f(p, std::ios::binary);
  if (!f) return "";
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static std::string jsonStrAfter(const std::string &s, const std::string &key,
                                size_t from) {
  size_t k = s.find("\"" + key + "\"", from);
  if (k == std::string::npos) return "";
  size_t c = s.find(':', k);
  if (c == std::string::npos) return "";
  size_t q1 = s.find('"', c);
  if (q1 == std::string::npos) return "";
  size_t q2 = s.find('"', q1 + 1);
  if (q2 == std::string::npos) return "";
  return s.substr(q1 + 1, q2 - q1 - 1);
}

static void copyShaderFromPacks() {
  namespace fs = std::filesystem;
  const char *appData = std::getenv("APPDATA");
  if (!appData) {
    Logger::log("no APPDATA");
    return;
  }

  fs::path mojang =
      fs::path(appData) / "Minecraft Education Edition" / "games" / "com.mojang";
  fs::path packs = mojang / "resource_packs";
  if (!fs::exists(packs)) {
    Logger::log("no resource_packs folder");
    return;
  }

  std::string cfg =
      readTextFile(mojang / "minecraftpe" / "global_resource_packs.json");
  std::string wantUuid = jsonStrAfter(cfg, "pack_id", 0);
  std::string wantSub;
  if (!wantUuid.empty()) {
    size_t p = cfg.find("\"pack_id\"");
    size_t nxt = cfg.find("\"pack_id\"", p + 1);
    size_t sp = cfg.find("\"subpack\"", p);
    if (sp != std::string::npos && (nxt == std::string::npos || sp < nxt))
      wantSub = jsonStrAfter(cfg, "subpack", p);
  }
  Logger::log("active pack uuid: %s  subpack: %s",
              wantUuid.empty() ? "(none)" : wantUuid.c_str(),
              wantSub.empty() ? "(none)" : wantSub.c_str());

  fs::path best;
  std::error_code ec;
  for (auto &e : fs::directory_iterator(packs, ec)) {
    if (!e.is_directory()) continue;
    if (!fs::exists(e.path() / "renderer" / "materials")) continue;
    std::string mf = readTextFile(e.path() / "manifest.json");
    bool match = !wantUuid.empty() && mf.find(wantUuid) != std::string::npos;
    Logger::log("pack: %s %s", e.path().filename().string().c_str(),
                match ? "<== ACTIVE" : "");
    if (match) best = e.path();
  }
  if (best.empty()) {
    Logger::log("active pack not found among packs");
    return;
  }

  char buf[MAX_PATH];
  GetModuleFileNameA(NULL, buf, MAX_PATH);
  fs::path dest =
      fs::path(buf).parent_path() / "data" / "renderer" / "materials";

  int n = 0;
  auto copyFrom = [&](const fs::path &src) {
    if (!fs::exists(src)) return;
    for (auto &f : fs::directory_iterator(src, ec)) {
      if (f.path().extension() != ".bin") continue;
      fs::copy_file(f.path(), dest / f.path().filename(),
                    fs::copy_options::overwrite_existing, ec);
      if (ec)
        Logger::log("FAILED %s : %s", f.path().filename().string().c_str(),
                    ec.message().c_str());
      else
        n++;
    }
  };
  copyFrom(best / "renderer" / "materials");
  if (!wantSub.empty())
    copyFrom(best / "subpacks" / wantSub / "renderer" / "materials");

  Logger::log("copied %d material files from %s", n,
              best.filename().string().c_str());
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
