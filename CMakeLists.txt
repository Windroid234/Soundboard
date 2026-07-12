cmake_minimum_required(VERSION 3.10)
project(Soundboard)

set(CMAKE_CXX_STANDARD 17)

add_executable(Soundboard WIN32 main.cpp SoundManager.cpp miniaudio_impl.cpp)
target_link_libraries(Soundboard winmm comdlg32)
