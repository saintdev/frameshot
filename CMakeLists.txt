# ########## Project setup ##########
PROJECT(frameshot)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ######### General setup ##########
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(PNG REQUIRED)
FIND_PACKAGE(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libswscale libavutil)
pkg_check_modules(SCHRO schroedinger-1.0)

# ########## frameshot executable ##########
# Sources:
IF(SCHRO_FOUND)
SET(dirac_SRCS
    input/dirac.c
)
INCLUDE_DIRECTORIES(${SCHRO_INCLUDE_DIRS})
ENDIF(SCHRO_FOUND)

SET(frameshot_executable_SRCS
    output.c
    frameshot.c
    utils.c
    input/y4m.c
    ${dirac_SRCS}
)

# Headers:
SET(frameshot_executable_HDRS
    utils.h
    input.h
    common.h
    output.h
)



# actual target:
ADD_EXECUTABLE(frameshot ${frameshot_executable_SRCS})
TARGET_LINK_LIBRARIES(frameshot ${PNG_LIBRARIES} ${ZLIB_LIBRARIES} ${FFMPEG_LIBRARIES} ${SCHRO_LIBRARIES})

# add install target:
INSTALL(TARGETS frameshot DESTINATION bin)
