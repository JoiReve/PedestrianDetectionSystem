# 更新记录

本文件由 [git-cliff](https://git-cliff.org/) 从提交历史自动生成，**请勿手工编辑**。
提交信息遵循 [Conventional Commits](https://www.conventionalcommits.org/) 约定。

## [0.2.2] - 2026-10-20

### ✨ 新功能

- 小幅度修改了冒烟测试的部分内容
- 新增性能观测

### 🐛 修复

- 优化文件结构
- 优化文件结构
- 修复了消除 Node20 警告并增加 OpenCV CMake 路径调试
- 修复 C++ 编译问题及 CI 警告，并同步更新 README 文档
- 修复了缺失头文件的 bug

### ⚡ 性能

- 更新行人检测算法实现及主函数调用流程

### 📝 文档

- 修复了本地编译环境（OpenCV 5.1）与 CI 环境的差异问题
- 更新 README 文档
- 更新项目说明文档、路线图及示例图片

### ✅ 测试

- 新增 DNN 冒烟测试，验证 ONNX 加载与前向推理链路

### ⚙️ 杂项

- 更新 CI 配置，替换假的冒烟测试，明确 job 定位
- 更新 CMakeLists.txt
- 更新工作流配置 ci.yml
- 更新CMake构建配置

## [0.2.1] - 2026-10-14

### ✨ 新功能

- 支持 Q/q 键退出

### 📝 文档

- 更新 README 与 roadmap
- 移除个人的开发笔记

### ⚙️ 杂项

- 新增 git-cliff 配置
- 新增 CI 工作流与文档校验脚本
- 补充 yolov8n PyTorch 权重（导出 ONNX 的源文件）回仓库
- 更新 .gitignore
- 版本号升至 0.2.1

## [0.2.0] - 2026-10-13

### ✨ 新功能

- 新增 FrameReader 抽象接口与图片/视频实现
- 新增 ProjConfig 集中配置
- main 改为统一帧循环，支持 --version

### ♻️ 重构

- PedestrianDetector 适配统一帧循环

### 📝 文档

- 更新 OpenCV 笔记，新增工厂模式、C++ OOP、roadmap 及演示截图
- 更新 README（技术栈、原理、能力边界）

### ⚙️ 杂项

- CMake 加入版本号 0.2.0 与 PROJECT_VERSION 宏
- 完善 .gitignore（C++ 产物、venv、IDE）
- 添加 AGPL-3.0 LICENSE

## [0.1.0] - 2026-10-09

### 📦 其他

- PedestrianDetectionSystem v0.1


