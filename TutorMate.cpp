#include "TutorMate.h"
#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>

namespace {
bool lessonLess(const Lesson* a, const Lesson* b) {
    if (a->getDate() != b->getDate()) return a->getDate() < b->getDate();
    if (a->getStartHour() != b->getStartHour()) return a->getStartHour() < b->getStartHour();
    return a->getId() < b->getId();
}

// 저장 파일 구분자('|')와 줄바꿈이 사용자 입력에 섞이지 않도록 정리
std::string clean(const std::string& s) {
    std::string r = s;
    for (char& c : r)
        if (c == '|' || c == '\n' || c == '\r') c = '/';
    return r;
}

std::vector<std::string> split(const std::string& line, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : line) {
        if (c == sep) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}

int toInt(const std::string& s, int def = 0) {
    try { return std::stoi(s); } catch (...) { return def; }
}

Date toDate(const std::string& s) {
    Date d;
    Date::tryParse(s, d, 2026);
    return d;
}
}  // namespace

// ======================= 설정 =======================
Date TutorMate::today() const { return useFakeToday ? fakeToday : Date::today(); }

// ======================= 커리큘럼 =======================
const Curriculum* TutorMate::findCurriculum(int id) const {
    for (const auto& c : curricula)
        if (c.getId() == id) return &c;
    return nullptr;
}

std::string TutorMate::curriculumTitle(int id) const {
    const Curriculum* c = findCurriculum(id);
    return c ? c->getTitle() : "(미배정)";
}

int TutorMate::addCurriculum(const std::string& category, const std::string& title, int duration) {
    curricula.emplace_back(nextCurriculumId, clean(category), clean(title), duration > 0 ? duration : 2);
    return nextCurriculumId++;
}

bool TutorMate::updateCurriculum(int id, const std::string& category, const std::string& title, int duration) {
    for (auto& c : curricula) {
        if (c.getId() != id) continue;
        c.setCategory(clean(category));
        c.setTitle(clean(title));
        c.setDuration(duration);
        for (auto& l : lessons)
            if (l->getCurriculumId() == id) {
                l->setCurriculum(id, c.getTitle());
                refreshTaskTitles(*l);
            }
        return true;
    }
    return false;
}

bool TutorMate::removeCurriculum(int id) {
    auto it = std::find_if(curricula.begin(), curricula.end(),
                           [id](const Curriculum& c) { return c.getId() == id; });
    if (it == curricula.end()) return false;
    curricula.erase(it);
    for (auto& l : lessons)
        if (l->getCurriculumId() == id) {
            l->setCurriculum(-1, "");
            refreshTaskTitles(*l);
        }
    return true;
}

bool TutorMate::moveCurriculum(size_t from, size_t to) {
    if (from >= curricula.size() || to >= curricula.size()) return false;
    Curriculum c = curricula[from];
    curricula.erase(curricula.begin() + static_cast<long>(from));
    curricula.insert(curricula.begin() + static_cast<long>(to), c);
    return true;
}

// ======================= 학생 =======================
const Student* TutorMate::findStudent(int id) const {
    for (const auto& s : students)
        if (s.getId() == id) return &s;
    return nullptr;
}

Student* TutorMate::findStudentMut(int id) {
    for (auto& s : students)
        if (s.getId() == id) return &s;
    return nullptr;
}

int TutorMate::addStudent(const std::string& name, const std::string& info, const Date& target,
                          const std::vector<int>& days, int startHour, int hours) {
    students.emplace_back(nextStudentId, clean(name), clean(info), target, days, startHour, hours);
    return nextStudentId++;
}

bool TutorMate::updateStudent(int id, const std::string& name, const std::string& info, const Date& target,
                              const std::vector<int>& days, int startHour, int hours) {
    Student* s = findStudentMut(id);
    if (!s) return false;
    s->setName(clean(name));
    s->setInfo(clean(info));
    s->setTargetDate(target);
    s->setLessonDays(days);
    s->setStartHour(startHour);
    s->setLessonHours(hours);
    return true;
}

bool TutorMate::removeStudent(int id) {
    auto it = std::find_if(students.begin(), students.end(),
                           [id](const Student& s) { return s.getId() == id; });
    if (it == students.end()) return false;
    students.erase(it);
    lessons.erase(std::remove_if(lessons.begin(), lessons.end(),
                                 [id](const std::unique_ptr<Lesson>& l) { return l->getStudentId() == id; }),
                  lessons.end());
    tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                               [id](const std::unique_ptr<PrepTask>& t) { return t->getStudentId() == id; }),
                tasks.end());
    return true;
}

