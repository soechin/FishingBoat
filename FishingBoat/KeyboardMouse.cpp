#include "FishingBoat.h"

void keyDown(int vk) {
  INPUT input;

  memset(&input, 0, sizeof(input));
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = vk;
  input.ki.wScan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
  input.ki.dwFlags = 0;

  SendInput(1, &input, sizeof(input));
}

void keyUp(int vk) {
  INPUT input;

  memset(&input, 0, sizeof(input));
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = vk;
  input.ki.wScan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
  input.ki.dwFlags = KEYEVENTF_KEYUP;

  SendInput(1, &input, sizeof(input));
}

void keyPress(int vk, int ms) {
  keyDown(vk);
  sleepFor(ms);
  keyUp(vk);
}

bool cursorShowing() {
  CURSORINFO ci;

  memset(&ci, 0, sizeof(ci));
  ci.cbSize = sizeof(ci);

  if (GetCursorInfo(&ci)) {
    if ((ci.flags & CURSOR_SHOWING) != 0) {
      return true;
    }
  }

  return false;
}
