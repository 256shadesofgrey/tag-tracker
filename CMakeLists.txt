cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)
#set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}-g -pg")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -pg")

project(tag-tracker)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

find_package(Boost CONFIG REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCE_FILES main.cpp)
set(GENERATE_TAGS_SOURCE_FILES generate_tags.cpp)
set(GENERATE_CHECKERBOARD_SOURCE_FILES generate_checkerboard.cpp)
set(CAMERA_CALIBRATION_SOURCE_FILES camera_calibration.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
add_executable("${PROJECT_NAME}-generate-tags" ${GENERATE_TAGS_SOURCE_FILES})
add_executable("${PROJECT_NAME}-generate-checkerboard" ${GENERATE_CHECKERBOARD_SOURCE_FILES})
add_executable("${PROJECT_NAME}-camera-calibration" ${CAMERA_CALIBRATION_SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories("${PROJECT_NAME}-generate-tags" PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories("${PROJECT_NAME}-generate-checkerboard" PRIVATE ${Boost_INCLUDE_DIRS})
target_include_directories("${PROJECT_NAME}-camera-calibration" PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} Boost::program_options)
target_link_libraries("${PROJECT_NAME}-generate-tags" ${OpenCV_LIBS} Boost::program_options)
target_link_libraries("${PROJECT_NAME}-generate-checkerboard" ${OpenCV_LIBS} Boost::program_options)
target_link_libraries("${PROJECT_NAME}-camera-calibration" ${OpenCV_LIBS} Boost::program_options)
