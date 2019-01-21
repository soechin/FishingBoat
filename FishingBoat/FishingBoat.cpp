#include "FishingBoat.h"
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
std::wstring g_curDir;
std::wstring g_logDir;
std::wstring g_logName;
std::wstring g_logDropsDir;
std::wstring g_logNodropsDir;
std::wstring g_tmpDir;
std::wstring g_wasd;
std::map<std::wstring, cv::Mat> g_tmpls;
std::mutex g_logMutex;
LOG_FUNC g_logFunc;
std::ofstream g_logFile;

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
  int x, y;
  bool logSlider;

  LogPrintf(L"收竿");
  g_stopBegin = stopEnd = timeNow();

  stopTimeout = g_json["TimeoutStop"];
  fishingTimeout = g_json["TimeoutFishing"];
  boxRect = g_json["BoxRect"];
  sliderDelay = g_json["SliderDelay"];
  sliderLen = g_json["SliderLen"];
  logSlider = g_json["LogSlider"];

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

  if (sliderBar(box, sliderLen, x, y)) {
    keyPress(VK_SPACE);
    LogPrintf(L"空白鍵(第2次)");

    if (logSlider) {
      cv::line(box, cv::Point(x - 5, y), cv::Point(x + 5, y),
               cv::Scalar(0, 255, 255));
      cv::line(box, cv::Point(x, y - 5), cv::Point(x, y + 5),
               cv::Scalar(0, 255, 255));

      saveImage(g_logDir, L"slider.png", box);
    }

    return FISHING_GUESS_WASD;
  }

  if (logSlider) {
    saveImage(g_logDir, L"slider.png", box);
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
  int x, y, color, type, tries;
  bool ok, logTimer;

  // loop until timer bar is visible
  timerBegin = timeNow();
  tries = 10;

OnTry:
  LogPrintf(L"文字辨識");

  arrowRect = g_json["ArrowRect"];
  arrowSize = g_json["ArrowSize"];
  boxRect = g_json["BoxRect"];
  timerDelay = g_json["TimerDelay"];
  timerLen = g_json["TimerLen"];
  logTimer = g_json["LogTimer"];

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

    if (logTimer) {
      saveImage(g_logDir, L"timer.png", box);
    }

    return FISHING_TAKE_DROP;
  }

  if (logTimer) {
    cv::line(box, cv::Point(x - 5, y), cv::Point(x + 5, y),
             cv::Scalar(0, 255, 255));
    cv::line(box, cv::Point(x, y - 5), cv::Point(x, y + 5),
             cv::Scalar(0, 255, 255));
  }

  arrowRect.x += x;
  arrowRect.y += y;

  for (int i = 0; i < 10; i++) {
    arrs[i] = box(arrowRect).clone();

    if (logTimer) {
      cv::line(box, cv::Point(arrowRect.x, arrowRect.y - 5),
               cv::Point(arrowRect.x, arrowRect.y + 5),
               cv::Scalar(0, 255, 255));
    }

    arrowRect.x += arrowRect.width;
  }

  if (logTimer) {
    cv::line(box, cv::Point(arrowRect.x, arrowRect.y - 5),
             cv::Point(arrowRect.x, arrowRect.y + 5), cv::Scalar(0, 255, 255));
    cv::line(box, cv::Point(arrowRect.x - arrowRect.width * 10, arrowRect.y),
             cv::Point(arrowRect.x, arrowRect.y), cv::Scalar(0, 255, 255));
    saveImage(g_logDir, L"timer.png", box);
  }

  color = arrowColor(arrs[0]);
  if (color != arrowColor(arrs[1])) {
    LogPrintf(L"顏色不對");

    if (--tries > 0) {
      LogPrintf(L"再試一次(%d)", tries);
      goto OnTry;
    }

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
  bool take, logDrops;

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
  logDrops = g_json["LogDrops"];

  // loop until timer bar is invisible
  do {
    sleepFor(100);
    box = screenshot(boxRect);
  } while (timerBar(box, timerLen, x, y));

  box = screenshot(dropRect);
  len = box.cols / 8;
  take = !dropGold && !dropBlue && !dropGreen;

  if (matchColor(box, goldHue, hueDif, goldSat, goldVal, len)) {
    LogPrintf(L"金色");

    if (dropGold) {
      take = true;
    }
  }

  if (matchColor(box, blueHue, hueDif, blueSat, blueVal, len)) {
    LogPrintf(L"藍色");

    if (dropBlue) {
      take = true;
    }
  }

  if (matchColor(box, greenHue, hueDif, greenSat, greenVal, len)) {
    LogPrintf(L"綠色");

    if (dropGreen) {
      take = true;
    }
  }

  if (!take) {
    for (auto it = g_tmpls.begin(); it != g_tmpls.end(); it++) {
      double val = matchTemplate(box, it->second) * 100;
      LogPrintf(L"(%.0lf%%) %ls", val, it->first.c_str());

      if (val >= matchRate) {
        take = true;
        break;
      }
    }
  }

  if (take) {
    keyPress('R');
    sleepFor(100);

    if (logDrops) {
      saveImage(g_logDropsDir, box);
    }
  } else {
    if (logDrops) {
      saveImage(g_logNodropsDir, box);
    }
  }

  return FISHING_RESTART;
}

