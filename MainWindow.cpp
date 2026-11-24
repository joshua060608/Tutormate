#include "MainWindow.h"
#include <algorithm>
#include <cstdio>
#include <cwchar>
#include <set>
#include "FormDialog.h"

namespace {
// ---------------- 색상 ----------------
const COLORREF BG = RGB(245, 247, 251);
const COLORREF WHITE = RGB(255, 255, 255);
const COLORREF BORDER = RGB(229, 231, 235);
const COLORREF LINEC = RGB(238, 240, 244);
const COLORREF INK = RGB(17, 24, 39);
const COLORREF SUB = RGB(107, 114, 128);
const COLORREF MUTED = RGB(156, 163, 175);
const COLORREF BLUE = RGB(37, 99, 235);
const COLORREF BLUE_L = RGB(232, 240, 254);
const COLORREF BLUE_XL = RGB(243, 247, 255);
const COLORREF GREEN = RGB(22, 163, 74);
const COLORREF GREEN_L = RGB(220, 252, 231);
const COLORREF RED = RGB(220, 38, 38);
const COLORREF RED_L = RGB(254, 226, 226);
const COLORREF PURPLE = RGB(124, 58, 237);
const COLORREF PURPLE_L = RGB(237, 233, 254);
const COLORREF YELLOW = RGB(180, 120, 4);
const COLORREF YELLOW_L = RGB(254, 249, 219);
const COLORREF YELLOW_B = RGB(234, 179, 8);
const COLORREF GRAY_L = RGB(243, 244, 246);
const COLORREF GRAY_M = RGB(209, 213, 219);
const COLORREF PINK_L = RGB(253, 236, 240);

struct Pal {
    COLORREF bg, border, text;
};
const Pal PAL[6] = {
    {RGB(219, 234, 254), RGB(147, 197, 253), RGB(30, 64, 175)},
    {RGB(220, 252, 231), RGB(134, 239, 172), RGB(21, 128, 61)},
    {RGB(252, 231, 243), RGB(249, 168, 212), RGB(190, 24, 93)},
    {RGB(237, 233, 254), RGB(196, 181, 253), RGB(109, 40, 217)},
    {RGB(255, 237, 213), RGB(253, 186, 116), RGB(194, 65, 12)},
    {RGB(204, 251, 241), RGB(94, 234, 212), RGB(15, 118, 110)},
};
const COLORREF DAY_TINT[7] = {RGB(239, 246, 255), RGB(240, 253, 244), RGB(236, 254, 255), RGB(254, 252, 232),
                              RGB(254, 242, 242), RGB(246, 247, 249), RGB(246, 247, 249)};

Pal categoryPal(const std::string& cat) {
    if (cat == "독서") return PAL[1];
    if (cat == "문학") return PAL[2];
    if (cat == "언매") return PAL[0];
    if (cat == "실전") return PAL[3];
    return Pal{GRAY_L, GRAY_M, SUB};
}

// ---------------- 레이아웃 ----------------
const int SB = 220;          // 사이드바 너비
const int X0 = SB + 32;      // 본문 시작 x
const int TOP = 108;         // 본문 카드 시작 y

// 버튼 스타일
enum Style { S_PRIMARY, S_SECONDARY, S_T_BLUE, S_T_YELLOW, S_T_GRAY, S_T_GREEN, S_T_PURPLE, S_T_RED, S_DISABLED };

// 클릭 동작
enum Act {
    A_NAV = 1, A_CUR_ADD, A_CUR_MENU, A_PLAN_PICK, A_PLAN_PREV, A_PLAN_NEXT, A_PLAN_EDIT_STUDENT, A_PLAN_GEN,
    A_PLAN_ASSIGN, A_PLAN_RESET, A_PLAN_CUR, A_LESSON_MENU, A_PLAN_ADD_LESSON, A_STU_ADD, A_STU_EDIT, A_STU_DEL,
    A_STU_PLAN, A_WEEK_PREV, A_WEEK_NEXT, A_WEEK_TODAY, A_CAL_PREV, A_CAL_NEXT, A_CAL_DAY, A_SELECT_LESSON,
    A_CELL_ADD, A_QUICK, A_TASK_TOGGLE, A_TASK_MENU, A_PREP_REGEN, A_PREP_SETTINGS, A_PREP_ROLLOVER, A_SET_WEEKEND,
    A_SET_TODAY, A_SET_REAL, A_SET_SAMPLE, A_SET_CLEAR, A_SET_SAVE
};

Date fromSerial(long s) { return Date(1970, 1, 1).addDays(static_cast<int>(s)); }

std::wstring num(int n) { return std::to_wstring(n); }

std::wstring md(const Date& d) { return W(d.shortStr()); }   // 11/26

std::wstring mdw(const Date& d) { return W(d.shortStr() + " (" + d.weekdayName() + ")"); }

std::wstring hourW(int h) {
    wchar_t buf[16];
    std::swprintf(buf, 16, L"%02d:00", h);
    return buf;
}

std::wstring longDate(const Date& d) {
    return num(d.year()) + L"년 " + num(d.month()) + L"월 " + num(d.day()) + L"일 (" + W(d.weekdayName()) + L")";
}

std::wstring weekLabel(const Date& ws) {
    Date th = ws.addDays(3);   // 목요일이 속한 달 기준
    int wk = (th.day() - 1) / 7 + 1;
    return num(th.year()) + L"년 " + num(th.month()) + L"월 " + num(wk) + L"주차";
}

std::vector<std::wstring> hourOptions() {
    std::vector<std::wstring> v;
    for (int h = 6; h <= 23; ++h) v.push_back(hourW(h));
    return v;
}
std::vector<std::wstring> lengthOptions() { return {L"1시간", L"2시간", L"3시간", L"4시간"}; }
int clampi(int v, int lo, int hi) { return std::max(lo, std::min(hi, v)); }
}  // namespace

// ======================================================================
//  창 생성 / 메시지 처리
// ======================================================================
MainWindow::MainWindow(std::string path) : dataPath(std::move(path)) {
    viewWeek = app.today().weekStart();
}

int MainWindow::run(HINSTANCE inst) {
    SetProcessDPIAware();
    MainWindow w("tutormate_data.txt");

    HDC sdc = GetDC(nullptr);
    w.scale = GetDeviceCaps(sdc, LOGPIXELSX) / 96.0;
    ReleaseDC(nullptr, sdc);
    if (w.scale < 1.0) w.scale = 1.0;
    w.createFonts();

    std::string err;
    int loaded = w.app.load(w.dataPath, err);
    w.viewWeek = w.app.today().weekStart();
    w.calYear = w.app.today().year();
    w.calMonth = w.app.today().month();

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = wndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"TutorMateMain";
    RegisterClassExW(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    int ww = std::min(static_cast<int>(1400 * w.scale), sw * 94 / 100);
    int wh = std::min(static_cast<int>(900 * w.scale), sh * 90 / 100);
    HWND h = CreateWindowExW(0, L"TutorMateMain", L"TutorMate - Plan Better, Teach Better", WS_OVERLAPPEDWINDOW,
                             (sw - ww) / 2, (sh - wh) / 3, ww, wh, nullptr, nullptr, inst, &w);
    if (!h) return 1;
    ShowWindow(h, SW_SHOW);
    UpdateWindow(h);

    if (loaded == -1) {
        w.info(L"저장 파일을 읽지 못했습니다.\n" + W(err) + L"\n빈 데이터로 시작합니다.");
        w.app.clearAll();
    } else if (loaded == 0) {
        if (w.ask(L"TutorMate에 오신 것을 환영합니다!\n\n샘플 데이터(커리큘럼 14개, 학생 4명)를 불러와서 둘러볼까요?")) {
            w.app.loadSampleData();
            w.viewWeek = w.app.today().weekStart();
        }
        w.refresh();
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    w.deleteFonts();
    return 0;
}

void MainWindow::createFonts() {
    auto mk = [](int px, bool bold) {
        return CreateFontW(-px, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                           L"Malgun Gothic");
    };
    f.logo = mk(21, true);
    f.h1 = mk(26, true);
    f.h2 = mk(17, true);
    f.body = mk(14, false);
    f.bodyB = mk(14, true);
    f.sm = mk(13, false);
    f.smB = mk(13, true);
    f.big = mk(28, true);
    f.tiny = mk(12, false);
    f.ctrl = mk(static_cast<int>(14 * scale), false);   // 입력 창 컨트롤용 (배율 직접 적용)
}

void MainWindow::deleteFonts() {
    for (HFONT x : {f.logo, f.h1, f.h2, f.body, f.bodyB, f.sm, f.smB, f.big, f.tiny, f.ctrl})
        if (x) DeleteObject(x);
}

LRESULT CALLBACK MainWindow::wndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        MainWindow* self = static_cast<MainWindow*>(cs->lpCreateParams);
        self->hwnd = h;
        SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    MainWindow* self = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(h, GWLP_USERDATA));
    if (self) return self->handle(msg, wp, lp);
    return DefWindowProcW(h, msg, wp, lp);
}

LRESULT MainWindow::handle(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_PAINT:
            paint();
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_SIZE:
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mm = reinterpret_cast<MINMAXINFO*>(lp);
            mm->ptMinTrackSize.x = static_cast<LONG>(1150 * scale);
            mm->ptMinTrackSize.y = static_cast<LONG>(700 * scale);
            return 0;
        }
        case WM_MOUSEWHEEL: {
            int delta = static_cast<short>(HIWORD(wp));
            scrollY = std::max(0, scrollY - delta * 70 / 120);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        case WM_LBUTTONDOWN:
            onClick(static_cast<short>(LOWORD(lp)), static_cast<short>(HIWORD(lp)));
            return 0;
        case WM_KEYDOWN:
            if (page == Page::Weekly || page == Page::Prep) {
                if (wp == VK_LEFT) dispatch(A_WEEK_PREV, 0, 0);
                if (wp == VK_RIGHT) dispatch(A_WEEK_NEXT, 0, 0);
            }
            if (wp == VK_PRIOR || wp == VK_NEXT) {
                scrollY = std::max(0, scrollY + (wp == VK_NEXT ? 400 : -400));
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        case WM_SETCURSOR:
            if (LOWORD(lp) == HTCLIENT) {
                POINT p;
                GetCursorPos(&p);
                ScreenToClient(hwnd, &p);
                if (hit(p.x, p.y)) {
                    SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32649)));
                    return TRUE;
                }
                SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32512)));
                return TRUE;
            }
            break;
        case WM_CLOSE:
            app.save(dataPath);
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void MainWindow::paint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);
    int pw = rc.right, ph = rc.bottom;
    if (pw <= 0 || ph <= 0) {
        EndPaint(hwnd, &ps);
        return;
    }
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, pw, ph);
    HGDIOBJ oldBmp = SelectObject(mem, bmp);
    viewW = static_cast<int>(pw / scale);
    viewH = static_cast<int>(ph / scale);
    zones.clear();
    {
        Canvas c(mem, scale);
        cv = &c;
        c.fill(R(0, 0, viewW + 2, viewH + 2), BG);

        // 본문 (스크롤 적용 + 사이드바 영역 잘라내기)
        int saved = SaveDC(mem);
        c.setOffset(scrollY);
        IntersectClipRect(mem, SB, scrollY, viewW + 2, scrollY + viewH + 2);
        contentBottom = 0;
        switch (page) {
            case Page::Dashboard: drawDashboard(); break;
            case Page::Curriculum: drawCurriculum(); break;
            case Page::Students: drawStudents(); break;
            case Page::Weekly: drawWeekly(); break;
            case Page::Prep: drawPrep(); break;
            case Page::Stats: drawStats(); break;
            case Page::Settings: drawSettings(); break;
        }
        RestoreDC(mem, saved);
        c.setOffset(0);

        int maxScroll = std::max(0, contentBottom + 32 - viewH);
        if (scrollY > maxScroll) {
            scrollY = maxScroll;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        if (maxScroll > 0) {   // 스크롤 위치 표시
            int barH = std::max(40, viewH * viewH / (contentBottom + 32));
            int barY = (viewH - barH) * scrollY / maxScroll;
            c.round(R(viewW - 8, barY + 2, 5, barH - 4), 2, GRAY_M);
        }
        drawSidebar();
        cv = nullptr;
    }
    ModifyWorldTransform(mem, nullptr, MWT_IDENTITY);
    BitBlt(hdc, 0, 0, pw, ph, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldBmp);
    DeleteObject(bmp);
    DeleteDC(mem);
    EndPaint(hwnd, &ps);
}

const MainWindow::Zone* MainWindow::hit(int x, int y) const {
    int lx = static_cast<int>(x / scale), ly = static_cast<int>(y / scale);
    for (auto it = zones.rbegin(); it != zones.rend(); ++it)   // 나중에 그린 것이 위
        if (lx >= it->r.left && lx < it->r.right && ly >= it->r.top && ly < it->r.bottom) return &*it;
    return nullptr;
}

