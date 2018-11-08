﻿#include "FishingBoat.h"
#include "json.hpp"

namespace nlohmann {
void from_json(const nlohmann::json &json, cv::Rect &rect) {
  rect.x = json[0];
  rect.y = json[1];
  rect.width = json[2];
  rect.height = json[3];
}

void to_json(nlohmann::json &json, const cv::Rect &rect) {
  json[0] = rect.x;
  json[1] = rect.y;
  json[2] = rect.width;
  json[3] = rect.height;
}
}  // namespace nlohmann

// constants

constexpr int FISHING_IDLE = 0;
constexpr int FISHING_START = 1;
constexpr int FISHING_START_LOOP = 2;
constexpr int FISHING_STOP = 3;
constexpr int FISHING_STOP_LOOP = 4;
constexpr int FISHING_GUESS_WASD = 5;
constexpr int FISHING_INPUT_TEXT = 6;
constexpr int FISHING_TAKE_DROP = 7;
constexpr int FISHING_RESTART = 8;

// variables

std::mutex g_mutex;
std::wstring g_inifile;
nlohmann::json g_json;
int64 g_startBegin;
int64 g_stopBegin;
std::map<std::wstring, cv::Mat> g_tmpls;
std::wstring g_wasd;

// running steps

int __stdcall StartFishing() {
  std::lock_guard<std::mutex> locker(g_mutex);

  LogPrintf(L"準備");
  g_startBegin = timeNow();

  return FISHING_START_LOOP;
}

int __stdcall StartLoop() {
  std::lock_guard<std::mutex> locker(g_mutex);
  int64 startEnd;
  int startTimeout;

  startEnd = timeNow();
  startTimeout = g_json["TimeoutStart"];

  if ((int)(startEnd - g_startBegin) >= startTimeout) {
    // break
    return FISHING_STOP;
  }

  // loop
  sleepFor(100);
  return FISHING_START_LOOP;
}

int __stdcall StopFishing() {
  std::lock_guard<std::mutex> locker(g_mutex);
  cv::Mat box;
  cv::Rect boxRect;
  int64 stopEnd;
  int stopTimeout, fishingTimeout, sliderDelay, sliderLen;

  LogPrintf(L"收竿");
  g_stopBegin = stopEnd = timeNow();

  stopTimeout = g_json["TimeoutStop"];
  fishingTimeout = g_json["TimeoutFishing"];
  boxRect = g_json["BoxRect"];
  sliderDelay = g_json["SliderDelay"];
  sliderLen = g_json["SliderLen"];

  if ((stopEnd - g_startBegin) >= fishingTimeout) {
    // abort
    LogPrintf(L"超時");
    return FISHING_IDLE;
  }

  if (cursorShowing()) {
    LogPrintf(L"使用者中斷");
    return FISHING_STOP_LOOP;
  }

  LogPrintf(L"空白鍵(第1次)");
  keyPress(VK_SPACE);
  sleepFor(sliderDelay);

  box = screenshot(boxRect);

  if (sliderBar(box, sliderLen)) {
    keyPress(VK_SPACE);
    LogPrintf(L"空白鍵(第2次)");
    return FISHING_GUESS_WASD;
  }

  return FISHING_STOP_LOOP;
}

int __stdcall StopLoop() {
  std::lock_guard<std::mutex> locker(g_mutex);
  int64 stopEnd;
  int stopTimeout;

  stopEnd = timeNow();
  stopTimeout = g_json["TimeoutStop"];

  if ((stopEnd - g_stopBegin) >= stopTimeout) {
    // break
    return FISHING_STOP;
  }

  // loop
  sleepFor(100);
  return FISHING_STOP_LOOP;
}

