// ---------------------------------------------------------------
// Minecraft Education options.txt editor
//
// F8 opens a normal Windows window (alt-tab to it) listing every setting
// from options.txt. Pick one, type a new value, Set. Save writes the file.
//
// IMPORTANT: Minecraft holds its settings in memory and rewrites
// options.txt when it exits, which would wipe anything we write while the
// game is running. So we also keep our own copy in
//   %APPDATA%\BetterRenderDragon\pending_options.txt
// and re-apply it on process detach, AFTER Minecraft has written its own.
// Changes therefore take effect on the NEXT launch.
// ---------------------------------------------------------------
#include <windows.h>
#include <commctrl.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "Global.h"
#include "api/Logger.h"

#pragma comment(lib, "comctl32.lib")

namespace {

namespace fs = std::filesystem;

struct Entry {
  std::string key;
  std::string value;
};

std::vector<Entry> gEntries;
bool gDirty = false;

HWND gWnd = nullptr;
HWND gList = nullptr;
HWND gKeyLabel = nullptr;
HWND gEdit = nullptr;
HWND gStatus = nullptr;
HWND gFilter = nullptr;
std::vector<int> gVisible;  // indices into gEntries currently listed

const int ID_LIST = 1001;
const int ID_EDIT = 1002;
const int ID_SET = 1003;
const int ID_SAVE = 1004;
const int ID_RELOAD = 1005;
const int ID_FILTER = 1006;

fs::path optionsPath() {
  const char *appData = std::getenv("APPDATA");
  if (!appData) return {};
  return fs::path(appData) / "Minecraft Education Edition" / "games" /
         "com.mojang" / "minecraftpe" / "options.txt";
}

fs::path pendingPath() {
  return fs::path(Global::GetBRDRaomingPath()) / "pending_options.txt";
}

std::vector<Entry> parseFile(const fs::path &p) {
  std::vector<Entry> out;
  std::ifstream f(p);
  if (!f) return out;
  std::string line;
  while (std::getline(f, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.empty()) continue;
    size_t c = line.find(':');
    if (c == std::string::npos) continue;
    out.push_back({line.substr(0, c), line.substr(c + 1)});
  }
  return out;
}

void loadEntries() {
  gEntries = parseFile(optionsPath());
  // Overlay any pending edits so the UI shows what will actually apply.
  for (auto &pe : parseFile(pendingPath())) {
    bool found = false;
    for (auto &e : gEntries) {
      if (e.key == pe.key) {
        e.value = pe.value;
        found = true;
        break;
      }
    }
    if (!found) gEntries.push_back(pe);
  }
  gDirty = false;
}

void refreshList() {
  if (!gList) return;
  char filt[128] = {0};
  if (gFilter) GetWindowTextA(gFilter, filt, sizeof(filt) - 1);
  std::string needle = filt;
  for (auto &ch : needle) ch = (char)tolower((unsigned char)ch);

  SendMessageA(gList, LB_RESETCONTENT, 0, 0);
  gVisible.clear();
  for (size_t i = 0; i < gEntries.size(); i++) {
    if (!needle.empty()) {
      std::string k = gEntries[i].key;
      for (auto &ch : k) ch = (char)tolower((unsigned char)ch);
      if (k.find(needle) == std::string::npos) continue;
    }
    std::string row = gEntries[i].key + " = " + gEntries[i].value;
    SendMessageA(gList, LB_ADDSTRING, 0, (LPARAM)row.c_str());
    gVisible.push_back((int)i);
  }
}

void setStatus(const std::string &s) {
  if (gStatus) SetWindowTextA(gStatus, s.c_str());
}

// Write our pending file (survives Minecraft overwriting options.txt).
void savePending() {
  std::ofstream f(pendingPath(), std::ios::trunc);
  if (!f) {
    setStatus("ERROR: cannot write pending file");
    return;
  }
  for (auto &e : gEntries) f << e.key << ":" << e.value << "\n";
  f.close();
  gDirty = false;
  setStatus("Saved. Applies on next Minecraft launch.");
  Logger::log("options: saved %zu entries to pending", gEntries.size());
}

void onSelect() {
  int sel = (int)SendMessageA(gList, LB_GETCURSEL, 0, 0);
  if (sel < 0 || sel >= (int)gVisible.size()) return;
  const Entry &e = gEntries[gVisible[sel]];
  SetWindowTextA(gKeyLabel, e.key.c_str());
  SetWindowTextA(gEdit, e.value.c_str());
}

void onSet() {
  int sel = (int)SendMessageA(gList, LB_GETCURSEL, 0, 0);
  if (sel < 0 || sel >= (int)gVisible.size()) {
    setStatus("Pick a setting first");
    return;
  }
  char buf[512] = {0};
  GetWindowTextA(gEdit, buf, sizeof(buf) - 1);
  int idx = gVisible[sel];
  gEntries[idx].value = buf;
  gDirty = true;

  std::string row = gEntries[idx].key + " = " + gEntries[idx].value;
  SendMessageA(gList, LB_DELETESTRING, sel, 0);
  SendMessageA(gList, LB_INSERTSTRING, sel, (LPARAM)row.c_str());
  SendMessageA(gList, LB_SETCURSEL, sel, 0);
  setStatus("Changed " + gEntries[idx].key + " (not saved yet)");
}

LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_COMMAND: {
    int id = LOWORD(wp);
    int code = HIWORD(wp);
    if (id == ID_LIST && code == LBN_SELCHANGE) onSelect();
    else if (id == ID_SET) onSet();
    else if (id == ID_SAVE) savePending();
    else if (id == ID_RELOAD) {
      loadEntries();
      refreshList();
      setStatus("Reloaded from disk");
    } else if (id == ID_FILTER && code == EN_CHANGE) {
      refreshList();
    }
    return 0;
  }
  case WM_CLOSE:
    if (gDirty) {
      int r = MessageBoxA(h, "You have unsaved changes. Save before closing?",
                          "Unsaved changes", MB_YESNOCANCEL | MB_ICONQUESTION);
      if (r == IDCANCEL) return 0;
      if (r == IDYES) savePending();
    }
    DestroyWindow(h);
    return 0;
  case WM_DESTROY:
    gWnd = gList = gEdit = gStatus = gKeyLabel = gFilter = nullptr;
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcA(h, msg, wp, lp);
}

