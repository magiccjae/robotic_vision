cmake_minimum_required(VERSION 2.8)
set (CMAKE_CXX_STANDARD 11)
project( test )
find_package( OpenCV REQUIRED )

add_executable(test_algorithm test2.cpp )

target_link_libraries(test_algorithm ${OpenCV_LIBS} )