void MainWindow::onClick(int x, int y) {
    const Zone* z = hit(x, y);
    if (!z) return;
    Zone copy = *z;   // dispatch 중에 zones가 바뀔 수 있으므로 복사
    dispatch(copy.act, copy.a, copy.b);
}

void MainWindow::refresh() {
    app.save(dataPath);
    InvalidateRect(hwnd, nullptr, FALSE);
}

void MainWindow::go(Page p) {
    if (page != p) scrollY = 0;
    page = p;
    InvalidateRect(hwnd, nullptr, FALSE);
}

// ======================================================================
//  그리기 도우미
// ======================================================================
void MainWindow::zone(const RECT& r, int act, int a, int b) {
    int dy = cv ? cv->offset() : 0;
    RECT v{r.left, r.top - dy, r.right, r.bottom - dy};
    zones.push_back(Zone{v, act, a, b});
}

void MainWindow::card(const RECT& r) {
    cv->round(R(r.left, r.top + 2, r.right - r.left, r.bottom - r.top), 12, RGB(236, 239, 244));   // 그림자
    cv->round(r, 12, WHITE, BORDER);
    contentBottom = std::max(contentBottom, static_cast<int>(r.bottom));
}

void MainWindow::header(const std::wstring& title, const std::wstring& sub) {
    cv->text(R(X0, 24, 700, 38), title, f.h1, INK);
    cv->text(R(X0, 62, viewW - X0 - 420, 24), sub, f.body, SUB);
}