// ======================= 수업 =======================
const Lesson* TutorMate::findLesson(int id) const {
    for (const auto& l : lessons)
        if (l->getId() == id) return l.get();
    return nullptr;
}

Lesson* TutorMate::findLessonMut(int id) {
    for (auto& l : lessons)
        if (l->getId() == id) return l.get();
    return nullptr;
}

std::vector<const Lesson*> TutorMate::lessonsOf(int studentId, bool includeCancelled) const {
    std::vector<const Lesson*> out;
    for (const auto& l : lessons)
        if (l->getStudentId() == studentId && (includeCancelled || !l->isCancelled()))
            out.push_back(l.get());
    std::sort(out.begin(), out.end(), lessonLess);
    return out;
}

std::vector<Lesson*> TutorMate::activeLessonsOf(int studentId) {
    std::vector<Lesson*> out;
    for (auto& l : lessons)
        if (l->getStudentId() == studentId && !l->isCancelled()) out.push_back(l.get());
    std::sort(out.begin(), out.end(), lessonLess);
    return out;
}

std::vector<const Lesson*> TutorMate::lessonsBetween(const Date& from, const Date& to) const {
    std::vector<const Lesson*> out;
    for (const auto& l : lessons)
        if (l->getDate() >= from && l->getDate() <= to) out.push_back(l.get());
    std::sort(out.begin(), out.end(), lessonLess);
    return out;
}

int TutorMate::countPossibleLessons(int studentId, const Date& from) const {
    const Student* s = findStudent(studentId);
    if (!s) return 0;
    int count = 0;
    for (Date d = from; d < s->getTargetDate(); d = d.addDays(1))
        if (s->hasLessonOn(d.weekday())) ++count;
    return count;
}

int TutorMate::generateLessons(int studentId, const Date& from) {
    const Student* s = findStudent(studentId);
    if (!s) return 0;

    // 기준일 이후의 미완료 수업은 새로 만든다 (완료된 수업은 기록으로 유지)
    std::vector<int> removed;
    for (const auto& l : lessons)
        if (l->getStudentId() == studentId && l->getDate() >= from && !l->isCompleted())
            removed.push_back(l->getId());
    for (int id : removed) deleteLesson(id);

    int created = 0;
    for (Date d = from; d < s->getTargetDate() && created < 400; d = d.addDays(1)) {
        if (!s->hasLessonOn(d.weekday())) continue;
        bool exists = false;
        for (const auto& l : lessons)
            if (l->getStudentId() == studentId && l->getDate() == d && !l->isCancelled()) exists = true;
        if (exists) continue;
        lessons.push_back(std::make_unique<Lesson>(nextItemId++, studentId, d, s->getStartHour(),
                                                   s->getLessonHours()));
        ++created;
    }
    return created;
}

int TutorMate::autoAssignCurriculum(int studentId) {
    std::vector<Lesson*> list = activeLessonsOf(studentId);
    std::set<int> taught;
    for (Lesson* l : list)
        if (l->isCompleted() && l->getCurriculumId() >= 0) taught.insert(l->getCurriculumId());

    std::vector<int> queue;
    for (const auto& c : curricula)
        if (!taught.count(c.getId())) queue.push_back(c.getId());

    size_t k = 0;
    int assigned = 0;
    for (Lesson* l : list) {
        if (l->isCompleted()) continue;
        if (k < queue.size()) {
            l->setCurriculum(queue[k], curriculumTitle(queue[k]));
            ++k;
            ++assigned;
        } else {
            l->setCurriculum(-1, "");
        }
        refreshTaskTitles(*l);
    }
    return assigned;
}

bool TutorMate::assignCurriculum(int lessonId, int curriculumId) {
    Lesson* l = findLessonMut(lessonId);
    if (!l) return false;
    if (curriculumId >= 0 && !findCurriculum(curriculumId)) return false;
    l->setCurriculum(curriculumId, curriculumTitle(curriculumId));
    refreshTaskTitles(*l);
    return true;
}

std::string TutorMate::conflictWarning(int studentId, const Date& d, int startHour, int hours, int ignoreId) const {
    Lesson probe(0, studentId, d, startHour, hours);
    for (const auto& l : lessons) {
        if (l->getId() == ignoreId || l->isCancelled()) continue;
        if (probe.overlaps(*l)) {
            const Student* s = findStudent(l->getStudentId());
            return "같은 시간에 " + std::string(s ? s->getName() : "?") + " 학생 수업(" + l->timeRange() +
                   ")이 있습니다.";
        }
    }
    return "";
}

