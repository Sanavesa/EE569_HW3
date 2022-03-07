cmake_minimum_required(VERSION 3.0.0)
project(EE569_HW2 VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
enable_testing()

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable(EE569_HW3_Q1 src/main_1.cpp src/Image.h src/Image.cpp src/Utility.h src/Utility.cpp src/Implementations.h)

include_directories(SYSTEM ./src)

target_link_libraries( EE569_HW3_Q1 ${OpenCV_LIBS} )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
