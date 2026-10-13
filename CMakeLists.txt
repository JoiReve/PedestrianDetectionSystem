cmake_minimum_required(VERSION 3.10)
project(PedestrianDetectionSystem VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV REQUIRED)


# 定义一个可执行目标，名字是 pedestrian_detection_system，由下列文件编译而成
add_executable(pedestrian_detection_system
        src/main.cpp
        src/PedestrianDetector.cpp
        src/ImageFrameReader.cpp
        src/VideoFrameReader.cpp
        src/FrameReaderFactory.cpp
)

# 给指定 target 添加头文件搜索路径
target_include_directories(pedestrian_detection_system PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
)

# 把库链接进可执行文件
target_link_libraries(pedestrian_detection_system PRIVATE
        ${OpenCV_LIBS}
)

target_compile_definitions(pedestrian_detection_system PRIVATE
        PROJECT_VERSION="${PROJECT_VERSION}"
)