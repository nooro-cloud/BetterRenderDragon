// ---------------------------------------------------------------
// Minecraft Education options.txt editor - Minecraft-styled UI
//
// F8 opens a window (alt-tab to it) listing every setting from options.txt.
// Pick one, type a new value, Set, then Save.
//
// Minecraft keeps settings in memory and rewrites options.txt on exit, so
// anything written while the game runs would be wiped. Edits are stored in
//   %APPDATA%\BetterRenderDragon\pending_options.txt
// and re-applied on process detach, AFTER Minecraft writes its own file.
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

// ---------------- Minecraft palette ----------------
const COLORREF kBtnFace     = RGB(198, 198, 198);
const COLORREF kBtnHi       = RGB(255, 255, 255);
const COLORREF kBtnLo       = RGB(85, 85, 85);
const COLORREF kBtnFaceHot  = RGB(160, 176, 208);
const COLORREF kText        = RGB(255, 255, 255);
const COLORREF kTextShadow  = RGB(63, 63, 63);
const COLORREF kTextHot     = RGB(255, 255, 160);
const COLORREF kPanelBg     = RGB(20, 20, 20);
const COLORREF kSelBg       = RGB(70, 90, 130);
const COLORREF kGreen       = RGB(190, 255, 190);

struct Entry {
  std::string key;
  std::string value;
};

std::vector<Entry> gEntries;
std::vector<int> gVisible;
bool gDirty = false;

HWND gWnd = nullptr, gList = nullptr, gEdit = nullptr, gFilter = nullptr;
HFONT gFont = nullptr, gFontBig = nullptr;
HBRUSH gDirtBrush = nullptr, gPanelBrush = nullptr;
HBITMAP gDirtBmp = nullptr;
std::string gSelKey, gStatus;

const int ID_LIST = 1001, ID_EDIT = 1002, ID_SET = 1003;
const int ID_SAVE = 1004, ID_RELOAD = 1005, ID_FILTER = 1006, ID_DONE = 1007;

// ---------------- file handling ----------------
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
  for (auto &pe : parseFile(pendingPath())) {
    bool found = false;
    for (auto &e : gEntries) {
      if (e.key == pe.key) { e.value = pe.value; found = true; break; }
    }
    if (!found) gEntries.push_back(pe);
  }
  gDirty = false;
}

void savePending() {
  std::ofstream f(pendingPath(), std::ios::trunc);
  if (!f) { gStatus = "ERROR: cannot write pending file"; return; }
  for (auto &e : gEntries) f << e.key << ":" << e.value << "\n";
  f.close();
  gDirty = false;
  gStatus = "Saved - applies next time you launch Minecraft";
  Logger::log("options: saved %zu entries", gEntries.size());
}

