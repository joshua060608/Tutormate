#include "FormDialog.h"

namespace {
const wchar_t* FORM_CLASS = L"TutorMateForm";
const int ID_FIELD = 100;
}

FormDialog::FormDialog(std::wstring t, std::wstring n) : title(std::move(t)), note(std::move(n)) {}

int FormDialog::addText(const std::wstring& label, const std::wstring& value) {
    Field f;
    f.label = label;
    f.kind = Kind::Text;
    f.text = value;
    fields.push_back(f);
    return static_cast<int>(fields.size()) - 1;
}

int FormDialog::addCombo(const std::wstring& label, const std::vector<std::wstring>& options, int sel) {
    Field f;
    f.label = label;
    f.kind = Kind::Combo;
    f.options = options;
    f.sel = sel;
    fields.push_back(f);
    return static_cast<int>(fields.size()) - 1;
}

int FormDialog::addComboEdit(const std::wstring& label, const std::vector<std::wstring>& options,
                             const std::wstring& value) {
    Field f;
    f.label = label;
    f.kind = Kind::ComboEdit;
    f.options = options;
    f.text = value;
    fields.push_back(f);
    return static_cast<int>(fields.size()) - 1;
}

int FormDialog::addCheck(const std::wstring& label, bool checked) {
    Field f;
    f.label = label;
    f.kind = Kind::Check;
    f.checked = checked;
    fields.push_back(f);
    return static_cast<int>(fields.size()) - 1;
}

void FormDialog::readBack() {
    for (Field& f : fields) {
        if (!f.ctrl) continue;
        if (f.kind == Kind::Text || f.kind == Kind::ComboEdit) {
            int len = GetWindowTextLengthW(f.ctrl);
            std::wstring s(static_cast<size_t>(len) + 1, L'\0');
            GetWindowTextW(f.ctrl, &s[0], len + 1);
            s.resize(static_cast<size_t>(len));
            f.text = s;
        } else if (f.kind == Kind::Combo) {
            LRESULT sel = SendMessageW(f.ctrl, CB_GETCURSEL, 0, 0);
            f.sel = sel < 0 ? 0 : static_cast<int>(sel);
        } else {
            f.checked = SendMessageW(f.ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED;
        }
    }
}

LRESULT CALLBACK FormDialog::proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }
    FormDialog* self = reinterpret_cast<FormDialog*>(GetWindowLongPtrW(h, GWLP_USERDATA));
    switch (msg) {
        case WM_COMMAND:
            if (self && LOWORD(wp) == IDOK) {
                self->readBack();
                self->accepted = true;
                self->done = true;
                return 0;
            }
            if (self && LOWORD(wp) == IDCANCEL) {
                self->done = true;
                return 0;
            }
            break;
        case WM_CLOSE:
            if (self) self->done = true;
            return 0;
        case WM_CTLCOLORSTATIC: {
            HDC dc = reinterpret_cast<HDC>(wp);
            SetBkColor(dc, RGB(255, 255, 255));
            SetTextColor(dc, RGB(55, 65, 81));
            return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
        }
        default:
            break;
    }
    return DefWindowProcW(h, msg, wp, lp);
}

