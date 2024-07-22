cmake_minimum_required(VERSION 2.8)
project(cv_final)
set(CMAKE_BUILD_TYPE "Release")

set(CMAKE_CXX_STANDARD 14)
find_package(OpenCV REQUIRED)

set(SRCS src/TableDetection.cpp src/BallDetection.cpp src/findCenters.cpp)
add_library(${PROJECT_NAME} ${SRCS})
target_include_directories( ${PROJECT_NAME} PUBLIC
        src
        include
        ${OpenCV_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
        ${OpenCV_LIBS})

add_executable(Starter src/main.cpp)
target_link_libraries(Starter ${PROJECT_NAME})
set_target_properties(Starter PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)


