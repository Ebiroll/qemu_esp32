cmake_minimum_required(VERSION 3.16)

project(nana_test LANGUAGES CXX)

# Required by Nana
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ---- System deps ----
find_package(Freetype REQUIRED)
find_package(X11 REQUIRED)

# IMPORTANT:
# Add freetype include globally so Nana itself sees ft2build.h
include_directories(${FREETYPE_INCLUDE_DIRS})

# ---- Fetch Nana ----
FetchContent_Declare(
    nana
    GIT_REPOSITORY https://github.com/cnjinhao/nana.git
    GIT_TAG develop-1.8
)

FetchContent_MakeAvailable(nana)

# ---- Your apps ----
add_executable(hdiff
    hdiff.cpp
)

target_link_libraries(hdiff PRIVATE
    nana
    ${FREETYPE_LIBRARIES}
    ${X11_LIBRARIES}
)

add_executable(plot
    plot.cpp
    realtime_main.cpp
)

target_link_libraries(plot PRIVATE
    nana
    ${FREETYPE_LIBRARIES}
    ${X11_LIBRARIES}
)
