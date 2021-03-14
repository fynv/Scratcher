cmake_minimum_required (VERSION 3.0)

SET(FFMPEG_ROOT ${FFMPEG_ROOT} CACHE PATH "FFMpeg path" FORCE)

set (INCLUDE_DIR
${FFMPEG_ROOT}/include
.
)

set (LIB_DIR
${FFMPEG_ROOT}/lib
)

if (WIN32) 
set (DEFINES  ${DEFINES}
-D"_CRT_SECURE_NO_DEPRECATE"  
-D"_SCL_SECURE_NO_DEPRECATE" 
-D"_CRT_SECURE_NO_WARNINGS"
)
else()
add_definitions(-std=c++17)
add_compile_options(-fPIC)
endif()

include_directories(${INCLUDE_DIR})
link_directories(${LIB_DIR})

set (LIB_SOURCES
TrackBuffer.cpp
AudioReadWrite.cpp
LinearInterpolate.cpp
CHSpline.cpp
SamplerDirect.cpp
SamplerScratch.cpp
SampleToTrackBuffer.cpp
)

set (LIB_HEADERS
TrackBuffer.h
AudioReadWrite.h
LinearInterpolate.h
CHSpline.h
Sampler.h
SamplerDirect.h
SamplerScratch.h
SampleToTrackBuffer.h
)

add_definitions(${DEFINES})

add_library(ScratcherLib ${LIB_SOURCES} ${LIB_HEADERS})
target_link_libraries(ScratcherLib avformat avcodec avutil swresample)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX  ../bin CACHE PATH "Install path" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

if (WIN32) 
set (FFMPEG_DLLS
${FFMPEG_ROOT}/bin/avformat-58.dll
${FFMPEG_ROOT}/bin/avcodec-58.dll
${FFMPEG_ROOT}/bin/avutil-56.dll
${FFMPEG_ROOT}/bin/swresample-3.dll
)

install(FILES ${FFMPEG_DLLS} DESTINATION .)
endif()