void MainWindow::button(const RECT& r, const std::wstring& label, int style, int act, int a, int b) {
    COLORREF bg = WHITE, fg = INK, bd = BORDER;
    switch (style) {
        case S_PRIMARY: bg = BLUE; fg = WHITE; bd = NOCOLOR; break;
        case S_SECONDARY: bg = WHITE; fg = INK; bd = GRAY_M; break;
        case S_T_BLUE: bg = BLUE_L; fg = BLUE; bd = NOCOLOR; break;
        case S_T_YELLOW: bg = YELLOW_L; fg = YELLOW; bd = NOCOLOR; break;
        case S_T_GRAY: bg = GRAY_L; fg = SUB; bd = NOCOLOR; break;
        case S_T_GREEN: bg = GREEN_L; fg = GREEN; bd = NOCOLOR; break;
        case S_T_PURPLE: bg = PURPLE_L; fg = PURPLE; bd = NOCOLOR; break;
        case S_T_RED: bg = RED_L; fg = RED; bd = NOCOLOR; break;
        default: bg = GRAY_L; fg = MUTED; bd = NOCOLOR; break;
    }
    cv->round(r, 8, bg, bd);
    cv->text(r, label, f.bodyB, fg, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (style != S_DISABLED) zone(r, act, a, b);
}

int MainWindow::pill(int x, int y, const std::wstring& s, COLORREF bg, COLORREF fg, HFONT font) {
    HFONT ft = font ? font : f.smB;
    int w = cv->textWidth(s, ft) + 20;
    cv->round(R(x, y, w, 24), 12, bg);
    cv->text(R(x, y, w, 24), s, ft, fg, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    return w;
}

void MainWindow::donut(int cx, int cy, int r, int done, int total, const std::wstring& caption) {
    cv->ring(cx, cy, r, 16, 0.0, 1.0, GRAY_L);
    if (total > 0) cv->ring(cx, cy, r, 16, 0.0, static_cast<double>(done) / total, GREEN);
    cv->text(R(cx - r, cy - 22, 2 * r, 24), num(done) + L" / " + num(total), f.h2, INK,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    int pct = total > 0 ? done * 100 / total : 0;
    cv->text(R(cx - r, cy + 2, 2 * r, 20), total > 0 ? num(pct) + L"%" : caption, f.smB, GREEN,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void MainWindow::progress(const RECT& r, int done, int total, COLORREF c) {
    cv->round(r, 4, GRAY_L);
    if (total > 0 && done > 0) {
        int w = (r.right - r.left) * std::min(done, total) / total;
        cv->round(R(r.left, r.top, std::max(8, w), r.bottom - r.top), 4, c);
    }
}

// 미니 달력: forPlan이면 선택한 학생의 수업일, 아니면 보고 있는 주를 강조
void MainWindow::calendar(const RECT& r, bool forPlan) {
    card(r);
    int x = r.left, y = r.top, w = r.right - r.left;
    cv->text(R(x + 18, y + 14, w - 100, 26), num(calYear) + L"년 " + num(calMonth) + L"월", f.h2, INK);
    cv->text(R(x + w - 74, y + 14, 26, 26), L"<", f.h2, SUB, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    zone(R(x + w - 74, y + 14, 26, 26), A_CAL_PREV);
    cv->text(R(x + w - 42, y + 14, 26, 26), L">", f.h2, SUB, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    zone(R(x + w - 42, y + 14, 26, 26), A_CAL_NEXT);

    const int cw = (w - 24) / 7, ch = 30, gx = x + 12, gy = y + 50;
    for (int i = 0; i < 7; ++i)
        cv->text(R(gx + i * cw, gy, cw, 20), W(Date::weekdayName(i)), f.sm, i == 6 ? RED : (i == 5 ? BLUE : SUB),
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    std::set<long> marked;
    Date target(1900, 1, 1);
    if (forPlan && app.findStudent(selStudent)) {
        for (const Lesson* l : app.lessonsOf(selStudent, false)) marked.insert(l->getDate().serial());
        target = app.findStudent(selStudent)->getTargetDate();
    }
    const Date first(calYear, calMonth, 1);
    const int offset = first.weekday();
    const int days = Date::daysInMonth(calYear, calMonth);
    const Date today = app.today();
    const Date ws = viewWeek, we = viewWeek.addDays(6);
    for (int d = 1; d <= days; ++d) {
        int idx = offset + d - 1;
        int col = idx % 7, row = idx / 7;
        Date day(calYear, calMonth, d);
        RECT cell = R(gx + col * cw, gy + 24 + row * ch, cw, ch);
        if (!forPlan && day >= ws && day <= we) cv->fill(R(cell.left, cell.top + 2, cw, ch - 4), BLUE_L);
        COLORREF tc = col == 6 ? RED : INK;
        int cx = cell.left + cw / 2, cy = cell.top + ch / 2;
        if (day == today) {
            cv->circle(cx, cy, 12, BLUE);
            tc = WHITE;
        } else if (marked.count(day.serial())) {
            cv->circle(cx, cy, 12, BLUE_L);
            tc = BLUE;
        }
        if (day == target) {
            cv->circle(cx, cy, 12, NOCOLOR, RED);
            tc = RED;
        }
        cv->text(cell, num(d), f.sm, tc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        zone(cell, A_CAL_DAY, static_cast<int>(day.serial()));
    }
    int ly = r.bottom - 30;
    if (forPlan) {
        cv->circle(x + 22, ly + 8, 5, BLUE_L);
        cv->text(R(x + 32, ly, 60, 16), L"수업일", f.tiny, SUB);
        cv->circle(x + 92, ly + 8, 5, BLUE);
        cv->text(R(x + 102, ly, 40, 16), L"오늘", f.tiny, SUB);
        cv->circle(x + 146, ly + 8, 5, NOCOLOR, RED);
        cv->text(R(x + 156, ly, 80, 16), L"목표일", f.tiny, SUB);
    } else {
        cv->text(R(x + 18, ly, w - 30, 16), L"날짜를 누르면 그 주로 이동합니다", f.tiny, SUB);
    }
}

// 주 이동 막대: ‹ › 2026년 11월 4주차 (11/23 ~ 11/29)
void MainWindow::weekBar(int x, int y, int w) {
    button(R(x, y, 34, 34), L"<", S_SECONDARY, A_WEEK_PREV);
    button(R(x + 40, y, 34, 34), L">", S_SECONDARY, A_WEEK_NEXT);
    std::wstring t = weekLabel(viewWeek);
    cv->text(R(x + 88, y, 260, 34), t, f.h2, INK);
    int tw = cv->textWidth(t, f.h2);
    cv->text(R(x + 96 + tw, y, 200, 34), L"(" + md(viewWeek) + L" ~ " + md(viewWeek.addDays(6)) + L")", f.sm, SUB);
    (void)w;
}

// ======================================================================
//  사이드바
// ======================================================================
void MainWindow::drawSidebar() {
    cv->fill(R(0, 0, SB, viewH + 2), WHITE);
    cv->line(SB - 1, 0, SB - 1, viewH + 2, BORDER);
    cv->round(R(22, 26, 36, 36), 9, BLUE);
    cv->text(R(22, 26, 36, 36), L"T", f.h2, WHITE, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    cv->text(R(68, 24, 150, 24), L"TutorMate", f.logo, INK);
    cv->text(R(68, 48, 150, 18), L"Plan Better, Teach Better", f.tiny, SUB);

    struct Nav {
        Page p;
        const wchar_t* label;
        COLORREF icon;
    };
    const Nav nav[] = {
        {Page::Dashboard, L"대시보드", BLUE},       {Page::Curriculum, L"커리큘럼 관리", GREEN},
        {Page::Students, L"학생 관리", PURPLE},      {Page::Weekly, L"주간 시간표", RGB(14, 165, 233)},
        {Page::Prep, L"수업 준비", RGB(234, 88, 12)}, {Page::Stats, L"통계", RGB(219, 39, 119)},
        {Page::Settings, L"설정", SUB},
    };
    int y = 96;
    for (const Nav& n : nav) {
        bool on = page == n.p;
        RECT r = R(12, y, SB - 24, 42);
        if (on) cv->round(r, 10, BLUE_L);
        cv->round(R(28, y + 14, 14, 14), 4, on ? BLUE : n.icon);
        cv->text(R(54, y, SB - 70, 42), n.label, on ? f.bodyB : f.body, on ? BLUE : INK);
        zone(r, A_NAV, static_cast<int>(n.p));
        y += 48;
    }

    // 아래쪽: 오늘 날짜
    const Date t = app.today();
    RECT box = R(14, viewH - 92, SB - 28, 76);
    cv->round(box, 10, BLUE_XL);
    cv->text(R(28, viewH - 84, SB - 50, 20), app.isTodayOverridden() ? L"기준 날짜 (시연용)" : L"오늘", f.tiny, SUB);
    cv->text(R(28, viewH - 64, SB - 50, 22), longDate(t), f.bodyB, INK);
    cv->text(R(28, viewH - 42, SB - 50, 18), L"변경 내용은 자동 저장됩니다", f.tiny, MUTED);
}

// ======================================================================
//  대시보드
// ======================================================================
void MainWindow::drawDashboard() {
    const Date t = app.today();
    header(L"대시보드", longDate(t) + L"  ·  오늘도 좋은 수업!");
    const int CW = viewW - X0 - 32;

    int todayLessons = 0, weekLessons = 0, todayTodo = 0, overdue = 0;
    for (const Lesson* l : app.lessonsBetween(t, t))
        if (!l->isCancelled()) ++todayLessons;
    for (const Lesson* l : app.lessonsBetween(t.weekStart(), t.weekStart().addDays(6)))
        if (!l->isCancelled()) ++weekLessons;
    for (const PrepTask* p : app.tasksBetween(t, t))
        if (!p->isCompleted()) ++todayTodo;
    for (const PrepTask* p : app.tasksBetween(t.addDays(-90), t.addDays(-1)))
        if (!p->isCompleted()) ++overdue;

    // 요약 타일 4개
    const int tw = (CW - 48) / 4;
    struct Tile {
        const wchar_t* label;
        int value;
        const wchar_t* unit;
        COLORREF c;
    };
    const Tile tiles[] = {{L"오늘 수업", todayLessons, L"회", BLUE},
                          {L"오늘 할 수업 준비", todayTodo, L"개", GREEN},
                          {L"이번 주 수업", weekLessons, L"회", PURPLE},
                          {L"밀린 준비 To-Do", overdue, L"개", overdue ? RED : SUB}};
    for (int i = 0; i < 4; ++i) {
        int x = X0 + i * (tw + 16);
        card(R(x, TOP, tw, 96));
        cv->round(R(x + 18, TOP + 20, 6, 56), 3, tiles[i].c);
        cv->text(R(x + 36, TOP + 16, tw - 50, 22), tiles[i].label, f.sm, SUB);
        std::wstring v = num(tiles[i].value);
        cv->text(R(x + 36, TOP + 40, 100, 40), v, f.big, INK);
        cv->text(R(x + 40 + cv->textWidth(v, f.big), TOP + 52, 40, 24), tiles[i].unit, f.body, SUB);
        if (i == 3 && overdue > 0) button(R(x + tw - 112, TOP + 50, 96, 30), L"오늘로 이동", S_T_RED, A_PREP_ROLLOVER);
    }

    // 오늘 일정 (수업 + 준비를 ScheduleItem 하나로 다룸 = 다형성)
    const int y2 = TOP + 96 + 20;
    const int lw = (CW - 16) * 55 / 100, rw = CW - 16 - lw;
    std::vector<const ScheduleItem*> items = app.itemsOn(t);
    int h1 = std::max(280, 70 + static_cast<int>(items.size()) * 50 + 20);
    card(R(X0, y2, lw, h1));
    cv->text(R(X0 + 22, y2 + 18, 300, 26), L"오늘 일정", f.h2, INK);
    cv->text(R(X0 + lw - 260, y2 + 18, 240, 26), L"수업과 준비 To-Do를 함께 보여줍니다", f.tiny, MUTED,
             DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    int iy = y2 + 60;
    if (items.empty())
        cv->text(R(X0 + 22, iy, lw - 44, 60), L"오늘은 일정이 없습니다. 여유 있게 개인 정비를 해 보세요!", f.body,
                 SUB);
    for (const ScheduleItem* it : items) {
        const PrepTask* task = dynamic_cast<const PrepTask*>(it);
        const Lesson* lesson = dynamic_cast<const Lesson*>(it);
        int x = X0 + 22;
        if (task) {
            cv->check(x, iy + 12, task->isCompleted());
            zone(R(x - 4, iy + 4, 28, 34), A_TASK_TOGGLE, task->getId());
        } else {
            cv->round(R(x + 6, iy + 8, 6, 26), 3, PAL[studentIndex(it->getStudentId()) % 6].border);
        }
        x += 30;
        bool isLesson = lesson != nullptr;
        x += pill(x, iy + 9, W(it->typeName()), isLesson ? BLUE_L : GREEN_L, isLesson ? BLUE : GREEN) + 10;
        cv->text(R(x, iy, 80, 42), studentName(it->getStudentId()), f.bodyB, INK);
        std::wstring desc = task ? W(task->getTitle()) + L"  ·  기한 " + md(task->getDeadline()) : W(it->describe());
        cv->text(R(x + 76, iy, lw - (x - X0) - 100, 42), desc, f.body, it->isCompleted() ? MUTED : INK);
        if (lesson) zone(R(x, iy, lw - (x - X0) - 22, 42), A_LESSON_MENU, lesson->getId());
        cv->line(X0 + 22, iy + 46, X0 + lw - 22, iy + 46, LINEC);
        iy += 50;
    }

    // 다가오는 수업
    const int rx = X0 + lw + 16;
    auto upcoming = app.lessonsBetween(t.addDays(1), t.addDays(3));
    int h2 = std::max(280, 70 + static_cast<int>(upcoming.size()) * 50 + 20);
    card(R(rx, y2, rw, h2));
    cv->text(R(rx + 22, y2 + 18, 300, 26), L"다가오는 수업 (3일)", f.h2, INK);
    iy = y2 + 60;
    if (upcoming.empty()) cv->text(R(rx + 22, iy, rw - 44, 30), L"예정된 수업이 없습니다.", f.body, SUB);
    for (const Lesson* l : upcoming) {
        cv->text(R(rx + 22, iy, 86, 42), mdw(l->getDate()), f.sm, SUB);
        cv->text(R(rx + 108, iy, 70, 42), studentName(l->getStudentId()), f.bodyB, INK);
        cv->text(R(rx + 176, iy, rw - 290, 42), l->isCancelled() ? L"휴강" : W(l->getTitle()), f.body,
                 l->isCancelled() ? MUTED : INK);
        if (!l->isCancelled()) {
            bool prepared = !app.needsPrep(*l);
            for (const PrepTask* p : app.tasksOfLesson(l->getId()))
                if (p->isCompleted()) prepared = true;
            pill(rx + rw - 104, iy + 9, prepared ? L"준비 완료" : L"준비 필요", prepared ? GREEN_L : RED_L,
                 prepared ? GREEN : RED);
        }
        zone(R(rx + 22, iy, rw - 44, 42), A_LESSON_MENU, l->getId());
        cv->line(rx + 22, iy + 46, rx + rw - 22, iy + 46, LINEC);
        iy += 50;
    }

    // 이번 주 준비 진행 + 학생별 목표일
    const int y3 = y2 + std::max(h1, h2) + 20;
    const Date ws = t.weekStart();
    auto weekTasks = app.tasksBetween(ws, ws.addDays(6));
    int done = 0;
    for (auto* p : weekTasks)
        if (p->isCompleted()) ++done;
    const auto& students = app.getStudents();
    int h3 = std::max(280, 70 + static_cast<int>(students.size()) * 40);
    card(R(X0, y3, CW, h3));
    cv->text(R(X0 + 22, y3 + 18, 300, 26), L"이번 주 준비 진행", f.h2, INK);
    donut(X0 + 110, y3 + 128, 56, done, static_cast<int>(weekTasks.size()), L"To-Do 없음");
    button(R(X0 + 40, y3 + h3 - 50, 140, 34), L"수업 준비 보기", S_T_BLUE, A_NAV, static_cast<int>(Page::Prep));

    const int sx = X0 + 260;
    cv->text(R(sx, y3 + 18, 300, 26), L"학생별 목표일", f.h2, INK);
    int sy = y3 + 58;
    for (const Student& s : students) {
        auto ls = app.lessonsOf(s.getId(), false);
        int fin = 0;
        for (auto* l : ls)
            if (l->isCompleted()) ++fin;
        cv->round(R(sx, sy + 8, 10, 10), 5, PAL[studentIndex(s.getId()) % 6].border);
        cv->text(R(sx + 18, sy, 80, 26), W(s.getName()), f.bodyB, INK);
        cv->text(R(sx + 96, sy, 90, 26), W(s.getInfo()), f.sm, SUB);
        pill(sx + 190, sy + 1, dday(s.getTargetDate()), BLUE_L, BLUE);
        progress(R(sx + 280, sy + 9, CW - 260 - 280 - 130, 8), fin, static_cast<int>(ls.size()), BLUE);
        cv->text(R(X0 + CW - 120, sy, 100, 26), L"수업 " + num(fin) + L"/" + num(static_cast<int>(ls.size())),
                 f.sm, SUB, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        zone(R(sx, sy, CW - 280, 28), A_STU_PLAN, s.getId());
        sy += 40;
    }
    if (students.empty()) {
        cv->text(R(sx, sy, 400, 26), L"등록된 학생이 없습니다.", f.body, SUB);
        button(R(sx, sy + 34, 120, 34), L"+ 학생 추가", S_PRIMARY, A_STU_ADD);
    }
}

// ======================================================================
//  커리큘럼 관리 + 학생별 수업 계획 (기능 1)
// ======================================================================
void MainWindow::drawCurriculum() {
    header(L"커리큘럼 관리", L"나만의 전체 커리큘럼을 등록하고, 학생별로 수업을 계획해보세요.");
    const int CW = viewW - X0 - 32;

    if (!app.findStudent(selStudent) && !app.getStudents().empty()) selStudent = app.getStudents()[0].getId();

    // ---- 왼쪽: 전체 커리큘럼 ----
    const auto& curs = app.getCurricula();
    const int lw = 390;
    int lh = std::max(220, 112 + static_cast<int>(curs.size()) * 40 + 16);
    card(R(X0, TOP, lw, lh));
    cv->text(R(X0 + 20, TOP + 18, 200, 28), L"전체 커리큘럼", f.h2, INK);
    button(R(X0 + lw - 146, TOP + 16, 128, 34), L"+ 커리큘럼 추가", S_PRIMARY, A_CUR_ADD);
    int hy = TOP + 66;
    cv->fill(R(X0 + 12, hy, lw - 24, 32), GRAY_L);
    cv->text(R(X0 + 20, hy, 34, 32), L"번호", f.smB, SUB);
    cv->text(R(X0 + 62, hy, 50, 32), L"분류", f.smB, SUB);
    cv->text(R(X0 + 126, hy, 160, 32), L"커리큘럼 제목", f.smB, SUB);
    cv->text(R(X0 + 296, hy, 60, 32), L"소요시간", f.smB, SUB);
    int ry = hy + 40;
    if (curs.empty()) cv->text(R(X0 + 20, ry, lw - 40, 30), L"커리큘럼을 추가해 주세요.", f.body, SUB);
    for (size_t i = 0; i < curs.size(); ++i) {
        const Curriculum& c = curs[i];
        Pal p = categoryPal(c.getCategory());
        cv->text(R(X0 + 20, ry, 34, 32), num(static_cast<int>(i) + 1), f.body, SUB);
        pill(X0 + 58, ry + 4, W(c.getCategory()), p.bg, p.text);
        cv->text(R(X0 + 126, ry, 166, 32), W(c.getTitle()), f.body, INK);
        pill(X0 + 300, ry + 4, num(c.getDuration()) + L"h", GRAY_L, SUB);
        for (int k = -1; k <= 1; ++k) cv->circle(X0 + lw - 28, ry + 16 + k * 6, 2, SUB);
        zone(R(X0 + 12, ry, lw - 24, 32), A_CUR_MENU, static_cast<int>(i));
        cv->line(X0 + 16, ry + 36, X0 + lw - 16, ry + 36, LINEC);
        ry += 40;
    }

    // ---- 오른쪽: 달력 (창이 좁으면 생략) ----
    const int calW = 250;
    const bool showCal = CW >= 1100;
    const int calX = showCal ? X0 + CW - calW : X0 + CW + 16;
    if (showCal) calendar(R(calX, TOP, calW, 300), true);

    // ---- 가운데: 학생별 수업 계획 ----
    const int px = X0 + lw + 16;
    const int pw = calX - 16 - px;
    if (!app.findStudent(selStudent) && !app.getStudents().empty()) selStudent = app.getStudents()[0].getId();
    const Student* s = app.findStudent(selStudent);
    if (!s) {
        card(R(px, TOP, pw, 200));
        cv->text(R(px + 24, TOP + 24, pw - 48, 28), L"학생별 수업 계획", f.h2, INK);
        cv->text(R(px + 24, TOP + 64, pw - 48, 24), L"먼저 학생을 등록해 주세요.", f.body, SUB);
        button(R(px + 24, TOP + 104, 130, 36), L"+ 학생 추가", S_PRIMARY, A_STU_ADD);
        return;
    }
    auto lessons = app.lessonsOf(selStudent, true);
    int ph = 300 + static_cast<int>(lessons.size()) * 40 + 70;
    card(R(px, TOP, pw, ph));

    button(R(px + 16, TOP + 16, 32, 32), L"<", S_SECONDARY, A_PLAN_PREV);
    std::wstring nm = W(s->getName()) + L" 학생";
    int nmW = cv->textWidth(nm, f.h2);
    cv->text(R(px + 58, TOP + 16, nmW + 4, 32), nm, f.h2, INK);
    cv->text(R(px + 62 + nmW, TOP + 16, 16, 32), L"▼", f.tiny, SUB);
    zone(R(px + 58, TOP + 16, nmW + 22, 32), A_PLAN_PICK);
    int nx = px + 86 + nmW;
    button(R(nx, TOP + 16, 32, 32), L">", S_SECONDARY, A_PLAN_NEXT);
    button(R(px + pw - 150, TOP + 16, 134, 32), L"학생 정보 수정", S_SECONDARY, A_PLAN_EDIT_STUDENT);

    // 정보 상자
    RECT ib = R(px + 16, TOP + 62, pw - 32, 104);
    cv->round(ib, 10, BLUE_XL);
    const int lx = px + 34, vx = px + 124;
    cv->text(R(lx, TOP + 72, 90, 26), L"목표일", f.sm, SUB);
    cv->text(R(vx, TOP + 72, 260, 26), W(s->getTargetDate().toString()) + L"  (" + dday(s->getTargetDate()) + L")",
             f.bodyB, INK);
    cv->text(R(lx, TOP + 100, 90, 26), L"수업 요일", f.sm, SUB);
    cv->text(R(vx, TOP + 100, 260, 26),
             W(s->daysToString()) + L" (주 " + num(s->getLessonsPerWeek()) + L"회)", f.bodyB, INK);
    cv->text(R(lx, TOP + 128, 90, 26), L"1회 수업", f.sm, SUB);
    cv->text(R(vx, TOP + 128, 260, 26),
             num(s->getLessonHours()) + L"시간  (" + hourW(s->getStartHour()) + L" 시작)", f.bodyB, INK);
    RECT tb = R(px + pw - 32 - 150, TOP + 72, 150, 84);
    cv->round(tb, 10, WHITE, BLUE_L);
    cv->text(R(tb.left, tb.top + 10, 150, 20), L"총 수업 가능 횟수", f.sm, BLUE, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    cv->text(R(tb.left, tb.top + 32, 150, 40), num(app.countPossibleLessons(selStudent, app.today())) + L"회", f.big,
             BLUE, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 버튼 줄
    int by = TOP + 184;
    cv->text(R(px + 20, by, 120, 34), L"수업 일정", f.bodyB, INK);
    button(R(px + pw - 16 - 118, by, 118, 34), L"자동 일정 생성", S_PRIMARY, A_PLAN_GEN);
    button(R(px + pw - 16 - 118 - 8 - 96, by, 96, 34), L"다시 배치", S_SECONDARY, A_PLAN_ASSIGN);
    button(R(px + pw - 16 - 118 - 8 - 96 - 8 - 84, by, 84, 34), L"초기화", S_SECONDARY, A_PLAN_RESET);

    // 표
    int ty = by + 48;
    cv->fill(R(px + 12, ty, pw - 24, 32), GRAY_L);
    const int cRound = px + 20, cDate = px + 66, cDay = px + 126, cCur = px + 164;
    const int cState = px + pw - 120, curW = cState - cCur - 14;
    cv->text(R(cRound, ty, 40, 32), L"회차", f.smB, SUB);
    cv->text(R(cDate, ty, 50, 32), L"날짜", f.smB, SUB);
    cv->text(R(cDay, ty, 34, 32), L"요일", f.smB, SUB);
    cv->text(R(cCur, ty, curW, 32), L"커리큘럼 (눌러서 선택)", f.smB, SUB);
    cv->text(R(cState, ty, 60, 32), L"상태", f.smB, SUB);
    int ry2 = ty + 40, round = 0;
    if (lessons.empty())
        cv->text(R(px + 20, ry2, pw - 40, 30), L"수업이 없습니다. [자동 일정 생성]을 눌러 보세요.", f.body, SUB);
    for (const Lesson* l : lessons) {
        bool past = l->isCompleted() || l->isCancelled();
        COLORREF tc = past ? MUTED : INK;
        cv->text(R(cRound, ry2, 40, 32), l->isCancelled() ? L"-" : num(++round), f.body, tc);
        cv->text(R(cDate, ry2, 56, 32), md(l->getDate()), f.body, tc);
        cv->text(R(cDay, ry2, 34, 32), W(l->getDate().weekdayName()), f.body, tc);
        RECT box = R(cCur, ry2 + 2, curW, 28);
        cv->round(box, 6, past ? GRAY_L : WHITE, past ? NOCOLOR : GRAY_M);
        cv->text(R(box.left + 10, box.top, curW - 34, 28), W(l->getTitle()), f.body,
                 l->getCurriculumId() < 0 ? MUTED : tc);
        cv->text(R(box.right - 24, box.top, 18, 28), L"▼", f.tiny, MUTED, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (!l->isCancelled()) zone(box, A_PLAN_CUR, l->getId());

        std::wstring st = W(l->statusName());
        COLORREF sb = BLUE_L, sf = BLUE;
        if (l->isCancelled()) { sb = GRAY_L; sf = SUB; }
        else if (l->isCompleted()) { sb = GREEN_L; sf = GREEN; }
        else if (l->getStatus() == LessonStatus::Makeup) { sb = YELLOW_L; sf = YELLOW; }
        pill(cState, ry2 + 4, st, sb, sf);
        for (int k = -1; k <= 1; ++k) cv->circle(px + pw - 32, ry2 + 16 + k * 6, 2, SUB);
        zone(R(px + pw - 48, ry2, 32, 32), A_LESSON_MENU, l->getId());
        zone(R(cState, ry2, 80, 32), A_LESSON_MENU, l->getId());
        cv->line(px + 16, ry2 + 36, px + pw - 16, ry2 + 36, LINEC);
        ry2 += 40;
    }
    button(R(px + pw / 2 - 70, ry2 + 12, 140, 36), L"+ 수업 추가", S_T_BLUE, A_PLAN_ADD_LESSON);
}

// ======================================================================
//  학생 관리
// ======================================================================
void MainWindow::drawStudents() {
    header(L"학생 관리", L"학생별 목표일과 수업 요일을 등록하면 수업 일정이 자동으로 만들어집니다.");
    button(R(viewW - 32 - 140, 28, 140, 40), L"+ 학생 추가", S_PRIMARY, A_STU_ADD);
    const int CW = viewW - X0 - 32;
    const auto& list = app.getStudents();
    int h = std::max(200, 80 + static_cast<int>(list.size()) * 58 + 20);
    card(R(X0, TOP, CW, h));

    const int y0 = TOP + 18;
    cv->fill(R(X0 + 12, y0, CW - 24, 34), GRAY_L);
    // 열 위치는 창 너비에 비례 (오른쪽 278px은 버튼 영역)
    const int avail = CW - 278 - 26;
    const double frac[] = {0.0, 0.10, 0.20, 0.32, 0.41, 0.53, 0.66, 0.76};
    int cx[8];
    for (int i = 0; i < 8; ++i) cx[i] = X0 + 26 + static_cast<int>(avail * frac[i]);
    const wchar_t* heads[] = {L"이름", L"학년/과목", L"목표일", L"D-day", L"수업 요일", L"수업 시간", L"남은 수업",
                              L"진도"};
    for (int i = 0; i < 8; ++i) cv->text(R(cx[i], y0, i < 7 ? cx[i + 1] - cx[i] - 4 : 110, 34), heads[i], f.smB, SUB);
    auto colW = [&](int i) { return (i < 7 ? cx[i + 1] : X0 + CW - 278) - cx[i] - 6; };

    int y = y0 + 44;
    if (list.empty()) cv->text(R(X0 + 26, y, 400, 30), L"등록된 학생이 없습니다. [+ 학생 추가]를 눌러 주세요.", f.body, SUB);
    const int total = static_cast<int>(app.getCurricula().size());
    for (const Student& s : list) {
        Pal p = PAL[studentIndex(s.getId()) % 6];
        cv->round(R(X0 + 14, y + 10, 6, 30), 3, p.border);
        cv->text(R(cx[0], y, colW(0), 50), W(s.getName()), f.bodyB, INK);
        cv->text(R(cx[1], y, colW(1), 50), W(s.getInfo()), f.body, SUB);
        cv->text(R(cx[2], y, colW(2), 50), W(s.getTargetDate().toString()), f.body, INK);
        pill(cx[3], y + 13, dday(s.getTargetDate()), BLUE_L, BLUE);
        cv->text(R(cx[4], y, colW(4), 50), W(s.daysToString()) + L" (주" + num(s.getLessonsPerWeek()) + L"회)", f.body,
                 INK);
        cv->text(R(cx[5], y, colW(5), 50), hourW(s.getStartHour()) + L" · " + num(s.getLessonHours()) + L"시간", f.body,
                 INK);
        cv->text(R(cx[6], y, colW(6), 50), num(app.countPossibleLessons(s.getId(), app.today())) + L"회", f.body, INK);
        std::set<int> learned;
        for (const Lesson* l : app.lessonsOf(s.getId(), false))
            if (l->isCompleted() && l->getCurriculumId() >= 0) learned.insert(l->getCurriculumId());
        int bw = std::max(40, static_cast<int>(avail * 0.24) - 52);
        progress(R(cx[7], y + 21, bw, 8), static_cast<int>(learned.size()), total, p.border);
        cv->text(R(cx[7] + bw + 6, y, 46, 50), num(total ? static_cast<int>(learned.size()) * 100 / total : 0) + L"%",
                 f.sm, SUB);
        int bx = X0 + CW - 24 - 254;
        button(R(bx, y + 9, 96, 32), L"수업 계획", S_T_BLUE, A_STU_PLAN, s.getId());
        button(R(bx + 104, y + 9, 70, 32), L"수정", S_SECONDARY, A_STU_EDIT, s.getId());
        button(R(bx + 182, y + 9, 70, 32), L"삭제", S_T_RED, A_STU_DEL, s.getId());
        cv->line(X0 + 16, y + 54, X0 + CW - 16, y + 54, LINEC);
        y += 58;
    }
}

// ======================================================================
//  주간 시간표 (부가기능)
// ======================================================================
void MainWindow::drawWeekly() {
    header(L"주간 시간표", L"이번 주의 수업 일정을 한눈에 확인하고, 간편하게 수정하세요.");
    button(R(viewW - 32 - 130, 28, 130, 40), L"+ 일정 추가", S_PRIMARY, A_QUICK, 0);
    const int CW = viewW - X0 - 32;
    const int rightW = 260;
    const int mw = CW - rightW - 16;

    auto week = app.lessonsBetween(viewWeek, viewWeek.addDays(6));
    int minH = 9, maxH = 22;
    for (const Lesson* l : week) {
        minH = std::min(minH, l->getStartHour());
        maxH = std::max(maxH, l->getEndHour());
    }
    maxH = std::min(24, maxH);
    const int rowH = 40, headH = 48, timeW = 58;
    const int gx = X0 + 16, gy = TOP + 66;
    const int colW = (mw - 32 - timeW) / 7;
    const int gridH = headH + (maxH - minH) * rowH;
    card(R(X0, TOP, mw, 66 + gridH + 56));

    weekBar(X0 + 16, TOP + 16, mw);
    button(R(X0 + mw - 16 - 70, TOP + 16, 70, 34), L"오늘", S_T_BLUE, A_WEEK_TODAY);

    // 머리글
    const Date today = app.today();
    for (int i = 0; i < 7; ++i) {
        Date d = viewWeek.addDays(i);
        RECT hr = R(gx + timeW + i * colW, gy, colW, headH);
        if (d == today) cv->round(R(hr.left + 2, hr.top + 2, colW - 4, headH - 4), 8, BLUE_L);
        COLORREF dc = i == 6 ? RED : (i == 5 ? BLUE : INK);
        cv->text(R(hr.left, hr.top + 4, colW, 20), W(d.weekdayName()), f.bodyB, dc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        cv->text(R(hr.left, hr.top + 24, colW, 18), W(std::to_string(d.month()) + "/" + std::to_string(d.day())), f.sm,
                 SUB, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    cv->text(R(gx, gy, timeW, headH), L"시간", f.smB, SUB, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 격자 + 빈 칸 클릭 영역
    const int bodyY = gy + headH;
    for (int h = minH; h <= maxH; ++h) {
        int y = bodyY + (h - minH) * rowH;
        cv->line(gx, y, gx + timeW + 7 * colW, y, LINEC);
        if (h < maxH) cv->text(R(gx, y, timeW, 22), hourW(h), f.tiny, SUB, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    for (int i = 0; i <= 7; ++i) cv->line(gx + timeW + i * colW, gy, gx + timeW + i * colW, bodyY + (maxH - minH) * rowH, LINEC);
    for (int i = 0; i < 7; ++i)
        for (int h = minH; h < maxH; ++h)
            zone(R(gx + timeW + i * colW, bodyY + (h - minH) * rowH, colW, rowH), A_CELL_ADD,
                 static_cast<int>(viewWeek.addDays(i).serial()), h);

    // 수업 블록
    for (const Lesson* l : week) {
        int day = l->getDate() - viewWeek;
        // 같은 시간대 겹치는 수업은 칸을 나눠서 표시
        int lane = 0, lanes = 1;
        for (const Lesson* o : week) {
            if (o == l || !o->overlaps(*l)) continue;
            ++lanes;
            if (o->getStartHour() < l->getStartHour() || (o->getStartHour() == l->getStartHour() && o->getId() < l->getId()))
                ++lane;
        }
        int bw = (colW - 6) / lanes;
        int x = gx + timeW + day * colW + 3 + lane * bw;
        int y = bodyY + (l->getStartHour() - minH) * rowH + 2;
        int h = l->getDuration() * rowH - 4;
        RECT b = R(x, y, bw - (lanes > 1 ? 2 : 0), h);
        Pal p = PAL[studentIndex(l->getStudentId()) % 6];
        COLORREF tc = p.text;
        if (l->isCancelled()) {
            cv->dashedRound(b, 8, GRAY_L, MUTED);
            cv->hatch(R(b.left + 2, b.top + 2, b.right - b.left - 4, b.bottom - b.top - 4), GRAY_M);
            tc = SUB;
        } else if (l->getStatus() == LessonStatus::Makeup) {
            cv->dashedRound(b, 8, YELLOW_L, YELLOW_B);
            tc = YELLOW;
        } else {
            cv->round(b, 8, p.bg, p.border);
        }
        if (l->getId() == selLesson) cv->round(b, 8, NOCOLOR, BLUE, 2);
        int tx = b.left + 7, twd = b.right - b.left - 10;
        cv->text(R(tx, b.top + 4, twd, 16), twd >= 80 ? W(l->timeRange()) : hourW(l->getStartHour()) + L"~", f.tiny, tc);
        std::wstring name = l->isCancelled() ? L"휴강 · " + studentName(l->getStudentId()) : studentName(l->getStudentId());
        cv->text(R(tx, b.top + 20, twd, 18), name, f.smB, l->isCompleted() ? MUTED : INK);
        const Student* s = app.findStudent(l->getStudentId());
        if (h >= 56 && s) cv->text(R(tx, b.top + 38, twd, 16), L"(" + W(s->getInfo()) + L")", f.tiny, tc);
        if (h >= 72) {
            std::wstring sub = W(l->getTitle());
            if (l->isCompleted()) sub = L"[완료] " + sub;
            if (l->hasOriginDate()) sub = L"보강 (" + md(l->getOriginDate()) + L"→)";
            cv->text(R(tx, b.top + 54, twd, 16), sub, f.tiny, tc);
        }
        zone(b, A_SELECT_LESSON, l->getId());
    }

    // 범례
    int ly = bodyY + (maxH - minH) * rowH + 18;
    int lx = gx;
    struct Leg {
        COLORREF c;
        const wchar_t* t;
    };
    const Leg legs[] = {{PAL[0].bg, L"정상 수업"}, {YELLOW_L, L"보강 수업"}, {GRAY_M, L"휴강"}};
    for (const Leg& lg : legs) {
        cv->round(R(lx, ly + 3, 14, 14), 4, lg.c);
        cv->text(R(lx + 20, ly, 80, 20), lg.t, f.sm, SUB);
        lx += 100;
    }
    cv->text(R(lx + 10, ly, mw - (lx - X0) - 30, 20), L"빈 칸을 누르면 그 시간에 수업을 추가합니다.", f.sm, MUTED);

    // ---- 오른쪽 패널 ----
    const int rx = X0 + mw + 16;
    calendar(R(rx, TOP, rightW, 300), false);

    int sy = TOP + 316;
    const Lesson* sel = app.findLesson(selLesson);
    card(R(rx, sy, rightW, 210));
    cv->text(R(rx + 18, sy + 16, 150, 26), L"선택된 일정", f.h2, INK);
    if (!sel) {
        cv->text(R(rx + 18, sy + 56, rightW - 36, 60), L"시간표에서 수업을 누르면\n자세한 내용이 여기에 보입니다.", f.sm,
                 SUB, DT_LEFT | DT_TOP | DT_WORDBREAK);
    } else {
        button(R(rx + rightW - 76, sy + 14, 60, 30), L"메뉴", S_SECONDARY, A_LESSON_MENU, sel->getId());
        cv->text(R(rx + 18, sy + 52, rightW - 36, 22), longDate(sel->getDate()), f.bodyB, INK);
        cv->text(R(rx + 18, sy + 74, 120, 22), W(sel->timeRange()), f.body, SUB);
        std::wstring st = W(sel->statusName());
        COLORREF sb = sel->isCancelled() ? GRAY_L : sel->isCompleted() ? GREEN_L
                      : sel->getStatus() == LessonStatus::Makeup ? YELLOW_L : BLUE_L;
        COLORREF sf = sel->isCancelled() ? SUB : sel->isCompleted() ? GREEN
                      : sel->getStatus() == LessonStatus::Makeup ? YELLOW : BLUE;
        pill(rx + rightW - 18 - cv->textWidth(st, f.smB) - 20, sy + 73, st, sb, sf);
        struct Row {
            const wchar_t* k;
            std::wstring v;
        };
        const Student* st2 = app.findStudent(sel->getStudentId());
        std::wstring note = W(sel->getNote());
        if (sel->hasOriginDate()) note = L"원래 " + mdw(sel->getOriginDate()) + L" 수업";
        const Row rows[] = {{L"학생", studentName(sel->getStudentId()) + (st2 ? L" (" + W(st2->getInfo()) + L")" : L"")},
                            {L"커리큘럼", W(sel->getTitle())},
                            {L"비고", note.empty() ? L"-" : note}};
        int yy = sy + 108;
        for (const Row& r : rows) {
            cv->text(R(rx + 18, yy, 70, 26), r.k, f.sm, SUB);
            cv->text(R(rx + 86, yy, rightW - 104, 26), r.v, f.sm, INK);
            yy += 30;
        }
    }

    int qy = sy + 226;
    card(R(rx, qy, rightW, 186));
    cv->text(R(rx + 18, qy + 16, 150, 26), L"빠른 작업", f.h2, INK);
    const bool has = sel != nullptr;
    const bool active = has && !sel->isCancelled();
    const int bw = (rightW - 44) / 2;
    button(R(rx + 16, qy + 54, bw, 36), L"+ 수업 추가", S_T_BLUE, A_QUICK, 0);
    button(R(rx + 28 + bw, qy + 54, bw, 36), L"보강으로 변경", has ? S_T_YELLOW : S_DISABLED, A_QUICK, 1);
    button(R(rx + 16, qy + 96, bw, 36), L"휴강으로 변경", active && !sel->isCompleted() ? S_T_GRAY : S_DISABLED, A_QUICK, 2);
    button(R(rx + 28 + bw, qy + 96, bw, 36), has && sel->isCompleted() ? L"완료 취소" : L"완료 처리",
           active ? S_T_GREEN : S_DISABLED, A_QUICK, 3);
    button(R(rx + 16, qy + 138, bw, 36), L"커리큘럼 변경", active ? S_T_PURPLE : S_DISABLED, A_QUICK, 4);
    button(R(rx + 28 + bw, qy + 138, bw, 36), L"일정 삭제", has ? S_T_RED : S_DISABLED, A_QUICK, 5);
}

// ======================================================================
//  수업 준비 To-Do (기능 2)
// ======================================================================
void MainWindow::drawPrep() {
    header(L"수업 준비", L"이번 주 수업에 필요한 준비 사항을 균등하게 배분하여, 여유 있게 준비할 수 있도록 도와드립니다.");
    button(R(viewW - 32 - 180, 28, 180, 40), L"주간 계획 다시 생성", S_PRIMARY, A_PREP_REGEN);
    button(R(viewW - 32 - 180 - 10 - 140, 28, 140, 40), L"자동 배분 설정", S_SECONDARY, A_PREP_SETTINGS);
    const int CW = viewW - X0 - 32;
    const Date ws = viewWeek, we = viewWeek.addDays(6), today = app.today();

    auto weekLessons = app.lessonsBetween(ws, we);
    std::vector<const PrepTask*> shown = app.tasksBetween(ws, we);
    std::set<int> ids;
    for (auto* p : shown) ids.insert(p->getId());
    std::vector<const PrepTask*> early;   // 이번 주 수업을 위해 지난주에 배정된 준비
    for (const Lesson* l : weekLessons)
        for (const PrepTask* p : app.tasksOfLesson(l->getId()))
            if (!ids.count(p->getId()) && p->getDate() < ws) {
                early.push_back(p);
                ids.insert(p->getId());
            }
    int active = 0;
    for (auto* l : weekLessons)
        if (!l->isCancelled()) ++active;
    int overdue = 0;
    for (auto* p : app.tasksBetween(today.addDays(-90), today.addDays(-1)))
        if (!p->isCompleted()) ++overdue;

    // 요일별 칸
    std::vector<std::vector<const PrepTask*>> cols(7);
    for (auto* p : early) cols[0].push_back(p);
    for (auto* p : shown) cols[static_cast<size_t>(p->getDate() - ws)].push_back(p);
    size_t maxItems = 0;
    for (auto& c : cols) maxItems = std::max(maxItems, c.size());
    const int itemH = 68;
    const int colH = std::max(210, 62 + static_cast<int>(maxItems) * itemH + 12);
    card(R(X0, TOP, CW, 70 + colH + 18));
    weekBar(X0 + 18, TOP + 16, CW);
    std::wstring summary = L"이번 주 수업 " + num(active) + L"회  |  준비할 내용 " +
                           num(static_cast<int>(shown.size() + early.size())) + L"개";
    int sw = cv->textWidth(summary, f.bodyB) + 32;
    int sx = X0 + CW - 18 - sw;
    if (overdue > 0) {
        button(R(sx - 10 - 160, TOP + 16, 160, 34), L"밀린 " + num(overdue) + L"개 오늘로", S_T_RED, A_PREP_ROLLOVER);
    }
    cv->round(R(sx, TOP + 16, sw, 34), 8, BLUE_XL, BLUE_L);
    cv->text(R(sx, TOP + 16, sw, 34), summary, f.bodyB, BLUE, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    const int gap = 10;
    const int colW = (CW - 36 - gap * 6) / 7;
    const int cy = TOP + 68;
    for (int i = 0; i < 7; ++i) {
        Date d = ws.addDays(i);
        int x = X0 + 18 + i * (colW + gap);
        cv->round(R(x, cy, colW, colH), 10, DAY_TINT[i], d == today ? BLUE : NOCOLOR, d == today ? 2 : 1);
        cv->text(R(x, cy + 10, colW, 20), mdw(d), f.bodyB, i == 6 ? RED : INK, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        cv->text(R(x, cy + 30, colW, 18), L"할 일 " + num(static_cast<int>(cols[static_cast<size_t>(i)].size())) + L"개",
                 f.tiny, SUB, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        int iy = cy + 56;
        for (const PrepTask* p : cols[static_cast<size_t>(i)]) {
            RECT ir = R(x + 6, iy, colW - 12, itemH - 8);
            cv->round(ir, 8, WHITE, BORDER);
            cv->check(ir.left + 8, ir.top + 9, p->isCompleted());
            zone(R(ir.left + 2, ir.top + 2, 30, 30), A_TASK_TOGGLE, p->getId());
            COLORREF tc = p->isCompleted() ? MUTED : INK;
            cv->text(R(ir.left + 32, ir.top + 6, ir.right - ir.left - 36, 18), studentName(p->getStudentId()), f.smB, tc);
            std::wstring t = W(p->getTitle());
            if (p->getDate() < ws) t = L"(" + md(p->getDate()) + L") " + t;
            if (p->isOverdue(today)) t = L"[밀림] " + t;
            cv->text(R(ir.left + 32, ir.top + 24, ir.right - ir.left - 38, 34), t, f.tiny,
                     p->isOverdue(today) ? RED : (p->isCompleted() ? MUTED : SUB), DT_LEFT | DT_TOP | DT_WORDBREAK);
            zone(R(ir.left + 32, ir.top, ir.right - ir.left - 32, ir.bottom - ir.top), A_TASK_MENU, p->getId());
            iy += itemH;
        }
        if (cols[static_cast<size_t>(i)].empty()) {
            std::wstring msg = d.isWeekend() ? L"주말에는\n충분한 휴식을\n취하세요!" : L"준비할 내용이\n없습니다.\n(여유 있게 개인 정비!)";
            cv->text(R(x + 8, cy + 84, colW - 16, 80), msg, f.tiny, SUB, DT_CENTER | DT_TOP | DT_WORDBREAK);
        }
    }

    // 아래: 준비 대상 수업 목록
    const int y2 = TOP + 70 + colH + 18 + 18;
    const int rw = 300;
    const int lw = CW - rw - 16;
    int listH = std::max(270, 70 + active * 40 + 20);
    card(R(X0, y2, lw, listH));
    cv->text(R(X0 + 20, y2 + 16, 300, 28), L"준비 대상 수업 목록 (이번 주)", f.h2, INK);
    int ty = y2 + 56;
    cv->fill(R(X0 + 12, ty, lw - 24, 32), GRAY_L);
    const int c1 = X0 + 20, c2 = X0 + 110, c3 = X0 + 190, c6 = X0 + lw - 100, c5 = c6 - 90, c4 = c5 - 100;
    cv->text(R(c1, ty, 80, 32), L"수업일", f.smB, SUB);
    cv->text(R(c2, ty, 70, 32), L"학생", f.smB, SUB);
    cv->text(R(c3, ty, c4 - c3, 32), L"커리큘럼", f.smB, SUB);
    cv->text(R(c4, ty, 96, 32), L"준비 필요 여부", f.smB, SUB);
    cv->text(R(c5, ty, 80, 32), L"준비일", f.smB, SUB);
    cv->text(R(c6, ty, 80, 32), L"상태", f.smB, SUB);
    int ry = ty + 40;
    if (active == 0) cv->text(R(c1, ry, 300, 30), L"이번 주 수업이 없습니다.", f.body, SUB);
    for (const Lesson* l : weekLessons) {
        if (l->isCancelled()) continue;
        Pal p = PAL[studentIndex(l->getStudentId()) % 6];
        cv->text(R(c1, ry, 86, 32), mdw(l->getDate()), f.body, INK);
        pill(c2, ry + 4, studentName(l->getStudentId()), p.bg, p.text);
        cv->text(R(c3, ry, c4 - c3 - 8, 32), W(l->getTitle()), f.body, INK);
        auto ts = app.tasksOfLesson(l->getId());
        bool need = app.needsPrep(*l) || !ts.empty();
        pill(c4, ry + 4, need ? L"필요" : L"불필요", need ? RED_L : GRAY_L, need ? RED : SUB);
        cv->text(R(c5, ry, 80, 32), ts.empty() ? L"-" : md(ts[0]->getDate()), f.body, INK);
        if (l->isCompleted()) pill(c6, ry + 4, L"수업 완료", GRAY_L, SUB);
        else if (!ts.empty()) pill(c6, ry + 4, ts[0]->isCompleted() ? L"완료" : L"미완료",
                                   ts[0]->isCompleted() ? GREEN_L : RED_L, ts[0]->isCompleted() ? GREEN : RED);
        else if (need) pill(c6, ry + 4, L"미배정", YELLOW_L, YELLOW);
        else pill(c6, ry + 4, L"지난 수업 내용", GRAY_L, SUB);
        zone(R(c1, ry, c4 - c1, 32), A_LESSON_MENU, l->getId());
        cv->line(X0 + 16, ry + 36, X0 + lw - 16, ry + 36, LINEC);
        ry += 40;
    }

    // 오른쪽: 진행 현황, 팁
    const int rx = X0 + lw + 16;
    int done = 0, late = 0, total = 0;
    for (auto& c : cols)
        for (auto* p : c) {
            ++total;
            if (p->isCompleted()) ++done;
            else if (p->isOverdue(today)) ++late;
        }
    card(R(rx, y2, rw, 196));
    cv->text(R(rx + 18, y2 + 16, 200, 26), L"준비 진행 현황", f.h2, INK);
    donut(rx + 84, y2 + 116, 52, done, total, L"없음");
    struct Leg {
        COLORREF c;
        const wchar_t* t;
        int v;
    };
    const Leg legs[] = {{GREEN, L"완료", done}, {GRAY_M, L"남음", total - done - late}, {RED, L"밀림", late}};
    int ly = y2 + 76;
    for (const Leg& lg : legs) {
        cv->round(R(rx + 162, ly + 5, 12, 12), 3, lg.c);
        cv->text(R(rx + 182, ly, 60, 22), lg.t, f.sm, INK);
        cv->text(R(rx + 230, ly, 50, 22), num(lg.v) + L"개", f.sm, SUB, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        ly += 28;
    }

    int tipY = y2 + 212;
    cv->round(R(rx, tipY, rw, 150), 12, PINK_L, RGB(251, 207, 232));
    contentBottom = std::max(contentBottom, tipY + 150);
    cv->text(R(rx + 18, tipY + 14, 200, 22), L"Tip", f.bodyB, RED);
    std::wstring tips = L"• 준비 기한은 해당 수업의 전날로 자동 설정됩니다.\n"
                        L"• 지난 수업에서 다룬 커리큘럼은 자동으로 제외됩니다.\n"
                        L"• 하루에 몰리지 않도록 가장 한가한 날에 배분합니다.\n"
                        L"• To-Do를 누르면 원하는 날짜로 옮길 수 있습니다.";
    cv->text(R(rx + 18, tipY + 40, rw - 30, 104), tips, f.tiny, INK, DT_LEFT | DT_TOP | DT_WORDBREAK);
    std::wstring weekendState = app.isWeekendPrepAllowed() ? L"주말 포함 배분" : L"평일 위주 배분";
    int qy = tipY + 166;
    cv->round(R(rx, qy, rw, 92), 12, BLUE_XL, BLUE_L);
    contentBottom = std::max(contentBottom, qy + 92);
    cv->text(R(rx + 18, qy + 14, rw - 36, 44), L"“준비된 한 시간은,\n학생의 한 달을 바꿀 수 있습니다.”", f.sm, BLUE,
             DT_LEFT | DT_TOP | DT_WORDBREAK);
    cv->text(R(rx + 18, qy + 62, rw - 36, 18), L"현재 설정: " + weekendState, f.tiny, SUB);
}

// ======================================================================
//  통계
// ======================================================================
void MainWindow::drawStats() {
    header(L"통계", L"학생별 수업 진행과 커리큘럼 진도를 확인하세요.");
    const int CW = viewW - X0 - 32;
    const int total = static_cast<int>(app.getCurricula().size());

    // 전체 To-Do
    auto all = app.tasksBetween(Date(1900, 1, 1), Date(2200, 12, 31));
    int done = 0, late = 0;
    for (auto* p : all) {
        if (p->isCompleted()) ++done;
        else if (p->isOverdue(app.today())) ++late;
    }
    int allLessons = 0, doneLessons = 0, cancelled = 0;
    for (const Student& s : app.getStudents())
        for (const Lesson* l : app.lessonsOf(s.getId(), true)) {
            if (l->isCancelled()) { ++cancelled; continue; }
            ++allLessons;
            if (l->isCompleted()) ++doneLessons;
        }
    card(R(X0, TOP, CW, 210));
    cv->text(R(X0 + 22, TOP + 18, 300, 26), L"전체 요약", f.h2, INK);
    donut(X0 + 110, TOP + 118, 52, done, static_cast<int>(all.size()), L"없음");
    cv->text(R(X0 + 45, TOP + 182, 130, 18), L"준비 To-Do 완료율", f.tiny, SUB, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    struct Big {
        const wchar_t* k;
        int v;
        const wchar_t* u;
    };
    const Big bigs[] = {{L"학생", static_cast<int>(app.getStudents().size()), L"명"},
                        {L"완료한 수업", doneLessons, L"회"},
                        {L"남은 수업", allLessons - doneLessons, L"회"},
                        {L"휴강", cancelled, L"회"},
                        {L"밀린 To-Do", late, L"개"}};
    int bx = X0 + 240;
    int bw = (CW - 260) / 5;
    for (const Big& b : bigs) {
        cv->text(R(bx, TOP + 64, bw, 20), b.k, f.sm, SUB);
        std::wstring v = num(b.v);
        cv->text(R(bx, TOP + 86, bw, 40), v, f.big, INK);
        cv->text(R(bx + cv->textWidth(v, f.big) + 4, TOP + 98, 40, 24), b.u, f.body, SUB);
        bx += bw;
    }

    // 학생별 카드 (2열)
    const auto& list = app.getStudents();
    const int cw2 = (CW - 16) / 2;
    int i = 0;
    for (const Student& s : list) {
        int x = X0 + (i % 2) * (cw2 + 16);
        int y = TOP + 226 + (i / 2) * 206;
        Pal p = PAL[studentIndex(s.getId()) % 6];
        card(R(x, y, cw2, 190));
        cv->round(R(x + 20, y + 20, 8, 30), 4, p.border);
        cv->text(R(x + 38, y + 16, 200, 24), W(s.getName()), f.h2, INK);
        cv->text(R(x + 38, y + 40, 200, 18), W(s.getInfo()) + L"  ·  " + W(s.daysToString()), f.sm, SUB);
        pill(x + cw2 - 90, y + 20, dday(s.getTargetDate()), BLUE_L, BLUE);

        int all2 = 0, fin = 0, canc = 0, makeup = 0;
        std::set<int> learned;
        for (const Lesson* l : app.lessonsOf(s.getId(), true)) {
            if (l->isCancelled()) { ++canc; continue; }
            ++all2;
            if (l->getStatus() == LessonStatus::Makeup) ++makeup;
            if (l->isCompleted()) {
                ++fin;
                if (l->getCurriculumId() >= 0) learned.insert(l->getCurriculumId());
            }
        }
        cv->text(R(x + 22, y + 74, 110, 22), L"수업 진행", f.sm, SUB);
        progress(R(x + 120, y + 82, cw2 - 220, 8), fin, all2, p.border);
        cv->text(R(x + cw2 - 92, y + 74, 72, 22), num(fin) + L"/" + num(all2) + L"회", f.sm, INK,
                 DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        cv->text(R(x + 22, y + 104, 110, 22), L"커리큘럼 진도", f.sm, SUB);
        progress(R(x + 120, y + 112, cw2 - 220, 8), static_cast<int>(learned.size()), total, GREEN);
        cv->text(R(x + cw2 - 92, y + 104, 72, 22), num(static_cast<int>(learned.size())) + L"/" + num(total) + L"개",
                 f.sm, INK, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        cv->text(R(x + 22, y + 142, cw2 - 180, 22), L"휴강 " + num(canc) + L"회  ·  보강 " + num(makeup) + L"회", f.sm, SUB);
        button(R(x + cw2 - 120, y + 138, 100, 32), L"수업 계획", S_T_BLUE, A_STU_PLAN, s.getId());
        ++i;
    }
}

// ======================================================================
//  설정
// ======================================================================
void MainWindow::drawSettings() {
    header(L"설정", L"자동 배분 방식과 데이터를 관리합니다.");
    const int CW = std::min(860, viewW - X0 - 32);
    card(R(X0, TOP, CW, 5 * 76 + 20));
    int y = TOP + 10;
    auto row = [&](const std::wstring& title, const std::wstring& desc) {
        cv->text(R(X0 + 24, y + 12, CW - 320, 24), title, f.bodyB, INK);
        cv->text(R(X0 + 24, y + 38, CW - 320, 20), desc, f.sm, SUB);
    };

    row(L"주말에도 준비 To-Do 배정", L"끄면 평일에 우선 배분하고, 평일이 없을 때만 주말을 사용합니다.");
    bool on = app.isWeekendPrepAllowed();
    RECT sw = R(X0 + CW - 24 - 52, y + 24, 52, 28);
    cv->round(sw, 14, on ? BLUE : GRAY_M);
    cv->circle(on ? sw.right - 14 : sw.left + 14, sw.top + 14, 11, WHITE);
    zone(sw, A_SET_WEEKEND);
    y += 76;
    cv->line(X0 + 20, y, X0 + CW - 20, y, LINEC);

    row(L"기준 날짜 (시연/테스트용)", L"현재: " + longDate(app.today()) +
                                         (app.isTodayOverridden() ? L"  [변경됨]" : L"  [실제 날짜]"));
    button(R(X0 + CW - 24 - 110, y + 20, 110, 36), L"실제 날짜로", S_SECONDARY, A_SET_REAL);
    button(R(X0 + CW - 24 - 110 - 8 - 80, y + 20, 80, 36), L"변경", S_T_BLUE, A_SET_TODAY);
    y += 76;
    cv->line(X0 + 20, y, X0 + CW - 20, y, LINEC);

    row(L"샘플 데이터 불러오기", L"커리큘럼 14개, 학생 4명으로 기능을 둘러봅니다. (현재 데이터는 지워짐)");
    button(R(X0 + CW - 24 - 110, y + 20, 110, 36), L"불러오기", S_SECONDARY, A_SET_SAMPLE);
    y += 76;
    cv->line(X0 + 20, y, X0 + CW - 20, y, LINEC);

    row(L"지금 저장", L"저장 파일: " + W(dataPath) + L" (실행 폴더). 변경할 때마다 자동 저장됩니다.");
    button(R(X0 + CW - 24 - 110, y + 20, 110, 36), L"저장", S_SECONDARY, A_SET_SAVE);
    y += 76;
    cv->line(X0 + 20, y, X0 + CW - 20, y, LINEC);

    row(L"모든 데이터 삭제", L"커리큘럼, 학생, 수업, To-Do를 모두 지웁니다. 되돌릴 수 없습니다.");
    button(R(X0 + CW - 24 - 110, y + 20, 110, 36), L"전체 삭제", S_T_RED, A_SET_CLEAR);
}

// ======================================================================
//  동작 처리
// ======================================================================
void MainWindow::dispatch(int act, int a, int b) {
    switch (act) {
        case A_NAV: go(static_cast<Page>(a)); return;

        // ---- 커리큘럼 ----
        case A_CUR_ADD: curriculumDialog(nullptr); return;
        case A_CUR_MENU: curriculumMenu(a); return;

        // ---- 수업 계획 ----
        case A_PLAN_PICK: pickStudentMenu(); return;
        case A_PLAN_PREV:
        case A_PLAN_NEXT: {
            const auto& list = app.getStudents();
            if (list.empty()) return;
            int idx = studentIndex(selStudent);
            int n = static_cast<int>(list.size());
            idx = (idx + (act == A_PLAN_NEXT ? 1 : n - 1)) % n;
            selStudent = list[static_cast<size_t>(idx)].getId();
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }
        case A_PLAN_EDIT_STUDENT:
            if (const Student* s = app.findStudent(selStudent)) {
                Student copy = *s;
                studentDialog(&copy);
            }
            return;
        case A_PLAN_GEN: {
            const Student* s = app.findStudent(selStudent);
            if (!s) return;
            int possible = app.countPossibleLessons(selStudent, app.today());
            std::wstring msg = L"오늘(" + W(app.today().toString()) + L")부터 목표일까지 " + W(s->daysToString()) +
                               L"요일마다 수업 " + num(possible) + L"회를 만들고,\n전체 커리큘럼(" +
                               num(static_cast<int>(app.getCurricula().size())) +
                               L"개)을 순서대로 배치합니다.\n\n오늘 이후 완료되지 않은 수업은 새로 만들어집니다. 계속할까요?";
            if (!ask(msg)) return;
            int c = app.generateLessons(selStudent, app.today());
            int asg = app.autoAssignCurriculum(selStudent);
            refresh();
            std::wstring res = L"수업 " + num(c) + L"회를 만들고 커리큘럼 " + num(asg) + L"개를 배치했습니다.";
            if (static_cast<size_t>(c) < app.getCurricula().size())
                res += L"\n\n수업 횟수가 커리큘럼보다 적어 일부 커리큘럼은 배치되지 않았습니다.";
            info(res);
            return;
        }
        case A_PLAN_ASSIGN:
            if (ask(L"완료되지 않은 수업의 커리큘럼을 전체 커리큘럼 순서대로 다시 배치할까요?\n(이미 완료한 커리큘럼은 제외됩니다)")) {
                app.autoAssignCurriculum(selStudent);
                refresh();
            }
            return;
        case A_PLAN_RESET:
            if (ask(L"이 학생의 완료되지 않은 수업과 준비 To-Do를 모두 삭제할까요?")) {
                app.resetPlan(selStudent);
                refresh();
            }
            return;
        case A_PLAN_CUR: chooseCurriculum(a); return;
        case A_LESSON_MENU: lessonMenu(a); return;
        case A_PLAN_ADD_LESSON: {
            const Student* s = app.findStudent(selStudent);
            addLessonDialog(selStudent, app.today(), s ? s->getStartHour() : 19);
            return;
        }

        // ---- 학생 ----
        case A_STU_ADD: studentDialog(nullptr); return;
        case A_STU_EDIT:
            if (const Student* s = app.findStudent(a)) {
                Student copy = *s;
                studentDialog(&copy);
            }
            return;
        case A_STU_DEL:
            if (ask(studentName(a) + L" 학생과 모든 수업, 준비 To-Do를 삭제할까요?")) {
                app.removeStudent(a);
                refresh();
            }
            return;
        case A_STU_PLAN:
            selStudent = a;
            go(Page::Curriculum);
            return;

        // ---- 주간 이동 / 달력 ----
        case A_WEEK_PREV:
        case A_WEEK_NEXT:
        case A_WEEK_TODAY: {
            viewWeek = act == A_WEEK_TODAY ? app.today().weekStart() : viewWeek.addDays(act == A_WEEK_PREV ? -7 : 7);
            Date th = viewWeek.addDays(3);   // 달력도 그 주가 속한 달로
            calYear = th.year();
            calMonth = th.month();
            break;
        }
        case A_CAL_PREV:
            if (--calMonth < 1) { calMonth = 12; --calYear; }
            break;
        case A_CAL_NEXT:
            if (++calMonth > 12) { calMonth = 1; ++calYear; }
            break;
        case A_CAL_DAY:
            viewWeek = fromSerial(a).weekStart();
            if (page != Page::Weekly && page != Page::Prep) go(Page::Weekly);
            break;

        // ---- 시간표 ----
        case A_SELECT_LESSON: selLesson = a; break;
        case A_CELL_ADD: {
            int sid = app.getStudents().empty() ? -1 : app.getStudents()[0].getId();
            if (app.findStudent(selStudent)) sid = selStudent;
            addLessonDialog(sid, fromSerial(a), b);
            return;
        }
        case A_QUICK: {
            const Lesson* l = app.findLesson(selLesson);
            if (a == 0) {
                int sid = l ? l->getStudentId() : (app.getStudents().empty() ? -1 : app.getStudents()[0].getId());
                Date d = viewWeek.weekStart() == app.today().weekStart() ? app.today() : viewWeek;
                const Student* s = app.findStudent(sid);
                addLessonDialog(sid, d, s ? s->getStartHour() : 19);
                return;
            }
            if (!l) return;
            if (a == 1) makeupDialog(l->getId());
            else if (a == 2) cancelDialog(l->getId());
            else if (a == 3) { app.setLessonCompleted(l->getId(), !l->isCompleted()); refresh(); }
            else if (a == 4) chooseCurriculum(l->getId());
            else if (a == 5 && ask(L"선택한 일정을 삭제할까요?")) { app.deleteLesson(l->getId()); selLesson = -1; refresh(); }
            return;
        }

        // ---- 수업 준비 ----
        case A_TASK_TOGGLE: app.toggleTask(a); refresh(); return;
        case A_TASK_MENU: taskMenu(a); return;
        case A_PREP_REGEN: {
            if (!ask(L"이번 주(" + md(viewWeek) + L" ~ " + md(viewWeek.addDays(6)) +
                     L") 수업의 미완료 준비 To-Do를 지우고 다시 배분할까요?\n(이미 완료한 준비는 유지됩니다)"))
                return;
            auto r = app.generatePrepPlan(viewWeek);
            refresh();
            info(L"대상 수업 " + num(r.targetLessons) + L"회\n새로 만든 To-Do " + num(r.created) + L"개\n이미 준비 완료 " +
                 num(r.keptDone) + L"개\n지난 수업 내용이라 제외 " + num(r.skippedTaught) + L"개");
            return;
        }
        case A_PREP_SETTINGS: {
            HMENU m = CreatePopupMenu();
            AppendMenuW(m, MF_STRING | (app.isWeekendPrepAllowed() ? MF_CHECKED : 0U), 1, L"주말에도 준비 To-Do 배정");
            AppendMenuW(m, MF_STRING | (app.isWeekendPrepAllowed() ? 0U : MF_CHECKED), 2, L"평일 위주로 배정");
            int cmd = popup(m);
            DestroyMenu(m);
            if (cmd == 1) app.setWeekendPrep(true);
            if (cmd == 2) app.setWeekendPrep(false);
            if (cmd) refresh();
            return;
        }
        case A_PREP_ROLLOVER: {
            int n = app.rollOverOverdueTasks();
            refresh();
            info(L"밀린 준비 To-Do " + num(n) + L"개를 오늘로 옮겼습니다.");
            return;
        }

        // ---- 설정 ----
        case A_SET_WEEKEND: app.setWeekendPrep(!app.isWeekendPrepAllowed()); refresh(); return;
        case A_SET_TODAY: todayDialog(); return;
        case A_SET_REAL:
            app.clearTodayOverride();
            viewWeek = app.today().weekStart();
            refresh();
            return;
        case A_SET_SAMPLE:
            if (ask(L"현재 데이터를 지우고 샘플 데이터를 불러올까요?")) {
                app.loadSampleData();
                viewWeek = app.today().weekStart();
                selStudent = selLesson = -1;
                refresh();
            }
            return;
        case A_SET_CLEAR:
            if (ask(L"정말 모든 데이터를 삭제할까요? 되돌릴 수 없습니다.")) {
                app.clearAll();
                selStudent = selLesson = -1;
                refresh();
            }
            return;
        case A_SET_SAVE:
            info(app.save(dataPath) ? L"저장했습니다." : L"저장에 실패했습니다.");
            return;
        default:
            return;
    }
    InvalidateRect(hwnd, nullptr, FALSE);
}

int MainWindow::popup(HMENU m) {
    POINT p;
    GetCursorPos(&p);
    return static_cast<int>(TrackPopupMenu(m, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY, p.x, p.y, 0, hwnd, nullptr));
}

void MainWindow::info(const std::wstring& msg) { MessageBoxW(hwnd, msg.c_str(), L"TutorMate", MB_OK | MB_ICONINFORMATION); }

bool MainWindow::ask(const std::wstring& msg) {
    return MessageBoxW(hwnd, msg.c_str(), L"TutorMate", MB_YESNO | MB_ICONQUESTION) == IDYES;
}

// 수업 하나에 대한 메뉴 (완료 / 커리큘럼 / 휴강 / 보강 / 삭제)
void MainWindow::lessonMenu(int lessonId) {
    const Lesson* l = app.findLesson(lessonId);
    if (!l) return;
    selLesson = lessonId;
    const auto& curs = app.getCurricula();
    HMENU m = CreatePopupMenu();
    HMENU sub = CreatePopupMenu();
    AppendMenuW(sub, MF_STRING | (l->getCurriculumId() < 0 ? MF_CHECKED : 0U), 999, L"(미배정)");
    for (size_t i = 0; i < curs.size(); ++i)
        AppendMenuW(sub, MF_STRING | (curs[i].getId() == l->getCurriculumId() ? MF_CHECKED : 0U),
                    static_cast<UINT_PTR>(1000 + i), W("[" + curs[i].getCategory() + "] " + curs[i].getTitle()).c_str());

    const UINT dis = l->isCancelled() ? MF_GRAYED : 0U;
    AppendMenuW(m, MF_STRING | MF_GRAYED, 0, (mdw(l->getDate()) + L"  " + studentName(l->getStudentId())).c_str());
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING | dis, 1, l->isCompleted() ? L"완료 취소" : L"완료 처리");
    AppendMenuW(m, MF_POPUP | dis, reinterpret_cast<UINT_PTR>(sub), L"커리큘럼 변경");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING | ((l->isCancelled() || l->isCompleted()) ? MF_GRAYED : 0U), 2, L"휴강으로 변경...");
    AppendMenuW(m, MF_STRING, 3, L"보강으로 변경 (다른 날로 이동)...");
    AppendMenuW(m, MF_STRING, 5, L"주간 시간표에서 보기");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING, 4, L"일정 삭제");
    int cmd = popup(m);
    DestroyMenu(m);   // 하위 메뉴도 함께 삭제됨

    if (cmd == 1) { app.setLessonCompleted(lessonId, !l->isCompleted()); refresh(); }
    else if (cmd == 2) cancelDialog(lessonId);
    else if (cmd == 3) makeupDialog(lessonId);
    else if (cmd == 4) {
        if (ask(L"이 일정을 삭제할까요?")) { app.deleteLesson(lessonId); selLesson = -1; refresh(); }
    } else if (cmd == 5) {
        viewWeek = l->getDate().weekStart();
        go(Page::Weekly);
    } else if (cmd == 999) { app.assignCurriculum(lessonId, -1); refresh(); }
    else if (cmd >= 1000 && cmd < 1000 + static_cast<int>(curs.size())) {
        app.assignCurriculum(lessonId, curs[static_cast<size_t>(cmd - 1000)].getId());
        refresh();
    } else InvalidateRect(hwnd, nullptr, FALSE);
}

void MainWindow::chooseCurriculum(int lessonId) {
    const Lesson* l = app.findLesson(lessonId);
    if (!l) return;
    const auto& curs = app.getCurricula();
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING | (l->getCurriculumId() < 0 ? MF_CHECKED : 0U), 999, L"(미배정)");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    for (size_t i = 0; i < curs.size(); ++i)
        AppendMenuW(m, MF_STRING | (curs[i].getId() == l->getCurriculumId() ? MF_CHECKED : 0U),
                    static_cast<UINT_PTR>(1000 + i), W("[" + curs[i].getCategory() + "] " + curs[i].getTitle()).c_str());
    int cmd = popup(m);
    DestroyMenu(m);
    if (cmd == 999) app.assignCurriculum(lessonId, -1);
    else if (cmd >= 1000 && cmd < 1000 + static_cast<int>(curs.size()))
        app.assignCurriculum(lessonId, curs[static_cast<size_t>(cmd - 1000)].getId());
    else return;
    refresh();
}

void MainWindow::taskMenu(int taskId) {
    const PrepTask* t = app.findTask(taskId);
    if (!t) return;
    const Lesson* l = app.findLesson(t->getLessonId());
    Date last = l ? l->getDate() : t->getDeadline().addDays(1);
    Date first = std::max(app.today(), last.addDays(-7));
    if (t->getDate() < first) first = t->getDate();

    HMENU m = CreatePopupMenu();
    HMENU sub = CreatePopupMenu();
    std::vector<Date> dates;
    for (Date d = first; d <= last; d = d.addDays(1)) {
        dates.push_back(d);
        std::wstring label = mdw(d);
        if (d == last) label += L"  (수업 당일)";
        else if (d == t->getDeadline()) label += L"  (기한)";
        AppendMenuW(sub, MF_STRING | (d == t->getDate() ? MF_CHECKED : 0U), static_cast<UINT_PTR>(100 + dates.size() - 1),
                    label.c_str());
    }
    AppendMenuW(m, MF_STRING | MF_GRAYED, 0, (studentName(t->getStudentId()) + L" - " + W(t->getTitle())).c_str());
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING, 1, t->isCompleted() ? L"완료 취소" : L"완료 표시");
    AppendMenuW(m, MF_POPUP, reinterpret_cast<UINT_PTR>(sub), L"날짜 이동");
    if (l) AppendMenuW(m, MF_STRING, 2, L"연결된 수업 보기");
    int cmd = popup(m);
    DestroyMenu(m);
    if (cmd == 1) { app.toggleTask(taskId); refresh(); }
    else if (cmd == 2 && l) lessonMenu(l->getId());
    else if (cmd >= 100 && cmd < 100 + static_cast<int>(dates.size())) {
        std::string msg;
        app.moveTask(taskId, dates[static_cast<size_t>(cmd - 100)], msg);
        refresh();
        if (!msg.empty()) info(W(msg));
    }
}

void MainWindow::curriculumMenu(int index) {
    const auto& curs = app.getCurricula();
    if (index < 0 || index >= static_cast<int>(curs.size())) return;
    const Curriculum c = curs[static_cast<size_t>(index)];
    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING, 1, L"수정...");
    AppendMenuW(m, MF_STRING | (index == 0 ? MF_GRAYED : 0U), 2, L"위로 이동");
    AppendMenuW(m, MF_STRING | (index + 1 == static_cast<int>(curs.size()) ? MF_GRAYED : 0U), 3, L"아래로 이동");
    AppendMenuW(m, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m, MF_STRING, 4, L"삭제");
    int cmd = popup(m);
    DestroyMenu(m);
    size_t i = static_cast<size_t>(index);
    if (cmd == 1) curriculumDialog(&c);
    else if (cmd == 2) { app.moveCurriculum(i, i - 1); refresh(); }
    else if (cmd == 3) { app.moveCurriculum(i, i + 1); refresh(); }
    else if (cmd == 4 && ask(L"'" + W(c.getTitle()) + L"'을(를) 삭제할까요?\n배치된 수업은 미배정이 됩니다.")) {
        app.removeCurriculum(c.getId());
        refresh();
    }
}

void MainWindow::curriculumDialog(const Curriculum* base) {
    FormDialog dlg(base ? L"커리큘럼 수정" : L"커리큘럼 추가");
    int iCat = dlg.addComboEdit(L"분류 (목록에서 고르거나 직접 입력)", {L"독서", L"문학", L"언매", L"실전", L"기타"},
                                base ? W(base->getCategory()) : L"독서");
    int iTitle = dlg.addText(L"커리큘럼 제목", base ? W(base->getTitle()) : L"");
    int iDur = dlg.addCombo(L"소요 시간", lengthOptions(), base ? clampi(base->getDuration() - 1, 0, 3) : 1);
    while (dlg.show(hwnd, f.ctrl, scale)) {
        if (dlg.text(iTitle).empty()) { info(L"제목을 입력해 주세요."); continue; }
        std::string cat = U(dlg.text(iCat));
        if (cat.empty()) cat = "기타";
        if (base) app.updateCurriculum(base->getId(), cat, U(dlg.text(iTitle)), dlg.selected(iDur) + 1);
        else app.addCurriculum(cat, U(dlg.text(iTitle)), dlg.selected(iDur) + 1);
        refresh();
        return;
    }
}

void MainWindow::studentDialog(const Student* base) {
    FormDialog dlg(base ? L"학생 정보 수정" : L"학생 추가");
    Date defTarget = base ? base->getTargetDate() : app.today().addDays(70);
    int iName = dlg.addText(L"이름", base ? W(base->getName()) : L"");
    int iInfo = dlg.addText(L"학년 / 과목 (예: 고3 국어)", base ? W(base->getInfo()) : L"");
    int iTarget = dlg.addText(L"목표일 (시험일, 예: 2027-01-20)", W(defTarget.toString()));
    int iDays = dlg.addText(L"수업 요일 (예: 화목, 월 수 금)", base ? W(base->daysToString()) : L"");
    int iHour = dlg.addCombo(L"수업 시작 시각", hourOptions(), clampi((base ? base->getStartHour() : 19) - 6, 0, 17));
    int iLen = dlg.addCombo(L"1회 수업 시간", lengthOptions(), clampi((base ? base->getLessonHours() : 2) - 1, 0, 3));
    while (dlg.show(hwnd, f.ctrl, scale)) {
        Date target;
        std::vector<int> days = Student::parseDays(U(dlg.text(iDays)));
        if (dlg.text(iName).empty()) { info(L"이름을 입력해 주세요."); continue; }
        if (!Date::tryParse(U(dlg.text(iTarget)), target, app.today().year())) {
            info(L"목표일 형식이 올바르지 않습니다. 예: 2027-01-20");
            continue;
        }
        if (days.empty()) { info(L"수업 요일을 하나 이상 입력해 주세요. (월화수목금토일)"); continue; }
        int hour = dlg.selected(iHour) + 6, len = dlg.selected(iLen) + 1;
        std::string name = U(dlg.text(iName)), infoS = U(dlg.text(iInfo));

        if (!base) {
            int id = app.addStudent(name, infoS, target, days, hour, len);
            selStudent = id;
            refresh();
            if (ask(W(name) + L" 학생을 추가했습니다.\n\n지금 바로 수업 일정(" +
                    num(app.countPossibleLessons(id, app.today())) + L"회)을 만들고 커리큘럼을 자동 배치할까요?")) {
                app.generateLessons(id, app.today());
                app.autoAssignCurriculum(id);
                go(Page::Curriculum);
                refresh();
            }
        } else {
            bool changed = days != base->getLessonDays() || hour != base->getStartHour() ||
                           len != base->getLessonHours() || target != base->getTargetDate();
            app.updateStudent(base->getId(), name, infoS, target, days, hour, len);
            refresh();
            if (changed && ask(L"수업 요일 / 시간 / 목표일이 바뀌었습니다.\n오늘 이후 수업 일정을 다시 만들까요?")) {
                app.generateLessons(base->getId(), app.today());
                app.autoAssignCurriculum(base->getId());
                refresh();
            }
        }
        return;
    }
}

void MainWindow::addLessonDialog(int studentId, const Date& day, int hour) {
    const auto& list = app.getStudents();
    if (list.empty()) {
        info(L"먼저 학생을 등록해 주세요.");
        return;
    }
    std::vector<std::wstring> names;
    int sel = 0;
    for (size_t i = 0; i < list.size(); ++i) {
        names.push_back(W(list[i].getName() + " (" + list[i].getInfo() + ")"));
        if (list[i].getId() == studentId) sel = static_cast<int>(i);
    }
    std::vector<std::wstring> curNames{L"(미배정)"};
    for (const Curriculum& c : app.getCurricula()) curNames.push_back(W("[" + c.getCategory() + "] " + c.getTitle()));

    FormDialog dlg(L"수업 추가");
    int iStu = dlg.addCombo(L"학생", names, sel);
    int iDate = dlg.addText(L"날짜 (예: 2026-12-01)", W(day.toString()));
    int iHour = dlg.addCombo(L"시작 시각", hourOptions(), clampi(hour - 6, 0, 17));
    int iLen = dlg.addCombo(L"수업 시간", lengthOptions(), clampi(list[static_cast<size_t>(sel)].getLessonHours() - 1, 0, 3));
    int iCur = dlg.addCombo(L"커리큘럼", curNames, 0);
    int iMk = dlg.addCheck(L"보강 수업으로 표시", false);
    while (dlg.show(hwnd, f.ctrl, scale)) {
        Date d;
        if (!Date::tryParse(U(dlg.text(iDate)), d, app.today().year())) {
            info(L"날짜 형식이 올바르지 않습니다. 예: 2026-12-01");
            continue;
        }
        int sid = list[static_cast<size_t>(dlg.selected(iStu))].getId();
        int cs = dlg.selected(iCur);
        int curId = cs == 0 ? -1 : app.getCurricula()[static_cast<size_t>(cs - 1)].getId();
        std::string warn;
        int id = app.addLesson(sid, d, dlg.selected(iHour) + 6, dlg.selected(iLen) + 1, curId,
                               dlg.checked(iMk) ? LessonStatus::Makeup : LessonStatus::Normal, warn);
        selLesson = id;
        viewWeek = d.weekStart();
        refresh();
        if (!warn.empty()) info(L"추가했습니다. 다만 " + W(warn));
        return;
    }
}

void MainWindow::cancelDialog(int lessonId) {
    const Lesson* l = app.findLesson(lessonId);
    if (!l) return;
    if (l->isCompleted()) { info(L"완료된 수업은 휴강으로 바꿀 수 없습니다. 먼저 완료를 취소해 주세요."); return; }
    if (l->isCancelled()) { info(L"이미 휴강된 수업입니다."); return; }
    FormDialog dlg(L"휴강으로 변경", mdw(l->getDate()) + L" " + studentName(l->getStudentId()) + L" - " + W(l->getTitle()));
    int iReason = dlg.addText(L"휴강 사유", L"개인 사정");
    int iShift = dlg.addCheck(L"이 수업의 커리큘럼을 다음 수업으로 미루기", l->getCurriculumId() >= 0);
    if (!dlg.show(hwnd, f.ctrl, scale)) return;
    std::string msg;
    app.cancelLesson(lessonId, U(dlg.text(iReason)), dlg.checked(iShift), msg);
    refresh();
    if (!msg.empty()) info(W(msg));
}

void MainWindow::makeupDialog(int lessonId) {
    const Lesson* l = app.findLesson(lessonId);
    if (!l) return;
    FormDialog dlg(L"보강으로 변경", L"원래 수업(" + mdw(l->getDate()) + L")은 휴강으로 바뀌고, 새 날짜에 보강 수업이 생깁니다.");
    int iDate = dlg.addText(L"보강 날짜 (예: 2026-12-05)", W(l->getDate().addDays(1).toString()));
    int iHour = dlg.addCombo(L"시작 시각", hourOptions(), clampi(l->getStartHour() - 6, 0, 17));
    while (dlg.show(hwnd, f.ctrl, scale)) {
        Date d;
        if (!Date::tryParse(U(dlg.text(iDate)), d, app.today().year())) {
            info(L"날짜 형식이 올바르지 않습니다. 예: 2026-12-05");
            continue;
        }
        std::string msg;
        int id = app.makeupLesson(lessonId, d, dlg.selected(iHour) + 6, msg);
        selLesson = id;
        viewWeek = d.weekStart();
        refresh();
        if (!msg.empty()) info(L"보강 일정을 만들었습니다. 다만 " + W(msg));
        return;
    }
}

void MainWindow::pickStudentMenu() {
    const auto& list = app.getStudents();
    HMENU m = CreatePopupMenu();
    for (size_t i = 0; i < list.size(); ++i)
        AppendMenuW(m, MF_STRING | (list[i].getId() == selStudent ? MF_CHECKED : 0U), static_cast<UINT_PTR>(1 + i),
                    W(list[i].getName() + "  (" + list[i].getInfo() + ")").c_str());
    int cmd = popup(m);
    DestroyMenu(m);
    if (cmd >= 1 && cmd <= static_cast<int>(list.size())) {
        selStudent = list[static_cast<size_t>(cmd - 1)].getId();
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

void MainWindow::todayDialog() {
    FormDialog dlg(L"기준 날짜 변경", L"발표/시연할 때 원하는 날짜를 '오늘'로 바꿔서 볼 수 있습니다.");
    int iDate = dlg.addText(L"기준 날짜 (예: 2026-09-01)", W(app.today().toString()));
    while (dlg.show(hwnd, f.ctrl, scale)) {
        Date d;
        if (!Date::tryParse(U(dlg.text(iDate)), d, app.today().year())) {
            info(L"날짜 형식이 올바르지 않습니다.");
            continue;
        }
        app.setTodayOverride(d);
        viewWeek = d.weekStart();
        calYear = d.year();
        calMonth = d.month();
        refresh();
        return;
    }
}

// ======================================================================
//  기타
// ======================================================================
std::wstring MainWindow::studentName(int id) const {
    const Student* s = app.findStudent(id);
    return s ? W(s->getName()) : L"(삭제됨)";
}

int MainWindow::studentIndex(int id) const {
    const auto& list = app.getStudents();
    for (size_t i = 0; i < list.size(); ++i)
        if (list[i].getId() == id) return static_cast<int>(i);
    return 0;
}

std::wstring MainWindow::dday(const Date& d) const {
    int n = d - app.today();
    if (n > 0) return L"D-" + num(n);
    if (n == 0) return L"D-Day";
    return L"D+" + num(-n);
}
