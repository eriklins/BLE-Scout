cmake_minimum_required(VERSION 3.5)
project(BLE-Scout)

find_package(nappgui REQUIRED)
include("${NAPPGUI_ROOT_PATH}/prj/NAppCompilers.cmake")
nap_config_compiler()

# Remove WIN32 in Linux and macOS
add_executable(BLEScout WIN32 blescout.c util.c logwin.c ble.c connect.c help.c)
target_link_libraries(BLEScout ${NAPPGUI_LIBRARIES})

# simpleble
add_subdirectory(simpleble/simpleble ${CMAKE_BINARY_DIR}/simpleble)
target_link_libraries(BLEScout simpleble::simpleble-c)

