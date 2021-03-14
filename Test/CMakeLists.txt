cmake_minimum_required (VERSION 3.0)

set (INCLUDE_DIR
../ScratcherLib
)

set (LIB_DIR
${FFMPEG_ROOT}/lib
)

include_directories(${INCLUDE_DIR})
link_directories(${LIB_DIR})

add_executable(Test main.cpp)
target_link_libraries(Test ScratcherLib)

install(TARGETS Test RUNTIME DESTINATION .)

