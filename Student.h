#pragma once
#include <string>
#include <vector>
#include "Date.h"

// 학생 정보: 목표일(시험일), 수업 요일, 수업 시작 시각, 1회 수업 시간
class Student {
private:
    int id;
    std::string name;
    std::string info;              // 예: "고3 국어"
    Date targetDate;
    std::vector<int> lessonDays;   // 0=월 ... 6=일
    int startHour;
    int lessonHours;

public:
    Student(int id, std::string n, std::string info, Date target,
            std::vector<int> days, int startHour, int lessonHours);

    int getId() const { return id; }
    const std::string& getName() const { return name; }
    const std::string& getInfo() const { return info; }
    const Date& getTargetDate() const { return targetDate; }
    const std::vector<int>& getLessonDays() const { return lessonDays; }
    int getLessonsPerWeek() const { return static_cast<int>(lessonDays.size()); }
    int getStartHour() const { return startHour; }
    int getLessonHours() const { return lessonHours; }

    bool hasLessonOn(int weekday) const;
    std::string daysToString() const;          // "화, 목"

    void setName(const std::string& n) { name = n; }
    void setInfo(const std::string& i) { info = i; }
    void setTargetDate(const Date& d) { targetDate = d; }
    void setLessonDays(const std::vector<int>& days);
    void setStartHour(int h) { if (h >= 0 && h <= 23) startHour = h; }
    void setLessonHours(int h) { if (h > 0 && h <= 8) lessonHours = h; }

    // "화목", "화, 목", "2 4" 등 → {1, 3}
    static std::vector<int> parseDays(const std::string& s);
    static std::string daysToCode(const std::vector<int>& days);   // 저장용 "1,3"
};