int __stdcall RestartFishing() {
  std::lock_guard<std::mutex> locker(g_mutex);
  time_t foodUsed, tnow;
  int foodHotkey, foodTime;
  bool foodEnabled;
  double tsec;

  foodHotkey = g_json["FoodHotkey"];
  foodTime = g_json["FoodTime"];
  foodEnabled = g_json["FoodEnabled"];
  foodUsed = g_json["FoodUsed"];

  LogPrintf(L"拋竿");
  keyPress(VK_SPACE);
  sleepFor(100);

  if (foodEnabled) {
    tnow = time(NULL);
    tsec = difftime(tnow, foodUsed);

    if (tsec > foodTime) {
      g_json["FoodUsed"] = tnow;

      if (isprint(foodHotkey)) {
        LogPrintf(L"使用食物 %c", foodHotkey);
      } else {
        LogPrintf(L"使用食物 %xh", foodHotkey);
      }

      keyPress(foodHotkey);
      sleepFor(100);
    }
  }

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
    std::wstring pat;
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

    // current folder
    PathRemoveFileSpecW(buf.data());
    PathAddBackslashW(buf.data());
    g_curDir = buf.data();

    // log folder
    buf.assign(g_curDir.begin(), g_curDir.end());
    buf.resize(buf.size() + MAX_PATH);
    PathAppendW(buf.data(), L"Logs");
    PathAddBackslashW(buf.data());
    g_logDir = buf.data();
    g_logDropsDir = g_logDir + L"Drops\\";
    g_logNodropsDir = g_logDir + L"Nodrops\\";

    CreateDirectoryW(g_logDir.c_str(), NULL);
    CreateDirectoryW(g_logDropsDir.c_str(), NULL);
    CreateDirectoryW(g_logNodropsDir.c_str(), NULL);

    // template folder
    buf.assign(g_curDir.begin(), g_curDir.end());
    buf.resize(buf.size() + MAX_PATH);
    PathAppendW(buf.data(), L"Templates");
    PathAddBackslashW(buf.data());
    g_tmpDir = buf.data();

    CreateDirectoryW(g_tmpDir.c_str(), NULL);

    // load template images
    pat = g_tmpDir + L"*.*";
    handle = FindFirstFileW(pat.c_str(), &find);

    if (handle != INVALID_HANDLE_VALUE) {
      do {
        mat = loadImage(g_tmpDir + find.cFileName);

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

    // close log file
    if (g_logFile.is_open()) {
      g_logFile.close();
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

void LogPrintf(const wchar_t *fmt, ...) {
  std::lock_guard<std::mutex> locker(g_logMutex);
  std::vector<wchar_t> buf;
  std::vector<char> utf;
  std::wstring str, name;
  va_list ap;
  SYSTEMTIME st;
  int len;

  GetLocalTime(&st);
  buf.resize(128);

  swprintf_s(buf.data(), buf.size(), L"%04d-%02d-%02d.log", st.wYear, st.wMonth,
             st.wDay);
  name = buf.data();

  if (g_logName != name) {
    g_logFile.close();
    g_logFile.open(g_logDir + name, std::ios::app);
  }

  swprintf_s(buf.data(), buf.size(), L"[%02d:%02d:%02d.%03d] ", st.wHour,
             st.wMinute, st.wSecond, st.wMilliseconds);
  str += buf.data();

  va_start(ap, fmt);
  buf.resize(_vscwprintf(fmt, ap) + 1);
  vswprintf_s(buf.data(), buf.size(), fmt, ap);
  va_end(ap);
  str += buf.data();

  if (g_logFile.is_open()) {
    len = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), (int)str.length(), NULL,
                              0, NULL, NULL);
    utf.resize(len + 1);
    len = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), (int)str.length(),
                              utf.data(), (int)utf.size(), NULL, NULL);
    utf.resize(len + 1);

    g_logFile << utf.data() << std::endl;
    g_logFile.flush();
  }

  if (g_logFunc != NULL) {
    g_logFunc(str.c_str());
  }
}

void __stdcall LogCallback(LOG_FUNC func) {
  std::lock_guard<std::mutex> locker(g_logMutex);
  g_logFunc = func;
}
