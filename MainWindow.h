#pragma once
#include <string>
#include <vector>
#include "Canvas.h"
#include "TutorMate.h"

// 메인 창: 왼쪽 사이드바 + 오른쪽 화면(페이지). 데이터 처리는 TutorMate에 맡긴다.
class MainWindow {
public:
    enum class Page { Dashboard, Curriculum, Students, Weekly, Prep, Stats, Settings };

private:
    // 화면에서 클릭 가능한 영역
    struct Zone {
        RECT r;
        int act;
        int a;
        int b;
    };
    struct Fonts {
        HFONT logo, h1, h2, body, bodyB, sm, smB, big, tiny, ctrl;
    };

    TutorMate app;
    std::string dataPath;
    HWND hwnd = nullptr;
    double scale = 1.0;
    Fonts f{};

    Page page = Page::Dashboard;
    int scrollY = 0;
    int contentBottom = 0;
    int viewW = 1280, viewH = 800;
    std::vector<Zone> zones;
    Canvas* cv = nullptr;   // 그리는 동안만 유효

    int selStudent = -1;   // 수업 계획 화면에서 고른 학생
    int selLesson = -1;    // 주간 시간표에서 고른 수업
    Date viewWeek;         // 주간 시간표 / 수업 준비에서 보는 주의 월요일
    int calYear = 2026, calMonth = 1;

    // ---- 창 처리 ----
    static LRESULT CALLBACK wndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT handle(UINT msg, WPARAM wp, LPARAM lp);
    void createFonts();
    void deleteFonts();
    void paint();
    void onClick(int x, int y);
    const Zone* hit(int x, int y) const;
    void refresh();                 // 저장 + 다시 그리기
    void go(Page p);

    // ---- 그리기 도우미 ----
    void zone(const RECT& r, int act, int a = 0, int b = 0);
    void card(const RECT& r);
    void header(const std::wstring& title, const std::wstring& sub);
    void button(const RECT& r, const std::wstring& label, int style, int act, int a = 0, int b = 0);
    int pill(int x, int y, const std::wstring& s, COLORREF bg, COLORREF fg, HFONT font = nullptr);
    void donut(int cx, int cy, int r, int done, int total, const std::wstring& caption);
    void progress(const RECT& r, int done, int total, COLORREF c);
    void calendar(const RECT& r, bool forPlan);
    void weekBar(int x, int y, int w);

    // ---- 화면 ----
    void drawSidebar();
    void drawDashboard();
    void drawCurriculum();
    void drawStudents();
    void drawWeekly();
    void drawPrep();
    void drawStats();
    void drawSettings();

    // ---- 동작 ----
    void dispatch(int act, int a, int b);
    int popup(HMENU m);
    void info(const std::wstring& msg);
    bool ask(const std::wstring& msg);
    void lessonMenu(int lessonId);
    void taskMenu(int taskId);
    void curriculumMenu(int index);
    void curriculumDialog(const Curriculum* base);
    void studentDialog(const Student* base);
    void addLessonDialog(int studentId, const Date& day, int hour);
    void cancelDialog(int lessonId);
    void makeupDialog(int lessonId);
    void chooseCurriculum(int lessonId);
    void pickStudentMenu();
    void todayDialog();

    std::wstring studentName(int id) const;
    int studentIndex(int id) const;
    std::wstring dday(const Date& d) const;

public:
    explicit MainWindow(std::string path);
    static int run(HINSTANCE inst);
};
