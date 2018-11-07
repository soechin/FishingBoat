﻿#include "FishingBoat.h"

cv::Mat loadImage(std::wstring file) {
  std::ifstream ifs;
  std::vector<uchar> buf;

  ifs.open(file, std::ios::binary);

  if (ifs.is_open()) {
    ifs.seekg(0, std::ios::end);
    buf.resize((int)ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    ifs.read((char *)buf.data(), (int)buf.size());
    ifs.close();
  }

  if (!buf.empty()) {
    return cv::imdecode(buf, cv::IMREAD_COLOR);
  }

  return cv::Mat();
}

cv::Mat screenshot(cv::Rect roi) {
  cv::Mat mat;
  BITMAPINFO bmi;
  BITMAPINFOHEADER &bmih(bmi.bmiHeader);
  BYTE *src, *dst;
  HWND hwnd;
  HDC hdc, mdc;
  HGDIOBJ dib, obj;
  RECT rect;
  POINT point;
  int x, y, width, height, step;
  void *data;

  hwnd = GetForegroundWindow();

  if (::GetClientRect(hwnd, &rect)) {
    point.x = rect.left;
    point.y = rect.top;

    if (::ClientToScreen(hwnd, &point)) {
      roi.x += point.x;
      roi.y += point.y;
    }
  }

  x = roi.x;
  y = roi.y;
  width = roi.width;
  height = roi.height;
  step = (width * 3 + 3) & (-4);

  memset(&bmih, 0, sizeof(bmih));
  bmih.biSize = sizeof(bmih);
  bmih.biWidth = width;
  bmih.biHeight = height;
  bmih.biPlanes = 1;
  bmih.biBitCount = 24;
  bmih.biCompression = BI_RGB;
  bmih.biSizeImage = step * height;

  hdc = GetDC(NULL);
  mdc = CreateCompatibleDC(hdc);
  dib = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &data, NULL, 0);
  obj = SelectObject(mdc, dib);

  BitBlt(mdc, 0, 0, width, height, hdc, x, y, SRCCOPY);

  mat.create(cv::Size(width, height), CV_8UC3);
  src = (BYTE *)data + step * (height - 1);
  dst = (BYTE *)mat.data;

  for (int j = 0; j < height; j++) {
    memcpy(dst, src, width * 3);
    src -= step;
    dst += mat.step;
  }

  SelectObject(mdc, obj);
  DeleteObject(dib);
  DeleteDC(mdc);
  ReleaseDC(NULL, hdc);

  return mat;
}

bool sliderBar(cv::Mat box, int len) {
  int mid = box.cols / 2;

  for (int j = 0; j < (box.rows - 1); j++) {
    // R R R R R R ? ? ?
    // . . . . . . ? ? ? W W W W
    uchar *p = box.ptr(j);
    uchar *q = box.ptr(j + 1);
    int left = mid;
    int right = mid - 1;

    // find left edge
    for (int i = mid - 1; i >= 0; i--) {
      int b = p[i * 3 + 0];
      int g = p[i * 3 + 1];
      int r = p[i * 3 + 2];

      // red color
      if (b <= 50 && g <= 50 && r >= 200) {
        left = i;
      } else {
        break;
      }
    }

    // find right edge
    for (int i = mid; i < box.cols; i++) {
      int b = p[i * 3 + 0];
      int g = p[i * 3 + 1];
      int r = p[i * 3 + 2];

      // red color
      if (b <= 50 && g <= 50 && r >= 200) {
        right = i;
      } else {
        break;
      }
    }

    // skip some pixels and find right edge
    for (int i = (left + right + len) / 2; i < box.cols; i++) {
      int b = q[i * 3 + 0];
      int g = q[i * 3 + 1];
      int r = q[i * 3 + 2];

      // white color
      if (b >= 200 && g >= 200 && r >= 200) {
        right = i;
      } else {
        break;
      }
    }

    if ((right - left + 1) >= len) {
      return true;
    }
  }

  return false;
}

bool timerBar(cv::Mat box, int len, int &x, int &y) {
  int mid = box.cols / 2;

  for (int j = 0; j < (box.rows - 1); j++) {
    // W W W W W
    // B B B B B
    uchar *p = box.ptr(j);
    uchar *q = box.ptr(j + 1);
    int left = mid;
    int right = mid - 1;

    for (int i = mid; i < box.cols; i++) {
      int b1 = p[i * 3 + 0];
      int g1 = p[i * 3 + 1];
      int r1 = p[i * 3 + 2];
      int b2 = q[i * 3 + 0];
      int g2 = q[i * 3 + 1];
      int r2 = q[i * 3 + 2];

      if ((b1 >= 200 && g1 >= 200 && r1 >= 200) &&
          (b2 <= 50 && g2 <= 50 && r2 <= 50)) {
        right = i;
      } else {
        break;
      }
    }

    for (int i = mid - 1; i >= 0; i--) {
      int b1 = p[i * 3 + 0];
      int g1 = p[i * 3 + 1];
      int r1 = p[i * 3 + 2];
      int b2 = q[i * 3 + 0];
      int g2 = q[i * 3 + 1];
      int r2 = q[i * 3 + 2];

      if ((b1 >= 200 && g1 >= 200 && r1 >= 200) &&
          (b2 <= 50 && g2 <= 50 && r2 <= 50)) {
        left = i;
      } else {
        break;
      }
    }

    if ((right - left + 1) >= len) {
      x = left;
      y = j;
      return true;
    }
  }

  return false;
}

