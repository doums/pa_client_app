cmake_minimum_required(VERSION 3.19)
project(audio C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-W -Wall -Wextra") #-Werror

include(FindPkgConfig)

add_library(paudio STATIC lib/src/audio.c)

if (PkgConfig_FOUND)
    pkg_check_modules(LIBPULSE libpulse>=13)
endif ()

if (NOT LIBPULSE_LINK_LIBRARIES)
    message(FATAL_ERROR "libpulse not found")
endif ()

target_include_directories(paudio INTERFACE ${PROJECT_BINARY_DIR}/include)
target_link_libraries(paudio PRIVATE ${LIBPULSE_LINK_LIBRARIES})
target_include_directories(paudio PUBLIC ${LIBPULSE_INCLUDE_DIRS})

install(TARGETS paudio DESTINATION lib)
install(FILES lib/include/audio.h DESTINATION include)

add_executable(audio main.c)
target_link_libraries(audio paudio)