// ---------------- dirt background ----------------
void makeDirt() {
  const int T = 64;
  BITMAPINFO bi{};
  bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth = T;
  bi.bmiHeader.biHeight = -T;
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;

  void *bits = nullptr;
  gDirtBmp = CreateDIBSection(nullptr, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
  if (!gDirtBmp || !bits) return;

  unsigned int seed = 1337u;
  auto rnd = [&seed]() {
    seed = seed * 1103515245u + 12345u;
    return (seed >> 16) & 0x7FFF;
  };

  unsigned char *px = (unsigned char *)bits;
  for (int y = 0; y < T; y += 4) {
    for (int x = 0; x < T; x += 4) {
      int v = (int)(rnd() % 14);
      int r = 34 + v, g = 25 + v, b = 18 + v;
      for (int dy = 0; dy < 4; dy++) {
        for (int dx = 0; dx < 4; dx++) {
          int o = ((y + dy) * T + (x + dx)) * 4;
          px[o + 0] = (unsigned char)b;
          px[o + 1] = (unsigned char)g;
          px[o + 2] = (unsigned char)r;
          px[o + 3] = 255;
        }
      }
    }
  }
  gDirtBrush = CreatePatternBrush(gDirtBmp);
}

// ---------------- drawing helpers ----------------
void drawTextShadow(HDC dc, RECT r, const char *s, COLORREF col, UINT fmt) {
  SetBkMode(dc, TRANSPARENT);
  RECT sh = r;
  OffsetRect(&sh, 2, 2);
  SetTextColor(dc, kTextShadow);
  DrawTextA(dc, s, -1, &sh, fmt);
  SetTextColor(dc, col);
  DrawTextA(dc, s, -1, &r, fmt);
}

void fillSolid(HDC dc, RECT r, COLORREF c) {
  HBRUSH b = CreateSolidBrush(c);
  FillRect(dc, &r, b);
  DeleteObject(b);
}

void drawMcButton(HDC dc, RECT r, const char *text, bool hot, bool down) {
  fillSolid(dc, r, RGB(0, 0, 0));  // outer border

  RECT in = r;
  InflateRect(&in, -2, -2);
  fillSolid(dc, in, hot ? kBtnFaceHot : kBtnFace);

  // bevel
  RECT e = in;
  if (!down) {
    e = {in.left, in.top, in.right, in.top + 2};
    fillSolid(dc, e, kBtnHi);
    e = {in.left, in.top, in.left + 2, in.bottom};
    fillSolid(dc, e, kBtnHi);
    e = {in.left, in.bottom - 2, in.right, in.bottom};
    fillSolid(dc, e, kBtnLo);
    e = {in.right - 2, in.top, in.right, in.bottom};
    fillSolid(dc, e, kBtnLo);
  } else {
    e = {in.left, in.top, in.right, in.top + 2};
    fillSolid(dc, e, kBtnLo);
    e = {in.left, in.top, in.left + 2, in.bottom};
    fillSolid(dc, e, kBtnLo);
  }

  RECT t = in;
  if (down) OffsetRect(&t, 1, 1);
  SelectObject(dc, gFont);
  SetBkMode(dc, TRANSPARENT);
  RECT sh = t;
  OffsetRect(&sh, 1, 1);
  SetTextColor(dc, RGB(120, 120, 120));
  DrawTextA(dc, text, -1, &sh, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  SetTextColor(dc, hot ? RGB(40, 40, 60) : RGB(20, 20, 20));
  DrawTextA(dc, text, -1, &t, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ---------------- button hover subclass ----------------
LRESULT CALLBACK BtnProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR id,
                         DWORD_PTR ref) {
  static bool tracking = false;
  switch (m) {
  case WM_MOUSEMOVE: {
    if (!GetPropA(h, "hot")) {
      SetPropA(h, "hot", (HANDLE)1);
      InvalidateRect(h, nullptr, TRUE);
      TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, h, 0};
      TrackMouseEvent(&tme);
    }
    return 0;
  }
  case WM_MOUSELEAVE:
    RemovePropA(h, "hot");
    InvalidateRect(h, nullptr, TRUE);
    return 0;
  case WM_NCDESTROY:
    RemoveWindowSubclass(h, BtnProc, id);
    break;
  }
  (void)ref;
  (void)tracking;
  return DefSubclassProc(h, m, w, l);
}

HWND mkButton(HWND parent, const char *text, int x, int y, int w, int h,
              int id) {
  HWND b = CreateWindowA("BUTTON", text,
                         WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, x, y, w, h,
                         parent, (HMENU)(INT_PTR)id,
                         GetModuleHandleA(nullptr), nullptr);
  SetWindowSubclass(b, BtnProc, (UINT_PTR)id, 0);
  return b;
}

// ---------------- list ----------------
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
    SendMessageA(gList, LB_ADDSTRING, 0, (LPARAM) "");
    gVisible.push_back((int)i);
  }
  InvalidateRect(gList, nullptr, TRUE);
}

void onSelect() {
  int sel = (int)SendMessageA(gList, LB_GETCURSEL, 0, 0);
  if (sel < 0 || sel >= (int)gVisible.size()) return;
  const Entry &e = gEntries[gVisible[sel]];
  gSelKey = e.key;
  SetWindowTextA(gEdit, e.value.c_str());
  InvalidateRect(gWnd, nullptr, FALSE);
}

void onSet() {
  int sel = (int)SendMessageA(gList, LB_GETCURSEL, 0, 0);
  if (sel < 0 || sel >= (int)gVisible.size()) {
    gStatus = "Pick a setting from the list first";
    InvalidateRect(gWnd, nullptr, FALSE);
    return;
  }
  char buf[512] = {0};
  GetWindowTextA(gEdit, buf, sizeof(buf) - 1);
  int idx = gVisible[sel];
  gEntries[idx].value = buf;
  gDirty = true;
  gStatus = "Changed " + gEntries[idx].key + "  (not saved yet)";
  InvalidateRect(gList, nullptr, TRUE);
  InvalidateRect(gWnd, nullptr, FALSE);
}

