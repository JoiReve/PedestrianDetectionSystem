cmake_minimum_required(VERSION 3.10)
project(PedestrianDetectionSystem VERSION 0.2.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# 只支持 OpenCV 5.x
find_package(OpenCV 5 REQUIRED)

message(STATUS "OpenCV_VERSION: ${OpenCV_VERSION}")
message(STATUS "OpenCV_DIR: ${OpenCV_DIR}")
message(STATUS "OpenCV_INCLUDE_DIRS: ${OpenCV_INCLUDE_DIRS}")

# 定义一个可执行目标，名字是 pedestrian_detection_system，由下列文件编译而成
add_executable(pedestrian_detection_system
        src/main.cpp
        src/PedestrianDetector.cpp
        src/ImageFrameReader.cpp
        src/VideoFrameReader.cpp
        src/FrameReaderFactory.cpp
        src/StageTimeTracker.cpp
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

# 冒烟测试
# 用于验证：验证 OpenCV DNN 能加载 ONNX 并前向
add_executable(smoke_dnn scripts/smoke_dnn.cpp)

target_include_directories(smoke_dnn PRIVATE
        ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(smoke_dnn PRIVATE
        ${OpenCV_LIBS}
)