// DnnEngine.hpp
#ifndef DNN_ENGINE_HPP
#define DNN_ENGINE_HPP

#include <string>

enum class DnnEngine {
    Auto,       // 自动选择，优先选择 New 引擎
    New,        // OpenCV 5 全新重写的推理引擎
    Classic,    // OpenCV 4 时代的经典老引擎
    Ort         // ONNX Runtime 引擎
};

// OpenCV 的 ENGINE_* 映射放在 PedestrianDetector.cpp，
// 避免 ProjConfig 间接依赖 OpenCV DNN。

inline const char* toStr(DnnEngine e) {
    switch(e) {
        case DnnEngine::Auto:
            return "auto";
        case DnnEngine::New:
            return "new";
        case DnnEngine::Classic:
            return "classic";
        case DnnEngine::Ort:
            return "ort";
    }
    return "auto";
}

inline bool fromStr(const std::string& s, DnnEngine& e) {
    if(s == "auto") {
        e = DnnEngine::Auto;
        return true;
    }
    if(s == "new") {
        e = DnnEngine::New;
        return true;
    }
    if(s == "classic") {
        e = DnnEngine::Classic;
        return true;
    }
    if(s == "ort") {
        e = DnnEngine::Ort;
        return true;
    }
    return false;
}

#endif // DNN_ENGINE_HPP