int TutorMate::addLesson(int studentId, const Date& d, int startHour, int hours, int curriculumId,
                         LessonStatus status, std::string& warning) {
    if (!findStudent(studentId)) return -1;
    warning = conflictWarning(studentId, d, startHour, hours, -1);
    auto l = std::make_unique<Lesson>(nextItemId++, studentId, d, startHour, hours);
    l->setCurriculum(curriculumId, curriculumTitle(curriculumId));
    l->setStatus(status);
    int id = l->getId();
    lessons.push_back(std::move(l));
    return id;
}

bool TutorMate::cancelLesson(int lessonId, const std::string& reason, bool shiftCurriculum, std::string& msg) {
    Lesson* target = findLessonMut(lessonId);
    if (!target || target->isCancelled()) return false;

    msg.clear();
    if (shiftCurriculum && target->getCurriculumId() >= 0) {
        // 휴강한 수업의 커리큘럼을 다음 수업으로, 나머지는 한 칸씩 뒤로 민다
        int carry = target->getCurriculumId();
        for (Lesson* l : activeLessonsOf(target->getStudentId())) {
            if (l == target || l->isCompleted() || lessonLess(l, target)) continue;
            int tmp = l->getCurriculumId();
            l->setCurriculum(carry, curriculumTitle(carry));
            refreshTaskTitles(*l);
            carry = tmp;
            if (carry < 0) break;
        }
        if (carry >= 0)
            msg = "남은 수업 수가 부족해 '" + curriculumTitle(carry) + "'은(는) 배정에서 빠졌습니다.";
    }
    target->setStatus(LessonStatus::Cancelled);
    target->setCompleted(false);
    target->setNote(clean(reason));
    removeTasksOfLesson(lessonId, false);
    return true;
}

int TutorMate::makeupLesson(int lessonId, const Date& newDate, int newHour, std::string& msg) {
    Lesson* orig = findLessonMut(lessonId);
    if (!orig) return -1;

    msg = conflictWarning(orig->getStudentId(), newDate, newHour, orig->getDuration(), lessonId);
    auto l = std::make_unique<Lesson>(nextItemId++, orig->getStudentId(), newDate, newHour, orig->getDuration());
    l->setCurriculum(orig->getCurriculumId(), curriculumTitle(orig->getCurriculumId()));
    l->setStatus(LessonStatus::Makeup);
    l->setOrigin(orig->getDate());
    int newId = l->getId();

    orig->setStatus(LessonStatus::Cancelled);
    orig->setCompleted(false);
    orig->setNote("보강 → " + newDate.shortStr());

    // 원래 수업의 준비 To-Do를 보강 수업으로 옮긴다
    Date deadline = newDate.addDays(-1);
    for (auto& t : tasks) {
        if (t->getLessonId() != lessonId) continue;
        t->relink(newId, deadline);
        if (!t->isCompleted() && t->getDate() > deadline) {
            Date d = deadline < today() ? today() : deadline;
            t->setDate(d);
        }
    }
    lessons.push_back(std::move(l));
    return newId;
}

bool TutorMate::deleteLesson(int lessonId) {
    auto it = std::find_if(lessons.begin(), lessons.end(),
                           [lessonId](const std::unique_ptr<Lesson>& l) { return l->getId() == lessonId; });
    if (it == lessons.end()) return false;
    lessons.erase(it);
    removeTasksOfLesson(lessonId, false);
    return true;
}

bool TutorMate::setLessonCompleted(int lessonId, bool done) {
    Lesson* l = findLessonMut(lessonId);
    if (!l || l->isCancelled()) return false;
    l->setCompleted(done);
    return true;
}

int TutorMate::resetPlan(int studentId) {
    std::vector<int> ids;
    for (const auto& l : lessons)
        if (l->getStudentId() == studentId && !l->isCompleted()) ids.push_back(l->getId());
    for (int id : ids) deleteLesson(id);
    return static_cast<int>(ids.size());
}

// ======================= 수업 준비 To-Do =======================
std::string TutorMate::makeTaskTitle(const Lesson& l) const {
    if (l.getCurriculumId() < 0) return "수업 자료 준비";
    return curriculumTitle(l.getCurriculumId()) + " 자료 준비";
}

void TutorMate::refreshTaskTitles(const Lesson& l) {
    for (auto& t : tasks)
        if (t->getLessonId() == l.getId() && !t->isCompleted()) t->setTitle(makeTaskTitle(l));
}

