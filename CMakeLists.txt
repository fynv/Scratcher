cmake_minimum_required (VERSION 3.0)

project(Scratcher)

add_subdirectory(ScratcherLib)
add_subdirectory(ScratcherUI)

set(SCRATCHER_BUILD_TESTS false CACHE BOOL "Build tests")

if (SCRATCHER_BUILD_TESTS)
add_subdirectory(Test)
endif()

