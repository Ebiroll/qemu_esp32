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

# ---- QEMU options ----
set(QEMU_PATH "/home/olof/play/calib/build" CACHE STRING "Path to qemu build directory")
set(QEMU_MACHINE "esp32" CACHE STRING "QEMU machine type (esp32, esp32s2, esp32s3)")
set(QEMU_FLASH_FILE "esp32flash.bin" CACHE STRING "Default flash file name")

# ---- Your apps ----
add_executable(hdiff
    hdiff.cpp
)

target_compile_definitions(hdiff PRIVATE
    QEMU_PATH="${QEMU_PATH}"
    QEMU_MACHINE="${QEMU_MACHINE}"
    QEMU_FLASH_FILE="${QEMU_FLASH_FILE}"
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