void TutorMate::removeTasksOfLesson(int lessonId, bool keepCompleted) {
    tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                               [&](const std::unique_ptr<PrepTask>& t) {
                                   return t->getLessonId() == lessonId && !(keepCompleted && t->isCompleted());
                               }),
                tasks.end());
}

int TutorMate::taskLoadOn(const Date& d) const {
    int n = 0;
    for (const auto& t : tasks)
        if (t->getDate() == d) ++n;
    return n;
}

bool TutorMate::needsPrep(const Lesson& lesson) const {
    if (lesson.isCancelled()) return false;
    if (lesson.getCurriculumId() < 0) return true;
    // 같은 커리큘럼을 이미 끝낸 적이 있으면 준비 불필요 (지난 수업 내용 자동 제외)
    for (const auto& l : lessons) {
        if (l.get() == &lesson || l->getStudentId() != lesson.getStudentId()) continue;
        if (l->isCompleted() && l->getCurriculumId() == lesson.getCurriculumId() &&
            l->getDate() <= lesson.getDate())
            return false;
    }
    return true;
}

TutorMate::PrepPlanResult TutorMate::generatePrepPlan(const Date& weekStart) {
    PrepPlanResult r;
    const Date now = today();
    const Date weekEnd = weekStart.addDays(6);

    std::vector<Lesson*> targets;
    for (auto& l : lessons)
        if (l->getDate() >= weekStart && l->getDate() <= weekEnd && !l->isCancelled() &&
            !l->isCompleted() && l->getDate() >= now)
            targets.push_back(l.get());
    std::sort(targets.begin(), targets.end(), lessonLess);
    r.targetLessons = static_cast<int>(targets.size());

    // 기존 미완료 To-Do는 지우고 새로 배분
    for (Lesson* l : targets) removeTasksOfLesson(l->getId(), true);

    for (Lesson* l : targets) {
        bool hasDone = false;
        for (const auto& t : tasks)
            if (t->getLessonId() == l->getId() && t->isCompleted()) hasDone = true;
        if (hasDone) { ++r.keptDone; continue; }
        if (!needsPrep(*l)) { ++r.skippedTaught; continue; }

        const Date deadline = l->getDate().addDays(-1);
        // 준비 가능 기간: 이번 주 월요일 ~ 수업 전날 (월요일 수업은 지난주 금요일부터)
        Date start = weekStart;
        if (deadline < weekStart) start = weekStart.addDays(-3);
        if (start < now) start = now;

        std::vector<Date> candidates, weekdaysOnly;
        for (Date d = start; d <= deadline; d = d.addDays(1)) {
            candidates.push_back(d);
            if (!d.isWeekend()) weekdaysOnly.push_back(d);
        }
        if (!weekendPrep && !weekdaysOnly.empty()) candidates = weekdaysOnly;

        Date chosen = now;   // 후보가 없으면 (오늘 수업) 오늘 바로 준비
        if (!candidates.empty()) {
            chosen = candidates[0];
            int best = taskLoadOn(chosen);
            for (const Date& d : candidates) {
                int load = taskLoadOn(d);
                if (load < best) { best = load; chosen = d; }   // 가장 한가한 날, 같으면 이른 날
            }
        }
        tasks.push_back(std::make_unique<PrepTask>(nextItemId++, l->getStudentId(), l->getId(), chosen,
                                                   deadline, makeTaskTitle(*l)));
        ++r.created;
    }
    return r;
}

std::vector<const PrepTask*> TutorMate::tasksBetween(const Date& from, const Date& to) const {
    std::vector<const PrepTask*> out;
    for (const auto& t : tasks)
        if (t->getDate() >= from && t->getDate() <= to) out.push_back(t.get());
    std::sort(out.begin(), out.end(), [](const PrepTask* a, const PrepTask* b) {
        if (a->getDate() != b->getDate()) return a->getDate() < b->getDate();
        return a->getId() < b->getId();
    });
    return out;
}

std::vector<const PrepTask*> TutorMate::tasksOfLesson(int lessonId) const {
    std::vector<const PrepTask*> out;
    for (const auto& t : tasks)
        if (t->getLessonId() == lessonId) out.push_back(t.get());
    return out;
}

const PrepTask* TutorMate::findTask(int id) const {
    for (const auto& t : tasks)
        if (t->getId() == id) return t.get();
    return nullptr;
}

