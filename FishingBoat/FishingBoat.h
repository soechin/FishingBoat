#pragma once

// stl
#include <chrono>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
// opencv
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
// windows
#include <windows.h>
// shlwapi
#include <shlwapi.h>

// timing control

void sleepFor(int ms);
int64 timeNow();

// keyboard and mouse simulation

void keyDown(int vk);
void keyUp(int vk);
void keyPress(int vk, int ms = 50);
bool cursorShowing();

// image process

cv::Mat loadImage(std::wstring file);
void saveImage(std::wstring dir, cv::Mat mat);
cv::Mat screenshot(cv::Rect roi);
bool sliderBar(cv::Mat box, int len);
bool timerBar(cv::Mat box, int len, int &x, int &y);
int arrowColor(cv::Mat arr);
bool arrowType(cv::Mat arr, int color, double size, int &type);
bool matchColor(cv::Mat mat, int hue, int dif, int sat, int val, int len);
double matchTemplate(cv::Mat mat, cv::Mat tmp);

// running steps

int __stdcall StartFishing();
int __stdcall StartLoop();
int __stdcall StopFishing();
int __stdcall StopLoop();
int __stdcall GuessWasd();
int __stdcall InputText();
int __stdcall TakeDrop();
int __stdcall RestartFishing();

// dll entry

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

// load save settings

bool __stdcall GetBoolean(const char *key);
void __stdcall SetBoolean(const char *key, bool val);
int __stdcall GetInteger(const char *key);
void __stdcall SetInteger(const char *key, int val);
void __stdcall GetRect(const char *key, int *x, int *y, int *width,
                       int *height);
void __stdcall SetRect(const char *key, int x, int y, int width, int height);
BSTR __stdcall GetText(const char *key);

// log

typedef void(__stdcall* LOG_FUNC)(const wchar_t* str);
void LogPrintf(const wchar_t *fmt, ...);
void __stdcall LogCallback(LOG_FUNC func);
