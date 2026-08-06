cmake_minimum_required(VERSION 3.15)
project(Soundboard)

set(CMAKE_CXX_STANDARD 17)

# Statically link MSVC C/C++ runtime library (/MT for Release, /MTd for Debug)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(Soundboard WIN32 main.cpp SoundManager.cpp miniaudio_impl.cpp)
target_link_libraries(Soundboard winmm comdlg32 comctl32)
target_link_options(Soundboard PRIVATE $<$<CXX_COMPILER_ID:GNU>:-municode>)