int __stdcall GuessWasd() {
  std::lock_guard<std::mutex> locker(g_mutex);
  cv::Mat box, arrs[10];
  cv::Rect arrowRect, boxRect;
  int64 timerBegin, timerEnd;
  int timerDelay, timerLen, arrowSize;
  int x, y, color, type;
  bool ok;

  LogPrintf(L"文字辨識");

  arrowRect = g_json["ArrowRect"];
  arrowSize = g_json["ArrowSize"];
  boxRect = g_json["BoxRect"];
  timerDelay = g_json["TimerDelay"];
  timerLen = g_json["TimerLen"];

  // loop until timer bar is visible
  timerBegin = timeNow();
  ok = false;

  do {
    sleepFor(100);
    box = screenshot(boxRect);

    if (timerBar(box, timerLen, x, y)) {
      ok = true;
      break;
    }

    timerEnd = timeNow();
  } while ((timerEnd - timerBegin) < timerDelay);

  if (!ok) {
    LogPrintf(L"完美");
    return FISHING_TAKE_DROP;
  }

  arrowRect.x += x;
  arrowRect.y += y;

  for (int i = 0; i < 10; i++) {
    arrs[i] = box(arrowRect);
    arrowRect.x += arrowRect.width;
  }

  color = arrowColor(arrs[0]);

  if (color != arrowColor(arrs[1])) {
    LogPrintf(L"顏色不對");
    return FISHING_TAKE_DROP;
  }

  g_wasd.clear();

  for (int i = 0; i < 10; i++) {
    if (!arrowType(arrs[i], color, arrowSize, type)) {
      break;
    }

    g_wasd += type;
  }

  return FISHING_INPUT_TEXT;
}

int __stdcall InputText() {
  LogPrintf(L"輸入文字: %ls", g_wasd.c_str());

  for (int i = 0; i < (int)g_wasd.length(); i++) {
    keyPress(g_wasd[i]);
    sleepFor(100);
  }

  return FISHING_TAKE_DROP;
}

int __stdcall TakeDrop() {
  std::lock_guard<std::mutex> locker(g_mutex);
  cv::Mat box;
  cv::Rect boxRect, dropRect;
  bool dropGold, dropBlue, dropGreen;
  int goldHue, blueHue, greenHue, hueDif;
  int goldSat, blueSat, greenSat;
  int goldVal, blueVal, greenVal;
  int timerLen, matchRate;
  int x, y, len;
  bool take;

  LogPrintf(L"撿取物品");

  boxRect = g_json["BoxRect"];
  timerLen = g_json["TimerLen"];
  dropRect = g_json["DropRect"];
  dropGold = g_json["DropGold"];
  dropBlue = g_json["DropBlue"];
  dropGreen = g_json["DropGreen"];
  goldHue = g_json["Color"]["GoldHue"];
  blueHue = g_json["Color"]["BlueHue"];
  greenHue = g_json["Color"]["GreenHue"];
  hueDif = g_json["Color"]["HueDif"];
  goldSat = g_json["Color"]["GoldSat"];
  blueSat = g_json["Color"]["BlueSat"];
  greenSat = g_json["Color"]["GreenSat"];
  goldVal = g_json["Color"]["GoldVal"];
  blueVal = g_json["Color"]["BlueVal"];
  greenVal = g_json["Color"]["GreenVal"];
  matchRate = g_json["MatchRate"];

  // loop until timer bar is invisible
  do {
    sleepFor(100);
    box = screenshot(boxRect);
  } while (timerBar(box, timerLen, x, y));

  box = screenshot(dropRect);
  len = box.cols / 8;
  take = !dropGold && !dropBlue && !dropGreen;

  if (!take && dropGold) {
    if (matchColor(box, goldHue, hueDif, goldSat, goldVal, len)) {
      take = true;
      LogPrintf(L"金色");
    }
  }

  if (!take && dropBlue) {
    if (matchColor(box, blueHue, hueDif, blueSat, blueVal, len)) {
      take = true;
      LogPrintf(L"藍色");
    }
  }

  if (!take && dropGreen) {
    if (matchColor(box, greenHue, hueDif, greenSat, greenVal, len)) {
      take = true;
      LogPrintf(L"綠色");
    }
  }

  if (!take) {
    for (auto it = g_tmpls.begin(); it != g_tmpls.end(); it++) {
      if (matchTemplate(box, it->second, matchRate)) {
        take = true;
        LogPrintf(L"模板: %ls", it->first.c_str());
        break;
      }
    }
  }

  if (take) {
    keyPress('R');
  }

  return FISHING_RESTART;
}

int __stdcall RestartFishing() {
  std::lock_guard<std::mutex> locker(g_mutex);

  LogPrintf(L"拋竿");
  keyPress(VK_SPACE);

  return FISHING_START;
}