void uiThread() {
  const char *cls = "BRD_OptionsEditor";
  WNDCLASSA wc{};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = GetModuleHandleA(nullptr);
  wc.lpszClassName = cls;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  RegisterClassA(&wc);

  gWnd = CreateWindowExA(WS_EX_TOPMOST, cls, "Minecraft Options Editor",
                         WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                         720, 620, nullptr, nullptr, wc.hInstance, nullptr);
  if (!gWnd) {
    Logger::log("options UI: CreateWindow failed");
    return;
  }

  CreateWindowA("STATIC", "Filter:", WS_CHILD | WS_VISIBLE, 12, 14, 45, 20,
                gWnd, nullptr, wc.hInstance, nullptr);
  gFilter = CreateWindowA("EDIT", "",
                          WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                          60, 12, 260, 24, gWnd, (HMENU)ID_FILTER,
                          wc.hInstance, nullptr);

  gList = CreateWindowA("LISTBOX", "",
                        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                            LBS_NOTIFY | LBS_HASSTRINGS,
                        12, 46, 680, 400, gWnd, (HMENU)ID_LIST, wc.hInstance,
                        nullptr);

  CreateWindowA("STATIC", "Setting:", WS_CHILD | WS_VISIBLE, 12, 460, 55, 20,
                gWnd, nullptr, wc.hInstance, nullptr);
  gKeyLabel = CreateWindowA("STATIC", "(none)", WS_CHILD | WS_VISIBLE, 70, 460,
                            330, 20, gWnd, nullptr, wc.hInstance, nullptr);

  CreateWindowA("STATIC", "Value:", WS_CHILD | WS_VISIBLE, 12, 490, 55, 20,
                gWnd, nullptr, wc.hInstance, nullptr);
  gEdit = CreateWindowA("EDIT", "",
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 70,
                        488, 330, 24, gWnd, (HMENU)ID_EDIT, wc.hInstance,
                        nullptr);

  CreateWindowA("BUTTON", "Set", WS_CHILD | WS_VISIBLE, 415, 487, 80, 26, gWnd,
                (HMENU)ID_SET, wc.hInstance, nullptr);
  CreateWindowA("BUTTON", "Save", WS_CHILD | WS_VISIBLE, 505, 487, 90, 26,
                gWnd, (HMENU)ID_SAVE, wc.hInstance, nullptr);
  CreateWindowA("BUTTON", "Reload", WS_CHILD | WS_VISIBLE, 603, 487, 89, 26,
                gWnd, (HMENU)ID_RELOAD, wc.hInstance, nullptr);

  gStatus = CreateWindowA(
      "STATIC",
      "Edits are saved separately and applied on the NEXT launch, because "
      "Minecraft rewrites options.txt when it closes.",
      WS_CHILD | WS_VISIBLE, 12, 525, 680, 40, gWnd, nullptr, wc.hInstance,
      nullptr);

  loadEntries();
  refreshList();
  Logger::log("options UI open (%zu settings)", gEntries.size());

  ShowWindow(gWnd, SW_SHOW);
  UpdateWindow(gWnd);
  SetForegroundWindow(gWnd);

  MSG m;
  while (GetMessageA(&m, nullptr, 0, 0) > 0) {
    if (!IsDialogMessageA(gWnd, &m)) {
      TranslateMessage(&m);
      DispatchMessageA(&m);
    }
  }
  Logger::log("options UI closed");
}

}  // namespace

// Called from dllmain on F8.
void openOptionsUI() {
  if (gWnd) {
    SetForegroundWindow(gWnd);
    return;
  }
  std::thread(uiThread).detach();
}

// Called from dllmain on process detach, AFTER Minecraft has written its
// own options.txt, so our values win.
void applyPendingOptions() {
  auto pending = parseFile(pendingPath());
  if (pending.empty()) return;

  auto live = parseFile(optionsPath());
  if (live.empty()) return;

  int changed = 0;
  for (auto &pe : pending) {
    bool found = false;
    for (auto &e : live) {
      if (e.key == pe.key) {
        if (e.value != pe.value) {
          e.value = pe.value;
          changed++;
        }
        found = true;
        break;
      }
    }
    if (!found) {
      live.push_back(pe);
      changed++;
    }
  }
  if (!changed) return;

  std::ofstream f(optionsPath(), std::ios::trunc);
  if (!f) return;
  for (auto &e : live) f << e.key << ":" << e.value << "\n";
  f.close();
  Logger::log("options: applied %d pending change(s) on exit", changed);
}
