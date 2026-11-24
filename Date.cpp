#include "Date.h"
#include <cstdio>
#include <ctime>
#include <sstream>
#include <vector>

Date::Date(int year, int month, int day) : y(year), m(month), d(day) {
    if (!isValid(y, m, d)) { y = 2026; m = 1; d = 1; }
}

long Date::daysFromCivil(int y, int m, int d) {
    y -= m <= 2;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const long yoe = y - era * 400;
    const long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

void Date::civilFromDays(long z, int& y, int& m, int& d) {
    z += 719468;
    const long era = (z >= 0 ? z : z - 146096) / 146097;
    const long doe = z - era * 146097;
    const long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long yy = yoe + era * 400;
    const long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const long mp = (5 * doy + 2) / 153;
    d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
    m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
    y = static_cast<int>(yy + (m <= 2));
}

Date Date::today() {
    std::time_t t = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &t);
#else
    localtime_r(&t, &local);
#endif
    return Date(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
}

int Date::daysInMonth(int y, int m) {
    static const int dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2) {
        bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
        return leap ? 29 : 28;
    }
    return dim[m - 1];
}

bool Date::isValid(int y, int m, int d) {
    if (y < 1900 || y > 2200 || m < 1 || m > 12) return false;
    return d >= 1 && d <= daysInMonth(y, m);
}

bool Date::tryParse(const std::string& s, Date& out, int defaultYear) {
    std::vector<int> nums;
    std::string cur;
    for (char c : s) {
        if (c >= '0' && c <= '9') cur += c;
        else if (c == '-' || c == '/' || c == '.' || c == ' ') {
            if (!cur.empty()) { nums.push_back(std::stoi(cur)); cur.clear(); }
        } else {
            return false;
        }
        if (cur.size() > 4) return false;
    }
    if (!cur.empty()) nums.push_back(std::stoi(cur));

    int y, m, d;
    if (nums.size() == 3) { y = nums[0]; m = nums[1]; d = nums[2]; }
    else if (nums.size() == 2) { y = defaultYear; m = nums[0]; d = nums[1]; }
    else return false;
    if (y < 100) y += 2000;
    if (!isValid(y, m, d)) return false;
    out = Date(y, m, d);
    return true;
}

long Date::serial() const { return daysFromCivil(y, m, d); }

Date Date::addDays(int n) const {
    int ny, nm, nd;
    civilFromDays(serial() + n, ny, nm, nd);
    return Date(ny, nm, nd);
}

int Date::weekday() const {
    long z = serial();                       // 1970-01-01 = 목요일(3)
    return static_cast<int>(((z % 7) + 7 + 3) % 7);
}

Date Date::weekStart() const { return addDays(-weekday()); }

std::string Date::toString() const {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y, m, d);
    return buf;
}

std::string Date::shortStr() const {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%02d/%02d", m, d);
    return buf;
}

std::string Date::weekdayName(int wd) {
    static const char* names[] = {"월", "화", "수", "목", "금", "토", "일"};
    if (wd < 0 || wd > 6) return "?";
    return names[wd];
}

std::string Date::weekdayName() const { return weekdayName(weekday()); }