bool FormDialog::show(HWND owner, HFONT font, double scale) {
    static bool registered = false;
    HINSTANCE inst = GetModuleHandleW(nullptr);
    if (!registered) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = proc;
        wc.hInstance = inst;
        wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
        wc.lpszClassName = FORM_CLASS;
        RegisterClassExW(&wc);
        registered = true;
    }
    auto S = [scale](int v) { return static_cast<int>(v * scale); };

    // 높이 계산
    const int width = 420, margin = 22;
    int h = margin;
    if (!note.empty()) h += 44;
    for (const Field& f : fields) h += (f.kind == Kind::Check) ? 34 : 62;
    h += 12 + 34 + margin;

    const DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    const DWORD exStyle = WS_EX_DLGMODALFRAME;
    RECT wr{0, 0, S(width), S(h)};
    AdjustWindowRectEx(&wr, style, FALSE, exStyle);
    int ww = wr.right - wr.left, wh = wr.bottom - wr.top;

    RECT orc;
    GetWindowRect(owner, &orc);
    int x = orc.left + ((orc.right - orc.left) - ww) / 2;
    int y = orc.top + ((orc.bottom - orc.top) - wh) / 3;

    done = false;
    accepted = false;
    hwnd = CreateWindowExW(exStyle, FORM_CLASS, title.c_str(), style, x, y, ww, wh, owner, nullptr, inst, this);
    if (!hwnd) return false;

    auto setFont = [font](HWND c) { SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE); };
    int cy = margin;
    if (!note.empty()) {
        HWND st = CreateWindowExW(0, L"STATIC", note.c_str(), WS_CHILD | WS_VISIBLE, S(margin), S(cy),
                                  S(width - 2 * margin), S(40), hwnd, nullptr, inst, nullptr);
        setFont(st);
        cy += 44;
    }
    const int cw = width - 2 * margin;
    for (size_t i = 0; i < fields.size(); ++i) {
        Field& f = fields[i];
        HMENU id = reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_FIELD + static_cast<int>(i)));
        if (f.kind == Kind::Check) {
            f.ctrl = CreateWindowExW(0, L"BUTTON", f.label.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                     S(margin), S(cy + 4), S(cw), S(24), hwnd, id, inst, nullptr);
            SendMessageW(f.ctrl, BM_SETCHECK, f.checked ? BST_CHECKED : BST_UNCHECKED, 0);
            setFont(f.ctrl);
            cy += 34;
            continue;
        }
        HWND lab = CreateWindowExW(0, L"STATIC", f.label.c_str(), WS_CHILD | WS_VISIBLE, S(margin), S(cy), S(cw),
                                   S(20), hwnd, nullptr, inst, nullptr);
        setFont(lab);
        if (f.kind == Kind::Text) {
            f.ctrl = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", f.text.c_str(),
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, S(margin), S(cy + 22),
                                     S(cw), S(30), hwnd, id, inst, nullptr);
        } else {
            DWORD cs = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
                       (f.kind == Kind::Combo ? CBS_DROPDOWNLIST : CBS_DROPDOWN);
            f.ctrl = CreateWindowExW(0, L"COMBOBOX", L"", cs, S(margin), S(cy + 22), S(cw), S(260), hwnd, id, inst,
                                     nullptr);
            for (const std::wstring& o : f.options)
                SendMessageW(f.ctrl, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(o.c_str()));
            if (f.kind == Kind::Combo) SendMessageW(f.ctrl, CB_SETCURSEL, static_cast<WPARAM>(f.sel), 0);
            else SetWindowTextW(f.ctrl, f.text.c_str());
        }
        setFont(f.ctrl);
        cy += 62;
    }
    cy += 12;
    HWND okBtn = CreateWindowExW(0, L"BUTTON", L"확인", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                 S(width - margin - 200), S(cy), S(96), S(34), hwnd,
                                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDOK)), inst, nullptr);
    HWND cancelBtn = CreateWindowExW(0, L"BUTTON", L"취소", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                     S(width - margin - 96), S(cy), S(96), S(34), hwnd,
                                     reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDCANCEL)), inst, nullptr);
    setFont(okBtn);
    setFont(cancelBtn);

    ShowWindow(hwnd, SW_SHOW);
    if (!fields.empty() && fields[0].ctrl) SetFocus(fields[0].ctrl);

    // 모달 루프: 이 창이 닫힐 때까지 메인 창 입력을 막는다
    EnableWindow(owner, FALSE);
    MSG msg;
    while (!done) {
        BOOL r = GetMessageW(&msg, nullptr, 0, 0);
        if (r == 0) {   // WM_QUIT
            PostQuitMessage(static_cast<int>(msg.wParam));
            break;
        }
        if (r < 0) break;
        if (!IsDialogMessageW(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    EnableWindow(owner, TRUE);
    DestroyWindow(hwnd);
    hwnd = nullptr;
    for (Field& f : fields) f.ctrl = nullptr;
    SetForegroundWindow(owner);
    return accepted;
}