// dll entry

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  if (fdwReason == DLL_PROCESS_ATTACH) {
    std::ifstream ifs;
    std::vector<wchar_t> buf;
    int len;
    WIN32_FIND_DATAW find;
    HANDLE handle;
    std::wstring dir, pat;
    cv::Mat mat;

    // json path
    do {
      buf.resize(buf.size() + 1);
      len = GetModuleFileNameW(hinstDLL, buf.data(), (int)buf.size());
    } while (len >= (int)(buf.size() - 1));

    buf.resize(buf.size() + MAX_PATH);
    PathRenameExtensionW(buf.data(), L".json");
    g_inifile = buf.data();

    // load json file
    ifs.open(g_inifile);

    if (ifs.is_open()) {
      ifs >> g_json;
      ifs.close();
    }

    // template folder
    PathRemoveFileSpecW(buf.data());
    PathAppendW(buf.data(), L"Templates");
    PathAddBackslashW(buf.data());

    dir = buf.data();

    // load template images
    pat = dir + L"*.*";
    handle = FindFirstFileW(pat.c_str(), &find);

    if (handle != INVALID_HANDLE_VALUE) {
      do {
        mat = loadImage(dir + find.cFileName);

        if (!mat.empty()) {
          PathRemoveExtensionW(find.cFileName);
          g_tmpls.insert(std::make_pair(find.cFileName, mat));
        }
      } while (FindNextFileW(handle, &find));

      FindClose(handle);
    }
  } else if (fdwReason == DLL_PROCESS_DETACH) {
    std::ofstream ofs;

    // save json file
    ofs.open(g_inifile);

    if (ofs.is_open()) {
      ofs << g_json.dump(4);
      ofs.close();
    }
  }

  return TRUE;
}

// load save settings

bool __stdcall GetBoolean(const char *key) {
  std::lock_guard<std::mutex> locker(g_mutex);
  try {
    return g_json[key];
  } catch (nlohmann::json::exception &) {
  }
  return false;
}

void __stdcall SetBoolean(const char *key, bool val) {
  std::lock_guard<std::mutex> locker(g_mutex);
  g_json[key] = val;
}

int __stdcall GetInteger(const char *key) {
  std::lock_guard<std::mutex> locker(g_mutex);
  try {
    return g_json[key];
  } catch (nlohmann::json::exception &) {
  }
  return 0;
}

void __stdcall SetInteger(const char *key, int val) {
  std::lock_guard<std::mutex> locker(g_mutex);
  g_json[key] = val;
}

void __stdcall GetRect(const char *key, int *x, int *y, int *width,
                       int *height) {
  std::lock_guard<std::mutex> locker(g_mutex);
  cv::Rect rect;
  try {
    rect = g_json[key];
    *x = rect.x;
    *y = rect.y;
    *width = rect.width;
    *height = rect.height;
  } catch (nlohmann::json::exception &) {
  }
}

void __stdcall SetRect(const char *key, int x, int y, int width, int height) {
  std::lock_guard<std::mutex> locker(g_mutex);
  g_json[key] = cv::Rect(x, y, width, height);
}

BSTR __stdcall GetText(const char *key) {
  std::lock_guard<std::mutex> locker(g_mutex);
  std::string k = key;
  std::wstring v;

  if (k == "Templates") {
    for (auto it = g_tmpls.begin(); it != g_tmpls.end(); it++) {
      if (!v.empty()) v += L"\n";
      v += it->first;
    }
  } else if (k == "WASD") {
    v = g_wasd;
  }

  if (!v.empty()) {
    return SysAllocString(v.c_str());
  }

  return NULL;
}

// log

std::mutex g_logMutex;
LOG_FUNC g_logFunc;

void LogPrintf(const wchar_t *fmt, ...) {
  std::lock_guard<std::mutex> locker(g_logMutex);
  std::vector<wchar_t> buf;
  va_list ap;

  va_start(ap, fmt);
  buf.resize(_vscwprintf(fmt, ap) + 1);
  vswprintf_s(buf.data(), buf.size(), fmt, ap);
  va_end(ap);

  if (g_logFunc != NULL) {
    g_logFunc(buf.data());
  }
}

void __stdcall LogCallback(LOG_FUNC func) {
  std::lock_guard<std::mutex> locker(g_logMutex);
  g_logFunc = func;
}