// ---------------- window ----------------
LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_ERASEBKGND: {
    HDC dc = (HDC)wp;
    RECT rc;
    GetClientRect(h, &rc);
    if (gDirtBrush) FillRect(dc, &rc, gDirtBrush);
    else fillSolid(dc, rc, RGB(30, 24, 18));
    return 1;
  }
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(h, &ps);
    RECT rc;
    GetClientRect(h, &rc);

    SelectObject(dc, gFontBig);
    RECT t = {0, 16, rc.right, 56};
    drawTextShadow(dc, t, "Options", kText, DT_CENTER | DT_SINGLELINE);

    SelectObject(dc, gFont);
    RECT f = {40, 74, 130, 98};
    drawTextShadow(dc, f, "Search:", kText, DT_LEFT | DT_SINGLELINE);

    std::string sel = "Setting:  " + (gSelKey.empty() ? "(none selected)" : gSelKey);
    RECT s1 = {40, 500, rc.right - 40, 524};
    drawTextShadow(dc, s1, sel.c_str(), kText, DT_LEFT | DT_SINGLELINE);

    RECT s2 = {40, 534, 130, 558};
    drawTextShadow(dc, s2, "Value:", kText, DT_LEFT | DT_SINGLELINE);

    if (!gStatus.empty()) {
      RECT st = {40, 690, rc.right - 40, 714};
      drawTextShadow(dc, st, gStatus.c_str(), kGreen, DT_LEFT | DT_SINGLELINE);
    }
    RECT note = {40, 714, rc.right - 40, 740};
    drawTextShadow(dc, note,
                   "Changes apply the next time you launch Minecraft.",
                   RGB(180, 180, 180), DT_LEFT | DT_SINGLELINE);

    EndPaint(h, &ps);
    return 0;
  }
  case WM_DRAWITEM: {
    DRAWITEMSTRUCT *d = (DRAWITEMSTRUCT *)lp;
    if (d->CtlType == ODT_BUTTON) {
      char txt[64] = {0};
      GetWindowTextA(d->hwndItem, txt, sizeof(txt) - 1);
      bool hot = GetPropA(d->hwndItem, "hot") != nullptr;
      bool down = (d->itemState & ODS_SELECTED) != 0;
      drawMcButton(d->hDC, d->rcItem, txt, hot, down);
      return TRUE;
    }
    if (d->CtlType == ODT_LISTBOX) {
      if ((int)d->itemID < 0 || (int)d->itemID >= (int)gVisible.size())
        return TRUE;
      const Entry &e = gEntries[gVisible[d->itemID]];
      bool sel = (d->itemState & ODS_SELECTED) != 0;
      fillSolid(d->hDC, d->rcItem, sel ? kSelBg : kPanelBg);
      if (sel) {
        RECT b = d->rcItem;
        HBRUSH wb = CreateSolidBrush(RGB(255, 255, 255));
        FrameRect(d->hDC, &b, wb);
        DeleteObject(wb);
      }
      SelectObject(d->hDC, gFont);
      SetBkMode(d->hDC, TRANSPARENT);
      RECT k = d->rcItem;
      k.left += 10;
      SetTextColor(d->hDC, sel ? RGB(255, 255, 255) : RGB(210, 210, 210));
      DrawTextA(d->hDC, e.key.c_str(), -1, &k,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE);
      RECT v = d->rcItem;
      v.right -= 12;
      SetTextColor(d->hDC, sel ? kTextHot : RGB(140, 200, 140));
      DrawTextA(d->hDC, e.value.c_str(), -1, &v,
                DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
      return TRUE;
    }
    return TRUE;
  }
  case WM_CTLCOLOREDIT: {
    HDC dc = (HDC)wp;
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkColor(dc, kPanelBg);
    if (!gPanelBrush) gPanelBrush = CreateSolidBrush(kPanelBg);
    return (LRESULT)gPanelBrush;
  }
  case WM_COMMAND: {
    int id = LOWORD(wp), code = HIWORD(wp);
    if (id == ID_LIST && code == LBN_SELCHANGE) onSelect();
    else if (id == ID_SET) onSet();
    else if (id == ID_SAVE) { savePending(); InvalidateRect(h, nullptr, FALSE); }
    else if (id == ID_RELOAD) {
      loadEntries();
      refreshList();
      gStatus = "Reloaded from disk";
      InvalidateRect(h, nullptr, FALSE);
    } else if (id == ID_DONE) PostMessageA(h, WM_CLOSE, 0, 0);
    else if (id == ID_FILTER && code == EN_CHANGE) refreshList();
    return 0;
  }
  case WM_CLOSE:
    if (gDirty) {
      int r = MessageBoxA(h, "You have unsaved changes. Save them?",
                          "Unsaved changes", MB_YESNOCANCEL | MB_ICONQUESTION);
      if (r == IDCANCEL) return 0;
      if (r == IDYES) savePending();
    }
    DestroyWindow(h);
    return 0;
  case WM_DESTROY:
    gWnd = gList = gEdit = gFilter = nullptr;
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcA(h, msg, wp, lp);
}

