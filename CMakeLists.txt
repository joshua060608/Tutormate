cmake_minimum_required(VERSION 3.10)
project(TutorMateGUI CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(TutorMate WIN32
    GuiMain.cpp MainWindow.cpp Canvas.cpp FormDialog.cpp
    Date.cpp Student.cpp Lesson.cpp TutorMate.cpp)
if(MSVC)
    target_compile_options(TutorMate PRIVATE /utf-8 /W3)
endif()
target_link_libraries(TutorMate user32 gdi32)
