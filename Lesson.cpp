#include "Lesson.h"
#include <cstdio>

Lesson::Lesson(int id, int studentId, Date d, int startHour, int duration)
    : ScheduleItem(id, studentId, d, "(미배정)", duration),
      startHour(startHour), curriculumId(-1), status(LessonStatus::Normal),
      hasOrigin(false), originDate(d) {}

void Lesson::setCurriculum(int curId, const std::string& curTitle) {
    curriculumId = curId;
    title = (curId < 0) ? "(미배정)" : curTitle;
}

bool Lesson::overlaps(const Lesson& o) const {
    if (date != o.date) return false;
    return startHour < o.getEndHour() && o.startHour < getEndHour();
}

std::string Lesson::timeRange() const {
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%02d:00-%02d:00", startHour, getEndHour());
    return buf;
}

std::string Lesson::statusName() const {
    if (status == LessonStatus::Cancelled) return "휴강";
    if (completed) return "완료";
    if (status == LessonStatus::Makeup) return "보강";
    return "예정";
}

std::string Lesson::describe() const {
    std::string s = timeRange() + " ";
    if (status == LessonStatus::Cancelled) s += "[휴강] ";
    else if (status == LessonStatus::Makeup) s += "[보강] ";
    s += title;
    if (completed) s += " (완료)";
    return s;
}
