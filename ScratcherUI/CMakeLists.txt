cmake_minimum_required (VERSION 3.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

SET(QT_ROOT ${QT_ROOT} CACHE PATH "Qt path" FORCE)
SET(Qt5_DIR ${QT_ROOT}/lib/cmake/Qt5)

find_package(Qt5 COMPONENTS Gui REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Multimedia REQUIRED)
find_package(Qt5 COMPONENTS Network REQUIRED)
find_package(Qt5 COMPONENTS OpenGL REQUIRED)

set(SOURCES
GLUtils.cpp
main.cpp
resources/resources.qrc
Scratcher.ui
Scratcher.cpp
Player.cpp
AudioPlayback.cpp
TimeMap.cpp
SourceView.cpp
VolumeView.cpp
BgmView.cpp
)

set(HEADERS
GLUtils.h
Scratcher.h
Player.h
AudioPlayback.h
TimeMap.h
SourceView.h
VolumeView.h
BgmView.h
)


set (INCLUDE_DIR
../ScratcherLib
.
)

set (LIB_DIR
${FFMPEG_ROOT}/lib
)

set (LINK_LIBS 
ScratcherLib 
Qt5::Gui
Qt5::Widgets
Qt5::Multimedia
Qt5::Network
Qt5::OpenGL
)

if (WIN32) 
set (DEFINES  ${DEFINES}
-D"_CRT_SECURE_NO_DEPRECATE"  
-D"_SCL_SECURE_NO_DEPRECATE" 
)
else()
add_definitions(-std=c++0x)
add_compile_options(-fPIC)
endif()


include_directories(${INCLUDE_DIR})
link_directories(${LIB_DIR})
add_definitions(${DEFINES})
add_executable (ScratcherUI ${SOURCES} ${HEADERS})
if (WIN32)
set_property(TARGET ScratcherUI PROPERTY WIN32_EXECUTABLE true)
endif()
target_link_libraries(ScratcherUI ${LINK_LIBS})

install(TARGETS ScratcherUI RUNTIME DESTINATION .)

if (WIN32)
install(FILES ${QT_ROOT}/plugins/platforms/qwindows.dll DESTINATION ./platforms)
install(FILES ${QT_ROOT}/plugins/audio/qtaudio_wasapi.dll DESTINATION ./audio)
set (QT_DLLS
${QT_ROOT}/bin/Qt5Core.dll
${QT_ROOT}/bin/Qt5Gui.dll
${QT_ROOT}/bin/Qt5Widgets.dll
${QT_ROOT}/bin/Qt5Multimedia.dll
${QT_ROOT}/bin/Qt5Network.dll
${QT_ROOT}/bin/Qt5OpenGL.dll
${QT_ROOT}/bin/libEGL.dll
${QT_ROOT}/bin/libGLESv2.dll
)
install(FILES ${QT_DLLS} DESTINATION .)
endif()



