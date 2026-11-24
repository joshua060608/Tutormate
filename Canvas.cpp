#include "Canvas.h"
#include <cmath>

Canvas::Canvas(HDC dc, double scale) : dc(dc), scale(scale), dy(0) {
    SetGraphicsMode(dc, GM_ADVANCED);
    SetBkMode(dc, TRANSPARENT);
    setOffset(0);
}

void Canvas::setOffset(int offY) {
    dy = offY;
    XFORM x;
    x.eM11 = static_cast<FLOAT>(scale);
    x.eM12 = 0.0f;
    x.eM21 = 0.0f;
    x.eM22 = static_cast<FLOAT>(scale);
    x.eDx = 0.0f;
    x.eDy = static_cast<FLOAT>(-offY * scale);
    SetWorldTransform(dc, &x);
}

void Canvas::fill(const RECT& r, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c);
    FillRect(dc, &r, b);
    DeleteObject(b);
}

void Canvas::round(const RECT& r, int radius, COLORREF fillColor, COLORREF border, int borderWidth) {
    HBRUSH brush = fillColor == NOCOLOR ? nullptr : CreateSolidBrush(fillColor);
    HPEN pen = border == NOCOLOR ? nullptr : CreatePen(PS_SOLID, borderWidth, border);
    HGDIOBJ ob = SelectObject(dc, brush ? static_cast<HGDIOBJ>(brush) : GetStockObject(NULL_BRUSH));
    HGDIOBJ op = SelectObject(dc, pen ? static_cast<HGDIOBJ>(pen) : GetStockObject(NULL_PEN));
    int extra = border == NOCOLOR ? 1 : 0;   // 테두리 없을 때 1px 모자라는 것 보정
    RoundRect(dc, r.left, r.top, r.right + extra, r.bottom + extra, radius * 2, radius * 2);
    SelectObject(dc, ob);
    SelectObject(dc, op);
    if (brush) DeleteObject(brush);
    if (pen) DeleteObject(pen);
}

void Canvas::dashedRound(const RECT& r, int radius, COLORREF fillColor, COLORREF border) {
    round(r, radius, fillColor);
    HPEN pen = CreatePen(PS_DASH, 1, border);
    HGDIOBJ ob = SelectObject(dc, GetStockObject(NULL_BRUSH));
    HGDIOBJ op = SelectObject(dc, pen);
    SetBkMode(dc, TRANSPARENT);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius * 2, radius * 2);
    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(pen);
}

void Canvas::hatch(const RECT& r, COLORREF color) {
    HBRUSH b = CreateHatchBrush(HS_BDIAGONAL, color);
    SetBkMode(dc, TRANSPARENT);
    FillRect(dc, &r, b);
    DeleteObject(b);
}

void Canvas::line(int x1, int y1, int x2, int y2, COLORREF c, int w) {
    HPEN pen = CreatePen(PS_SOLID, w, c);
    HGDIOBJ op = SelectObject(dc, pen);
    MoveToEx(dc, x1, y1, nullptr);
    LineTo(dc, x2, y2);
    SelectObject(dc, op);
    DeleteObject(pen);
}

void Canvas::circle(int cx, int cy, int r, COLORREF fillColor, COLORREF border) {
    HBRUSH brush = fillColor == NOCOLOR ? nullptr : CreateSolidBrush(fillColor);
    HPEN pen = border == NOCOLOR ? nullptr : CreatePen(PS_SOLID, 1, border);
    HGDIOBJ ob = SelectObject(dc, brush ? static_cast<HGDIOBJ>(brush) : GetStockObject(NULL_BRUSH));
    HGDIOBJ op = SelectObject(dc, pen ? static_cast<HGDIOBJ>(pen) : GetStockObject(NULL_PEN));
    Ellipse(dc, cx - r, cy - r, cx + r + 1, cy + r + 1);
    SelectObject(dc, ob);
    SelectObject(dc, op);
    if (brush) DeleteObject(brush);
    if (pen) DeleteObject(pen);
}

// 도넛 차트 한 조각: from, to는 0~1 (12시 방향에서 시계 방향)
void Canvas::ring(int cx, int cy, int r, int thickness, double from, double to, COLORREF c) {
    if (to - from < 0.002) return;
    LOGBRUSH lb;
    lb.lbStyle = BS_SOLID;
    lb.lbColor = c;
    lb.lbHatch = 0;
    HPEN pen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT, static_cast<DWORD>(thickness), &lb, 0, nullptr);
    HGDIOBJ op = SelectObject(dc, pen);
    HGDIOBJ ob = SelectObject(dc, GetStockObject(NULL_BRUSH));
    if (to - from >= 0.999) {
        Ellipse(dc, cx - r, cy - r, cx + r, cy + r);
    } else {
        const double PI = 3.14159265358979;
        double a1 = 2 * PI * from, a2 = 2 * PI * to;
        int x1 = cx + static_cast<int>(std::lround(r * 2 * std::sin(a1)));
        int y1 = cy - static_cast<int>(std::lround(r * 2 * std::cos(a1)));
        int x2 = cx + static_cast<int>(std::lround(r * 2 * std::sin(a2)));
        int y2 = cy - static_cast<int>(std::lround(r * 2 * std::cos(a2)));
        SetArcDirection(dc, AD_CLOCKWISE);
        Arc(dc, cx - r, cy - r, cx + r, cy + r, x1, y1, x2, y2);
    }
    SelectObject(dc, op);
    SelectObject(dc, ob);
    DeleteObject(pen);
}

void Canvas::text(const RECT& r, const std::wstring& s, HFONT f, COLORREF c, UINT flags) {
    HGDIOBJ of = SelectObject(dc, f);
    SetTextColor(dc, c);
    RECT rr = r;
    UINT fl = flags | DT_NOPREFIX;
    if (fl & DT_SINGLELINE) fl |= DT_END_ELLIPSIS;
    DrawTextW(dc, s.c_str(), -1, &rr, fl);
    SelectObject(dc, of);
}

int Canvas::textWidth(const std::wstring& s, HFONT f) {
    HGDIOBJ of = SelectObject(dc, f);
    SIZE sz{0, 0};
    GetTextExtentPoint32W(dc, s.c_str(), static_cast<int>(s.size()), &sz);
    SelectObject(dc, of);
    return sz.cx;
}

void Canvas::check(int x, int y, bool on) {
    if (on) {
        round(R(x, y, 18, 18), 4, RGB(37, 99, 235));
        line(x + 4, y + 9, x + 8, y + 13, RGB(255, 255, 255), 2);
        line(x + 8, y + 13, x + 14, y + 5, RGB(255, 255, 255), 2);
    } else {
        round(R(x, y, 18, 18), 4, RGB(255, 255, 255), RGB(156, 163, 175), 1);
    }
}
