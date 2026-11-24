#include "Student.h"
#include <algorithm>

Student::Student(int id, std::string n, std::string info, Date target,
                 std::vector<int> days, int startHour, int lessonHours)
    : id(id), name(std::move(n)), info(std::move(info)), targetDate(target),
      startHour(startHour), lessonHours(lessonHours) {
    setLessonDays(days);
}

bool Student::hasLessonOn(int weekday) const {
    return std::find(lessonDays.begin(), lessonDays.end(), weekday) != lessonDays.end();
}

void Student::setLessonDays(const std::vector<int>& days) {
    lessonDays.clear();
    for (int d : days)
        if (d >= 0 && d <= 6 && !hasLessonOn(d)) lessonDays.push_back(d);
    std::sort(lessonDays.begin(), lessonDays.end());
}

std::string Student::daysToString() const {
    std::string s;
    for (size_t i = 0; i < lessonDays.size(); ++i) {
        if (i) s += ", ";
        s += Date::weekdayName(lessonDays[i]);
    }
    return s.empty() ? "-" : s;
}

std::vector<int> Student::parseDays(const std::string& s) {
    std::vector<int> out;
    for (int i = 0; i < 7; ++i)
        if (s.find(Date::weekdayName(i)) != std::string::npos) out.push_back(i);
    for (char c : s)
        if (c >= '1' && c <= '7') out.push_back(c - '1');   // 1=월 ... 7=일
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

std::string Student::daysToCode(const std::vector<int>& days) {
    std::string s;
    for (size_t i = 0; i < days.size(); ++i) {
        if (i) s += ",";
        s += std::to_string(days[i] + 1);
    }
    return s;
}
