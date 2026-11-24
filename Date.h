#pragma once
#include <string>

// 날짜 계산 전용 클래스 (요일, 날짜 더하기, 주 시작일 등)
class Date {
private:
    int y, m, d;

    static long daysFromCivil(int y, int m, int d);
    static void civilFromDays(long z, int& y, int& m, int& d);

public:
    Date(int year = 2026, int month = 1, int day = 1);

    static Date today();
    static bool isValid(int y, int m, int d);
    static int daysInMonth(int y, int m);
    // "2026-09-01", "9-1", "9/1" (올해 기준) 형식 지원
    static bool tryParse(const std::string& s, Date& out, int defaultYear);

    int year() const { return y; }
    int month() const { return m; }
    int day() const { return d; }

    long serial() const;          // 1970-01-01 기준 일수
    Date addDays(int n) const;
    int weekday() const;          // 0=월 ... 6=일
    bool isWeekend() const { return weekday() >= 5; }
    Date weekStart() const;       // 해당 주 월요일

    std::string toString() const; // 2026-09-01
    std::string shortStr() const; // 09/01
    std::string weekdayName() const;
    static std::string weekdayName(int wd);

    int operator-(const Date& o) const { return static_cast<int>(serial() - o.serial()); }
    bool operator==(const Date& o) const { return serial() == o.serial(); }
    bool operator!=(const Date& o) const { return !(*this == o); }
    bool operator<(const Date& o) const { return serial() < o.serial(); }
    bool operator<=(const Date& o) const { return serial() <= o.serial(); }
    bool operator>(const Date& o) const { return serial() > o.serial(); }
    bool operator>=(const Date& o) const { return serial() >= o.serial(); }
};
