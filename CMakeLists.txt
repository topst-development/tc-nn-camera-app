cmake_minimum_required(VERSION 2.8)
project( tcnncameraapp )

include_directories( ${LINUX_KERNEL_DIR}/include/video/telechips )

include_directories(
    ${CMAKE_SOURCE_DIR}/common
)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -lpthread ")

add_executable(tcnncameraapp 
camera_api.c
display_api.c
scaler_api.c
main.c )
target_link_libraries(tcnncameraapp ${LIBRARIES})
install(TARGETS tcnncameraapp DESTINATION bin)

