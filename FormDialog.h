#pragma once
#include <string>
#include <vector>
#include "Win32Util.h"

// 입력 칸을 원하는 만큼 추가해서 띄우는 범용 입력 창 (학생 추가, 휴강 사유, 보강 날짜 등)
class FormDialog {
public:
    enum class Kind { Text, Combo, ComboEdit, Check };

private:
    struct Field {
        std::wstring label;
        Kind kind;
        std::wstring text;
        std::vector<std::wstring> options;
        int sel = 0;
        bool checked = false;
        HWND ctrl = nullptr;
    };

    std::wstring title;
    std::wstring note;
    std::vector<Field> fields;
    HWND hwnd = nullptr;
    bool done = false;
    bool accepted = false;

    static LRESULT CALLBACK proc(HWND h, UINT msg, WPARAM wp, LPARAM lp);
    void readBack();

public:
    explicit FormDialog(std::wstring title, std::wstring note = L"");

    int addText(const std::wstring& label, const std::wstring& value = L"");
    int addCombo(const std::wstring& label, const std::vector<std::wstring>& options, int sel = 0);
    int addComboEdit(const std::wstring& label, const std::vector<std::wstring>& options, const std::wstring& value);
    int addCheck(const std::wstring& label, bool checked);

    // 확인을 누르면 true. 같은 객체로 다시 show() 하면 입력했던 값이 남아 있다.
    bool show(HWND owner, HFONT font, double scale);

    std::wstring text(int i) const { return trimW(fields[static_cast<size_t>(i)].text); }
    int selected(int i) const { return fields[static_cast<size_t>(i)].sel; }
    bool checked(int i) const { return fields[static_cast<size_t>(i)].checked; }
};
