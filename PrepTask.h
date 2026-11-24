#pragma once
#include "ScheduleItem.h"

// 수업 준비 To-Do: date = 준비하기로 배정된 날, deadline = 수업 전날
class PrepTask : public ScheduleItem {
private:
    int lessonId;
    Date deadline;

public:
    PrepTask(int id, int studentId, int lessonId, Date planned, Date deadline, std::string t)
        : ScheduleItem(id, studentId, planned, std::move(t), 1),
          lessonId(lessonId), deadline(deadline) {}

    int getLessonId() const { return lessonId; }
    const Date& getDeadline() const { return deadline; }
    void relink(int newLessonId, const Date& newDeadline) {
        lessonId = newLessonId;
        deadline = newDeadline;
    }
    bool isOverdue(const Date& today) const { return !completed && date < today; }

    std::string typeName() const override { return "준비"; }
    std::string describe() const override {
        return std::string(completed ? "[v] " : "[ ] ") + title + " (기한 " + deadline.shortStr() + ")";
    }
};
