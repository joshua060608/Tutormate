#pragma once
#include <string>
#include <utility>

// 커리큘럼 한 항목 (예: [독서] 인문 지문 분석, 2시간)
class Curriculum {
private:
    int id;
    std::string category;
    std::string title;
    int duration;   // 소요 시간 (시간)

public:
    Curriculum(int id, std::string c, std::string t, int d)
        : id(id), category(std::move(c)), title(std::move(t)), duration(d) {}

    int getId() const { return id; }
    const std::string& getCategory() const { return category; }
    const std::string& getTitle() const { return title; }
    int getDuration() const { return duration; }

    void setCategory(const std::string& c) { category = c; }
    void setTitle(const std::string& t) { title = t; }
    void setDuration(int d) { if (d > 0) duration = d; }
};
