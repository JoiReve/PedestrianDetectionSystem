// config.hpp
// 配置部分
#ifndef PROJ_CONFIG_HPP
#define PROJ_CONFIG_HPP

#include <string>

struct ProjConfig {
    // 路径
    std::string model_path;
    std::string source_path;

    // 阈值
    float conf_threshold = 0.5f;    // 调高：漏检多、误检少；调低：漏检少、误检多
    float iou_threshold = 0.45f;    // 调高：保留更多重叠框；调低：合并得更激进

    // 模型输入尺寸
    int input_width = 640;
    int input_height = 640;

    // 类别过滤
    int person_class_id = 0;
};

#endif // PROJ_CONFIG_HPP