void uiThread() {
  const char *cls = "BRD_OptionsEditor";
  HINSTANCE inst = GetModuleHandleA(nullptr);

  WNDCLASSA wc{};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = inst;
  wc.lpszClassName = cls;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = nullptr;
  RegisterClassA(&wc);

  gFont = CreateFontA(19, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                      CLEARTYPE_QUALITY, FF_DONTCARE, "Consolas");
  gFontBig = CreateFontA(34, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                         OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         CLEARTYPE_QUALITY, FF_DONTCARE, "Consolas");
  makeDirt();

  RECT want = {0, 0, 780, 780};
  AdjustWindowRect(&want, WS_OVERLAPPEDWINDOW, FALSE);
  gWnd = CreateWindowExA(WS_EX_TOPMOST, cls, "Minecraft Options",
                         WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                         want.right - want.left, want.bottom - want.top,
                         nullptr, nullptr, inst, nullptr);
  if (!gWnd) {
    Logger::log("options UI: CreateWindow failed");
    return;
  }

  gFilter = CreateWindowA("EDIT", "",
                          WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                          134, 72, 606, 28, gWnd, (HMENU)ID_FILTER, inst,
                          nullptr);
  SendMessageA(gFilter, WM_SETFONT, (WPARAM)gFont, TRUE);

  gList = CreateWindowA("LISTBOX", "",
                        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
                            LBS_NOTIFY | LBS_OWNERDRAWFIXED,
                        40, 112, 700, 372, gWnd, (HMENU)ID_LIST, inst,
                        nullptr);
  SendMessageA(gList, LB_SETITEMHEIGHT, 0, 24);

  gEdit = CreateWindowA("EDIT", "",
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                        134, 532, 606, 28, gWnd, (HMENU)ID_EDIT, inst,
                        nullptr);
  SendMessageA(gEdit, WM_SETFONT, (WPARAM)gFont, TRUE);

  mkButton(gWnd, "Set Value",   40,  580, 340, 40, ID_SET);
  mkButton(gWnd, "Save",       400,  580, 340, 40, ID_SAVE);
  mkButton(gWnd, "Reload",      40,  630, 340, 40, ID_RELOAD);
  mkButton(gWnd, "Done",       400,  630, 340, 40, ID_DONE);

  loadEntries();
  refreshList();
  gStatus = "Loaded " + std::to_string(gEntries.size()) + " settings";
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

  if (gFont) { DeleteObject(gFont); gFont = nullptr; }
  if (gFontBig) { DeleteObject(gFontBig); gFontBig = nullptr; }
  if (gDirtBrush) { DeleteObject(gDirtBrush); gDirtBrush = nullptr; }
  if (gDirtBmp) { DeleteObject(gDirtBmp); gDirtBmp = nullptr; }
  if (gPanelBrush) { DeleteObject(gPanelBrush); gPanelBrush = nullptr; }
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

// Called from dllmain on process detach, AFTER Minecraft has rewritten
// options.txt, so our values win.
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
        if (e.value != pe.value) { e.value = pe.value; changed++; }
        found = true;
        break;
      }
    }
    if (!found) { live.push_back(pe); changed++; }
  }
  if (!changed) return;

  std::ofstream f(optionsPath(), std::ios::trunc);
  if (!f) return;
  for (auto &e : live) f << e.key << ":" << e.value << "\n";
  f.close();
  Logger::log("options: applied %d pending change(s) on exit", changed);
}
