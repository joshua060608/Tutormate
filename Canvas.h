#pragma once
#include <string>
#include "Win32Util.h"

const COLORREF NOCOLOR = 0xFFFFFFFF;

inline RECT R(int x, int y, int w, int h) {
    RECT r{x, y, x + w, y + h};
    return r;
}

// GDI 그리기 도우미. 96 DPI 기준 좌표로 그리면 화면 배율에 맞춰 자동 확대된다.
class Canvas {
private:
    HDC dc;
    double scale;
    int dy;   // 스크롤 오프셋

public:
    Canvas(HDC dc, double scale);

    HDC hdc() const { return dc; }
    int offset() const { return dy; }
    void setOffset(int offY);

    void fill(const RECT& r, COLORREF c);
    void round(const RECT& r, int radius, COLORREF fillColor, COLORREF border = NOCOLOR, int borderWidth = 1);
    void dashedRound(const RECT& r, int radius, COLORREF fillColor, COLORREF border);
    void hatch(const RECT& r, COLORREF color);
    void line(int x1, int y1, int x2, int y2, COLORREF c, int w = 1);
    void circle(int cx, int cy, int r, COLORREF fillColor, COLORREF border = NOCOLOR);
    void ring(int cx, int cy, int r, int thickness, double from, double to, COLORREF c);
    void text(const RECT& r, const std::wstring& s, HFONT f, COLORREF c,
              UINT flags = DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    int textWidth(const std::wstring& s, HFONT f);
    void check(int x, int y, bool on);   // 18x18 체크박스
};