PrepTask* TutorMate::findTaskMut(int id) {
    for (auto& t : tasks)
        if (t->getId() == id) return t.get();
    return nullptr;
}

bool TutorMate::toggleTask(int taskId) {
    PrepTask* t = findTaskMut(taskId);
    if (!t) return false;
    t->setCompleted(!t->isCompleted());
    return true;
}

bool TutorMate::moveTask(int taskId, const Date& newDate, std::string& msg) {
    PrepTask* t = findTaskMut(taskId);
    if (!t) { msg = "To-Do를 찾을 수 없습니다."; return false; }
    const Lesson* l = findLesson(t->getLessonId());
    if (l && newDate > l->getDate()) {
        msg = "수업일(" + l->getDate().shortStr() + ") 이후로는 옮길 수 없습니다.";
        return false;
    }
    t->setDate(newDate);
    msg.clear();
    if (l && newDate == l->getDate())
        msg = "주의: 수업 당일로 옮겼습니다. 가능하면 하루 전까지 준비하세요.";
    return true;
}

int TutorMate::rollOverOverdueTasks() {
    const Date now = today();
    int moved = 0;
    for (auto& t : tasks) {
        if (!t->isOverdue(now)) continue;
        const Lesson* l = findLesson(t->getLessonId());
        if (l && l->getDate() < now) continue;   // 이미 지난 수업은 옮기지 않음
        t->setDate(now);
        ++moved;
    }
    return moved;
}

std::vector<const ScheduleItem*> TutorMate::itemsOn(const Date& d) const {
    std::vector<const ScheduleItem*> out;
    for (const Lesson* l : lessonsBetween(d, d)) out.push_back(l);
    for (const PrepTask* t : tasksBetween(d, d)) out.push_back(t);
    return out;
}

// ======================= 저장 / 불러오기 =======================
bool TutorMate::save(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << "TUTORMATE|1\n";
    out << "SET|" << (weekendPrep ? 1 : 0) << "|" << (useFakeToday ? 1 : 0) << "|" << fakeToday.toString() << "\n";
    out << "NEXT|" << nextCurriculumId << "|" << nextStudentId << "|" << nextItemId << "\n";
    for (const auto& c : curricula)
        out << "C|" << c.getId() << "|" << c.getCategory() << "|" << c.getTitle() << "|" << c.getDuration() << "\n";
    for (const auto& s : students)
        out << "S|" << s.getId() << "|" << s.getName() << "|" << s.getInfo() << "|" << s.getTargetDate().toString()
            << "|" << Student::daysToCode(s.getLessonDays()) << "|" << s.getStartHour() << "|" << s.getLessonHours()
            << "\n";
    for (const auto& l : lessons)
        out << "L|" << l->getId() << "|" << l->getStudentId() << "|" << l->getDate().toString() << "|"
            << l->getStartHour() << "|" << l->getDuration() << "|" << l->getCurriculumId() << "|"
            << static_cast<int>(l->getStatus()) << "|" << (l->isCompleted() ? 1 : 0) << "|" << l->getNote() << "|"
            << (l->hasOriginDate() ? 1 : 0) << "|" << l->getOriginDate().toString() << "\n";
    for (const auto& t : tasks)
        out << "T|" << t->getId() << "|" << t->getStudentId() << "|" << t->getLessonId() << "|"
            << t->getDate().toString() << "|" << t->getDeadline().toString() << "|" << t->getTitle() << "|"
            << (t->isCompleted() ? 1 : 0) << "\n";
    return static_cast<bool>(out);
}

void TutorMate::clearAll() {
    curricula.clear();
    students.clear();
    lessons.clear();
    tasks.clear();
    nextCurriculumId = nextStudentId = nextItemId = 1;
    weekendPrep = false;
    useFakeToday = false;
}

