#include "FishingBoat.h"

void sleepFor(int ms) {
  if (ms > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  } else {
    std::this_thread::yield();
  }
}

int64 timeNow() {
  auto a = std::chrono::high_resolution_clock::now();
  auto b = a.time_since_epoch();
  auto c = std::chrono::duration_cast<std::chrono::milliseconds>(b);
  return c.count();
}
