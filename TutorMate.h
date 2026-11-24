#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Curriculum.h"
#include "Lesson.h"
#include "PrepTask.h"
#include "Student.h"

// 앱 전체 데이터와 기능을 관리하는 클래스
class TutorMate {
public:
    struct PrepPlanResult {
        int targetLessons = 0;   // 이번 주 대상 수업
        int created = 0;         // 새로 만든 To-Do
        int keptDone = 0;        // 이미 준비 완료되어 유지
        int skippedTaught = 0;   // 지난 수업 내용이라 준비 불필요
    };

private:
    std::vector<Curriculum> curricula;              // 순서 = 전체 커리큘럼 진행 순서
    std::vector<Student> students;
    std::vector<std::unique_ptr<Lesson>> lessons;
    std::vector<std::unique_ptr<PrepTask>> tasks;

    int nextCurriculumId = 1;
    int nextStudentId = 1;
    int nextItemId = 1;

    bool weekendPrep = false;     // 주말에도 준비 To-Do 배정
    bool useFakeToday = false;    // 시연용 기준 날짜
    Date fakeToday;

    Lesson* findLessonMut(int id);
    PrepTask* findTaskMut(int id);
    Student* findStudentMut(int id);
    std::vector<Lesson*> activeLessonsOf(int studentId);   // 휴강 제외, 날짜순
    void removeTasksOfLesson(int lessonId, bool keepCompleted);
    int taskLoadOn(const Date& d) const;
    std::string makeTaskTitle(const Lesson& l) const;
    void refreshTaskTitles(const Lesson& l);

public:
    // ---------- 설정 ----------
    Date today() const;
    bool isTodayOverridden() const { return useFakeToday; }
    void setTodayOverride(const Date& d) { useFakeToday = true; fakeToday = d; }
    void clearTodayOverride() { useFakeToday = false; }
    bool isWeekendPrepAllowed() const { return weekendPrep; }
    void setWeekendPrep(bool v) { weekendPrep = v; }

    // ---------- 커리큘럼 ----------
    const std::vector<Curriculum>& getCurricula() const { return curricula; }
    const Curriculum* findCurriculum(int id) const;
    std::string curriculumTitle(int id) const;
    int addCurriculum(const std::string& category, const std::string& title, int duration);
    bool updateCurriculum(int id, const std::string& category, const std::string& title, int duration);
    bool removeCurriculum(int id);
    bool moveCurriculum(size_t from, size_t to);

    // ---------- 학생 ----------
    const std::vector<Student>& getStudents() const { return students; }
    const Student* findStudent(int id) const;
    int addStudent(const std::string& name, const std::string& info, const Date& target,
                   const std::vector<int>& days, int startHour, int hours);
    bool updateStudent(int id, const std::string& name, const std::string& info, const Date& target,
                       const std::vector<int>& days, int startHour, int hours);
    bool removeStudent(int id);

    // ---------- 수업 (기능 1: 커리큘럼 할당 / 부가기능: 주간 시간표) ----------
    const Lesson* findLesson(int id) const;
    std::vector<const Lesson*> lessonsOf(int studentId, bool includeCancelled = true) const;
    std::vector<const Lesson*> lessonsBetween(const Date& from, const Date& to) const;
    int countPossibleLessons(int studentId, const Date& from) const;
    int generateLessons(int studentId, const Date& from);      // 요일 기반 자동 일정 생성
    int autoAssignCurriculum(int studentId);                   // 전체 커리큘럼 순서대로 배치
    bool assignCurriculum(int lessonId, int curriculumId);
    int addLesson(int studentId, const Date& d, int startHour, int hours, int curriculumId,
                  LessonStatus status, std::string& warning);
    bool cancelLesson(int lessonId, const std::string& reason, bool shiftCurriculum, std::string& msg);
    int makeupLesson(int lessonId, const Date& newDate, int newHour, std::string& msg);
    bool deleteLesson(int lessonId);
    bool setLessonCompleted(int lessonId, bool done);
    int resetPlan(int studentId);                              // 완료되지 않은 수업 모두 삭제
    std::string conflictWarning(int studentId, const Date& d, int startHour, int hours, int ignoreId) const;

    // ---------- 수업 준비 To-Do (기능 2) ----------
    bool needsPrep(const Lesson& lesson) const;                // 지난 수업 내용이면 false
    PrepPlanResult generatePrepPlan(const Date& weekStart);
    std::vector<const PrepTask*> tasksBetween(const Date& from, const Date& to) const;
    std::vector<const PrepTask*> tasksOfLesson(int lessonId) const;
    const PrepTask* findTask(int id) const;
    bool toggleTask(int taskId);
    bool moveTask(int taskId, const Date& newDate, std::string& msg);
    int rollOverOverdueTasks();

    // 날짜 하나에 걸린 모든 일정 (수업 + 준비) - ScheduleItem 다형성 사용
    std::vector<const ScheduleItem*> itemsOn(const Date& d) const;

    // ---------- 저장 / 불러오기 ----------
    bool save(const std::string& path) const;
    int load(const std::string& path, std::string& error);     // 1=성공, 0=파일 없음, -1=오류
    void clearAll();
    void loadSampleData();
};
