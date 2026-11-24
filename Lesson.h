#pragma once
#include "ScheduleItem.h"

enum class LessonStatus { Normal = 0, Makeup = 1, Cancelled = 2 };

// 실제 수업
class Lesson : public ScheduleItem {
private:
    int startHour;
    int curriculumId;      // -1 = 미배정
    LessonStatus status;
    std::string note;      // 휴강 사유 등
    bool hasOrigin;        // 보강 수업이면 원래 날짜 보관
    Date originDate;

public:
    Lesson(int id, int studentId, Date d, int startHour, int duration);

    int getStartHour() const { return startHour; }
    int getEndHour() const { return startHour + duration; }
    int getCurriculumId() const { return curriculumId; }
    LessonStatus getStatus() const { return status; }
    bool isCancelled() const { return status == LessonStatus::Cancelled; }
    const std::string& getNote() const { return note; }
    bool hasOriginDate() const { return hasOrigin; }
    const Date& getOriginDate() const { return originDate; }

    void setStartHour(int h) { startHour = h; }
    void setDuration(int h) { duration = h; }
    void setCurriculum(int curId, const std::string& curTitle);
    void setStatus(LessonStatus s) { status = s; }
    void setNote(const std::string& n) { note = n; }
    void setOrigin(const Date& d) { hasOrigin = true; originDate = d; }

    bool overlaps(const Lesson& o) const;
    std::string timeRange() const;           // "19:00-21:00"
    std::string statusName() const;

    std::string typeName() const override { return "수업"; }
    std::string describe() const override;
};
