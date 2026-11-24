// TutorMate (윈도우 앱 버전) - 초보 과외 강사를 위한 수업 계획 도우미
// 객체지향프로젝트 (2026101925 정은수)
#include "MainWindow.h"

// 한글 문자열이 UTF-8로 컴파일되었는지 확인 (Visual Studio는 /utf-8 옵션 필요)
static_assert(sizeof("가") == 4,
              "Visual Studio: 프로젝트 속성 > C/C++ > 명령줄 > 추가 옵션에 /utf-8 을 넣어 주세요.");

#ifdef _MSC_VER
// 콘솔 창 없이 윈도우 앱으로 실행 (프로젝트 설정을 따로 바꾸지 않아도 됨)
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#endif

int main() {
    return MainWindow::run(GetModuleHandleW(nullptr));
}