int TutorMate::load(const std::string& path, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return 0;
    clearAll();
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF) line = line.substr(3);  // BOM
        if (line.empty()) continue;
        std::vector<std::string> f = split(line, '|');
        const std::string& tag = f[0];
        if (tag == "TUTORMATE") continue;
        if (tag == "SET" && f.size() >= 4) {
            weekendPrep = f[1] == "1";
            useFakeToday = f[2] == "1";
            fakeToday = toDate(f[3]);
        } else if (tag == "NEXT" && f.size() >= 4) {
            nextCurriculumId = toInt(f[1], 1);
            nextStudentId = toInt(f[2], 1);
            nextItemId = toInt(f[3], 1);
        } else if (tag == "C" && f.size() >= 5) {
            curricula.emplace_back(toInt(f[1]), f[2], f[3], toInt(f[4], 2));
        } else if (tag == "S" && f.size() >= 8) {
            students.emplace_back(toInt(f[1]), f[2], f[3], toDate(f[4]), Student::parseDays(f[5]), toInt(f[6], 10),
                                  toInt(f[7], 2));
        } else if (tag == "L" && f.size() >= 12) {
            auto l = std::make_unique<Lesson>(toInt(f[1]), toInt(f[2]), toDate(f[3]), toInt(f[4]), toInt(f[5], 2));
            int cur = toInt(f[6], -1);
            l->setCurriculum(cur, "");
            l->setStatus(static_cast<LessonStatus>(std::clamp(toInt(f[7]), 0, 2)));
            l->setCompleted(f[8] == "1");
            l->setNote(f[9]);
            if (f[10] == "1") l->setOrigin(toDate(f[11]));
            lessons.push_back(std::move(l));
        } else if (tag == "T" && f.size() >= 8) {
            auto t = std::make_unique<PrepTask>(toInt(f[1]), toInt(f[2]), toInt(f[3]), toDate(f[4]), toDate(f[5]), f[6]);
            t->setCompleted(f[7] == "1");
            tasks.push_back(std::move(t));
        } else {
            error = std::to_string(lineNo) + "번째 줄을 읽을 수 없습니다.";
            return -1;
        }
    }
    // 수업에 커리큘럼 제목 연결
    for (auto& l : lessons) l->setCurriculum(l->getCurriculumId(), curriculumTitle(l->getCurriculumId()));
    // ID 카운터가 기존 데이터보다 작지 않도록 보정
    for (const auto& c : curricula) nextCurriculumId = std::max(nextCurriculumId, c.getId() + 1);
    for (const auto& s : students) nextStudentId = std::max(nextStudentId, s.getId() + 1);
    for (const auto& l : lessons) nextItemId = std::max(nextItemId, l->getId() + 1);
    for (const auto& t : tasks) nextItemId = std::max(nextItemId, t->getId() + 1);
    return 1;
}

void TutorMate::loadSampleData() {
    clearAll();
    const char* sample[][2] = {
        {"독서", "독서 기본 원리"},     {"독서", "인문 지문 분석"},     {"독서", "사회 지문 분석"},
        {"독서", "과학·기술 지문 분석"}, {"문학", "현대시 개념"},        {"문학", "현대시 기출 분석"},
        {"문학", "고전시가 개념"},       {"문학", "고전시가 기출 분석"}, {"문학", "현대소설"},
        {"문학", "고전소설"},           {"언매", "음운과 단어"},        {"언매", "문장과 매체"},
        {"실전", "실전 모의고사 1회"},   {"실전", "실전 모의고사 2회"},
    };
    for (auto& c : sample) addCurriculum(c[0], c[1], 2);

    const Date t = today();
    const Date ws = t.weekStart();
    int a = addStudent("김민지", "고3 국어", t.addDays(70), {1, 3}, 10, 2);  // 화, 목 10시
    int b = addStudent("박서연", "고3 국어", t.addDays(56), {0, 4}, 14, 2);  // 월, 금 14시
    int c = addStudent("이준호", "고2 국어", t.addDays(84), {2}, 19, 2);     // 수 19시
    int d = addStudent("최지훈", "고1 국어", t.addDays(63), {0, 1}, 19, 2);  // 월, 화 19시

    // 학생마다 수업을 시작한 시점을 다르게 해서 진도가 다르게 보이도록 함
    const int startedWeeksAgo[] = {4, 2, 3, 1};
    int k = 0;
    for (int id : {a, b, c, d}) {
        generateLessons(id, ws.addDays(-7 * startedWeeksAgo[k++]));
        autoAssignCurriculum(id);
        for (auto& l : lessons)
            if (l->getStudentId() == id && l->getDate() < t) l->setCompleted(true);
    }

    // 예시: 다음 주 김민지 목요일 수업을 토요일로 보강 이동
    for (const Lesson* l : lessonsOf(a)) {
        if (l->getDate() >= ws.addDays(7) && l->getDate().weekday() == 3) {
            std::string msg;
            makeupLesson(l->getId(), l->getDate().addDays(2), 14, msg);
            break;
        }
    }
    generatePrepPlan(ws);
    // 오늘 이전이 기한인 준비는 이미 지난주에 끝낸 것으로 처리
    for (auto& task : tasks)
        if (task->getDeadline() < t) task->setCompleted(true);
}
