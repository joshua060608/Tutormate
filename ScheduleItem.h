#pragma once
#include <string>
#include <utility>
#include "Date.h"

// 수업(Lesson)과 준비 일정(PrepTask)의 공통 정보를 담는 상위 클래스
class ScheduleItem {
protected:
    int id;
    int studentId;
    Date date;
    std::string title;
    int duration;      // 시간 단위
    bool completed;

public:
    ScheduleItem(int id, int studentId, Date d, std::string t, int time)
        : id(id), studentId(studentId), date(d), title(std::move(t)),
          duration(time), completed(false) {}
    virtual ~ScheduleItem() = default;

    int getId() const { return id; }
    int getStudentId() const { return studentId; }
    const Date& getDate() const { return date; }
    const std::string& getTitle() const { return title; }
    int getDuration() const { return duration; }
    bool isCompleted() const { return completed; }

    void setDate(const Date& d) { date = d; }
    void setTitle(const std::string& t) { title = t; }
    void setCompleted(bool c) { completed = c; }

    // 다형성: 종류별로 다르게 표시
    virtual std::string typeName() const = 0;
    virtual std::string describe() const = 0;
};