int arrowColor(cv::Mat arr) {
  std::map<int, int> hist;
  int color, num;

  for (int j = 0; j < arr.rows; j++) {
    uchar *p = arr.ptr(j);

    for (int i = 0; i < arr.cols; i++) {
      int b = p[i * 3 + 0];
      int g = p[i * 3 + 1];
      int r = p[i * 3 + 2];
      int n = RGB(r, g, b);
      hist[n]++;
    }
  }

  color = 0;
  num = 0;

  for (auto it = hist.begin(); it != hist.end(); it++) {
    if (it->second > num) {
      color = it->first;
      num = it->second;
    }
  }

  return color;
}

bool arrowType(cv::Mat arr, int color, double size, int &type) {
  std::vector<std::vector<cv::Point>> conts;
  std::vector<cv::Point2f> pts;
  cv::Mat bin;
  int idx;
  double area, mindx, mindy;

  bin.create(arr.size(), CV_8UC1);

  for (int j = 0; j < arr.rows; j++) {
    uchar *p = arr.ptr(j);
    uchar *q = bin.ptr(j);

    for (int i = 0; i < arr.cols; i++) {
      int b = p[i * 3 + 0];
      int g = p[i * 3 + 1];
      int r = p[i * 3 + 2];
      int n = RGB(r, g, b);
      q[i] = (n == color) ? 255 : 0;
    }
  }

  cv::findContours(bin, conts, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
  idx = 0;
  area = 0;

  for (int i = 0; i < (int)conts.size(); i++) {
    double a = cv::contourArea(conts[i]);

    if (a > area) {
      idx = i;
      area = a;
    }
  }

  if (conts.empty() || area < size) {
    return false;
  }

  cv::minEnclosingTriangle(conts[idx], pts);
  if (pts.size() != 3) return false;

  idx = 0;
  mindx = FLT_MAX;
  mindy = FLT_MAX;

  for (int i = 0; i < 3; i++) {
    cv::Point2f &a = pts[i];
    cv::Point2f &b = pts[(i + 1) % 3];
    float dx = abs(a.x - b.x);
    float dy = abs(a.y - b.y);

    if (dx < mindx && dx < mindy) {
      idx = i;
      mindx = dx;
    }

    if (dy < mindx && dy < mindy) {
      idx = i;
      mindy = dy;
    }
  }

  if (mindx < mindy) {
    cv::Point2f &a = pts[idx];
    cv::Point2f &b = pts[(idx + 2) % 3];
    type = b.x < a.x ? 'A' : 'D';
  } else {
    cv::Point2f &a = pts[idx];
    cv::Point2f &b = pts[(idx + 2) % 3];
    type = b.y < a.y ? 'W' : 'S';
  }

  return true;
}

bool matchColor(cv::Mat mat, int hue, int dif, int sat, int val, int len) {
  cv::Mat chnls[3], hsv, bin;
  int huel, hueh, num;

  cv::cvtColor(mat, hsv, cv::COLOR_BGR2HSV);
  cv::split(hsv, chnls);

  huel = (hue - dif + 180) % 180;
  hueh = (hue + dif + 180) % 180;

  if (huel < hueh) {
    bin = chnls[0] >= huel & chnls[0] <= hueh;
  } else {
    bin = chnls[0] >= huel | chnls[0] <= hueh;
  }

  bin &= chnls[1] >= sat & chnls[2] >= val;
  num = 0;

  for (int j = 0; j < bin.rows; j++) {
    uchar *p = bin.ptr(j);
    int n = 0;

    for (int i = 0; i < (bin.cols - len); i++) {
      if (p[i] >= 128) {
        if ((++n) >= len) {
          num += 1;
          j += len / 2;
          break;
        }
      } else {
        n = 0;
      }
    }

    if (num >= 2) {
      return true;
    }
  }

  return false;
}

bool matchTemplate(cv::Mat mat, cv::Mat tmp, int thr) {
  cv::Mat ret;
  double val;

  cv::matchTemplate(mat, tmp, ret, cv::TM_CCORR_NORMED);
  cv::minMaxLoc(ret, NULL, &val, NULL, NULL);

  if ((val * 100) >= thr) {
    return true;
  }

  return false;
}