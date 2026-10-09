# PedestrianDetectionSystem · OpenCV 学习笔记

> 配套项目：PedestrianDetectionSystem v0.1（单张图片 · 单类别 person · YOLOv8n ONNX + OpenCV DNN）
> 相关文档：[README.md](../README.md)（项目说明与工作原理）
> 适用：OpenCV 4.x / 5.x —— 本项目实际编译链接的是自编译的 **OpenCV 5.1.0**（`/usr/local`），用到的 API 两个大版本一致。
> 文中行号对应本仓库当前源码（`src/`、`include/`、`scripts/`）。
>
> 阅读建议：先看 §0 建立地图 → 按 §1~§10 顺序学习 → 用 §11 的练习和自测题巩固。

## 目录

- [§0 总览：项目代码 ↔ OpenCV 知识点](#0-总览项目代码--opencv-知识点)
- [§1 cv::Mat：整个 OpenCV 的地基](#1-cvmat整个-opencv-的地基)
- [§2 图像 I/O 与 HighGUI](#2-图像-io-与-highgui)
- [§3 几何变换：resize 与 LetterBox](#3-几何变换resize-与-letterbox)
- [§4 blobFromImage：预处理全家桶](#4-cv_dnnblobfromimage预处理全家桶)
- [§5 DNN 模块](#5-dnn-模块)
- [§6 后处理里的 core 操作](#6-后处理里的-core-操作)
- [§7 绘图 API](#7-绘图-api)
- [§8 基础类型速查](#8-基础类型速查)
- [§9 本项目中的易错点（自查清单）](#9-本项目中的易错点自查清单)
- [§10 学习路线](#10-学习路线)
- [§11 练习与自测题](#11-练习与自测题)

---

## §0 总览：项目代码 ↔ OpenCV 知识点

这个项目虽然只有 200 行，但恰好覆盖了 OpenCV 的四条主干：`core`（Mat 内存模型）、`imgproc`（几何变换）、`highgui`（显示）、`dnn`（推理）。

| 项目位置 | OpenCV API | 所属模块 / 知识主题 |
|---|---|---|
| `src/main.cpp:13-17` | `cv::imread` + `cv::Mat::empty()` | imgcodecs：读取、通道序、失败判空 |
| `src/main.cpp:24-30` | `cv::rectangle` `cv::putText` `cv::Scalar` | imgproc 绘图：颜色序、坐标语义 |
| `src/main.cpp:32-34` | `cv::imshow` `cv::waitKey` `cv::destroyAllWindows` | highgui：显示与事件循环 |
| `include/PedestrianDetector.hpp:5-6` | `opencv2/opencv.hpp` `opencv2/dnn.hpp` | 头文件组织 |
| `include/PedestrianDetector.hpp:11-22` | `cv::Rect` | core 基础类型 |
| `src/PedestrianDetector.cpp:9` | `cv::dnn::readNetFromONNX` | dnn：模型加载 |
| `src/PedestrianDetector.cpp:11-13` | `setPreferableBackend/Target`（已注释，默认值生效） | dnn：后端与设备 |
| `src/PedestrianDetector.cpp:19-21` | `cv::Mat::empty()` + 抛异常 | core：空矩阵判断与错误处理 |
| `src/PedestrianDetector.cpp:24-32` | 手工计算 scale/pad | 几何变换的数学 |
| `src/PedestrianDetector.cpp:36` | `cv::resize` | imgproc：重采样与插值 |
| `src/PedestrianDetector.cpp:39` | `cv::Mat(rows, cols, type, Scalar)` | core：Mat 构造与填充 |
| `src/PedestrianDetector.cpp:40-41` | `mat(Rect)` + `copyTo` | core：ROI 视图与深拷贝 |
| `src/PedestrianDetector.cpp:50` | `reshape` + `transpose` | core：形状操作（视图 vs 新分配） |
| `src/PedestrianDetector.cpp:52,60` | `data` 裸指针 + 包裹外部内存的 Mat 头 | core：内存模型 |
| `src/PedestrianDetector.cpp:63` | `cv::minMaxLoc` | core：矩阵统计 |
| `src/PedestrianDetector.cpp:88` | `cv::dnn::NMSBoxes` | dnn：后处理 |
| `src/PedestrianDetector.cpp:107-115` | `cv::dnn::blobFromImage` | dnn：预处理 |
| `src/PedestrianDetector.cpp:118-119` | `setInput` / `forward` | dnn：前向推理 |

---

## §1 cv::Mat：整个 OpenCV 的地基

### 1.1 本质：矩阵头 + 数据块 + 引用计数

`cv::Mat` 变量 = 一个**矩阵头**（`rows, cols, type, step, data 指针, 引用计数`）+ 它指向的**像素数据块**。拷贝 `Mat` 变量只拷贝头、共享数据：

```cpp
cv::Mat b = a;         // 浅拷贝：b 和 a 指向同一块像素，改 b 影响 a！
cv::Mat c = a.clone(); // 深拷贝：一定新分配
a.copyTo(d);           // 深拷贝（d 尺寸类型不符时才重新分配）
```

**本项目正是靠这个机制工作的**（`cpp:40-41`）：

```cpp
cv::Mat roi = letterbox_img(cv::Rect(lb_info.pad_w, lb_info.pad_h, new_w, new_h));
resized.copyTo(roi);
```

第一行取出的 `roi` 是**共享内存的 ROI 视图**（不是拷贝！），第二行的 `copyTo` 才把像素真正写进画布。

如果写成下面这样就完全错了 —— 那只是让 `roi` 这个头改指向 `resized`，画布纹丝不动：

```cpp
cv::Mat roi = letterbox_img(rect);
roi = resized;   // 错误！这只是"改指向"，不是"把 resized 写进 ROI"
```

> **第一课：赋值 = 改指向，copyTo = 写数据。**

另一个推论：**ROI 视图会拖住整块父内存**（引用计数挂在父 buffer 上）。只要子视图还活着，整张原图就不会被释放。

### 1.2 Mat 的构造方式

| 写法 | 是否分配内存 | 项目中 |
|---|---|---|
| `cv::Mat m;` | 否，空矩阵 | `cpp:35 resized`、`cpp:106 blob`、`cpp:119 output`，作为 API 的输出参数，由 API 内部 `create()` |
| `Mat(rows, cols, type, Scalar)` | 是，且填充 | `cpp:39` 灰底画布 `Mat(640, 640, CV_8UC3, Scalar(114,114,114))` |
| `Mat(rows, cols, type, void* data, step=0)` | **否，只包一个头** | `cpp:60` 的 `scores` —— 把转置矩阵里一段内存"当成" 1×80 的 Mat 用，零拷贝 |

延伸要会：`Mat::zeros/ones`、`Mat(Size(w,h), type)`（注意 Size 是**先宽后高**，与 `Mat(rows, cols)` 正好相反，经典坑）。

**空矩阵判断**：`m.empty()` 等价于 `m.data == nullptr || m.total() == 0`。**`imread` 失败时返回空 Mat 而不是抛异常**，所以本项目两处检查它：`main.cpp:14-17`（命令行入口）和 `cpp:19-21`（`preprocess` 开头，抛 `std::invalid_argument`，由 `main` 的 `try/catch` 兜底）。

### 1.3 类型系统：`CV_<depth>C<channels>`

| 宏 | 含义 | 项目中出现处 |
|---|---|---|
| `CV_8UC3` | 8 位无符号 × 3 通道（彩色图） | `cpp:39` 画布；`imread` 的默认产物 |
| `CV_32FC1` | 32 位浮点 × 1 通道 | `cpp:60` scores |
| `CV_8U` / `CV_32F` | 仅深度（无通道） | 概念上贯穿全流程 |

- 常用查询：`m.type()`（完整类型码）、`m.depth()`（元素深度）、`m.channels()`、`m.elemSize()`。
- **推理链上的类型流转**：`CV_8UC3`（原图/画布）→ `CV_32F, dims=4`（blob，NCHW，**通道被折进维度里，channels()==1**）→ `CV_32F, dims=3`（output）→ `CV_32FC1` 2D（转置后）。

> ⚠️ blob 不是"3 通道浮点图"，而是 4 维张量、单通道。这点最容易误解。

### 1.4 维度与 shape：`Size(w,h)` vs `Mat(rows,cols)` 的大坑

- 2D Mat：`rows`、`cols`；`size()` 返回 `cv::Size(cols, rows)`，而 `cv::Size` 的字段是 **width, height**。
- **高维 Mat（`dims > 2`）不要依赖 `rows/cols`**（文档语义是 `(-1,-1)`），要用 `dims` 和 `size[i]`。

  本项目 `output` 是 1×84×8400 的三维矩阵，所以 `cpp:50` 先 `reshape(1, 84)` 变成 2D，再去 `transpose` 和访问 `rows/cols` —— 这个顺序不是随意的。
- `cv::Size(input_width_, input_height_)` 传的是 (宽, 高)，`image.cols/rows` 也是 (宽, 高) —— 同一个概念两套参数顺序，写反了不报错、只是结果错，务必形成肌肉记忆。

### 1.5 连续性、step、data 指针

- 内存布局：**行优先 + 通道交错**（HWC：BGRBGRBGR…）。
- `step` = 每行字节数，可能大于 `cols × elemSize`（当 Mat 是父矩阵的 ROI 时）。用 `isContinuous()` 判断连续性。
- 连续时才能线性寻址 `data[i*cols + j]` —— `cpp:52` 的 `reinterpret_cast<float*>(output_transposed.data)` 正是依赖"transpose 的结果必然是新分配的连续矩阵"。

**三种像素访问方式**（性能从高到低、安全性从低到高）：

| 方式 | 写法 | 特点 |
|---|---|---|
| 裸指针 | `float* p = (float*)m.data; p[i*84 + j]` | 最快；需自己保证连续 + 类型正确。本项目 `cpp:52` 之后的 `*(data + i*cols + j)` |
| 行指针 | `float* p = m.ptr<float>(i); p[j]` | **推荐**：自动处理 step、类型检查。本项目可改写成这样 |
| 元素访问 | `m.at<float>(i, j)` | 最慢（Debug 带边界检查），调试期用 |

### 1.6 视图 vs 新分配

| 操作 | 拷贝数据？ | 说明 |
|---|---|---|
| `m(Rect)` | 视图 | ROI，共享内存，通常不连续 |
| `m.row(i)` / `m.colRange(a,b)` | 视图 | 行/列切片 |
| `m.reshape(cn, rows)` | 视图 | O(1)，**要求连续**，否则抛异常 |
| `Mat(..., void* data, ...)` | 视图 | 包裹外部内存 |
| `cv::transpose` | **新分配** | 位置重排，必须新内存 |
| `resize` / `copyTo` / `clone` / `blobFromImage` | **新分配** | |

本项目一次 `detect()` 里真正分配内存的只有 4 次（resize、画布、blob、transpose），其余全是视图 —— 这就是所谓的"零拷贝设计"。

---

## §2 图像 I/O 与 HighGUI

| API | 要点 |
|---|---|
| `cv::imread(path, flags)` | 默认 `IMREAD_COLOR`：**3 通道 BGR、CV_8U、丢弃 alpha**；另有 `IMREAD_GRAYSCALE` / `IMREAD_UNCHANGED`。**失败返回空 Mat 不抛异常** —— 必须 `if (img.empty())` 检查。本项目在 `main.cpp:14-17` 做了空图检查 |
| BGR 不是 RGB | OpenCV 的历史约定。`cv::Scalar(0,0,255)` 是**红**（`main.cpp:24`）；转 RGB 用 `cvtColor(BGR2RGB)` 或交给 `blobFromImage(swapRB=true)` |
| `cv::imshow(win, mat)` | 只接受 8U；**浮点 Mat 会按 [0,1]→[0,255] 映射**（调试 blob 时会看到全黑/全白）；无头环境不可用 |
| `cv::waitKey(0)` | 双重身份：**等按键 + GUI 事件泵**（不调用窗口就不刷新）；参数为毫秒，0 = 无限等；返回按键码 |
| `cv::destroyAllWindows()` | 关闭全部窗口 |
| （延伸）`cv::imwrite` | 调试神器：把中间 Mat 存盘看效果，比 imshow 更适合无头/远程环境 |
| （延伸）`cv::namedWindow` `cv::VideoCapture` | v0.1 没用，但 v0.2 做视频必用 |

---

## §3 几何变换：resize 与 LetterBox

### 3.1 `cv::resize` 签名与语义（`cpp:36`）

```cpp
cv::resize(src, dst, dsize, fx, fy, interpolation);
// dsize 非空时以 dsize 为准；dsize=Size() 时才用 fx/fy
```

插值方式选择：

| 插值 | 何时用 |
|---|---|
| `INTER_LINEAR` | 默认（本项目省略了该参数，走的就是它），放大/通用 |
| `INTER_AREA` | **缩小首选**（避免混叠/摩尔纹）—— 本项目是缩小却用了 LINEAR，可作为实验点 |
| `INTER_NEAREST` | 最快最糙（标签图） |
| `INTER_CUBIC` / `INTER_LANCZOS4` | 更平滑但更慢 |

**目标尺寸的防御**：`new_w/new_h` 被钳到至少 1（`cpp:29-30`）。极端长宽比的图（如 2000×10）等比缩到 640 宽后，高度 `round` 可能算成 0，而 `resize` 的目标尺寸出现 0 会直接抛异常 —— 钳位是最低成本的保护（见 §11 练习 3）。

### 3.2 LetterBox 的数学（本项目核心算法，值得吃透）

- 三个量：`scale = min(640/W, 640/H)`、`new_w/new_h`、`pad_w/pad_h`（`cpp:24-32`）。
- **正变换**（原图 → 网络输入）：`x' = x·scale + pad_w`，`y' = y·scale + pad_h`
- **逆变换**（网络输出 → 原图，`cpp:73-76`）：`x = (x' - pad_w)/scale`
- 灰边 114 是 YOLO 系列的训练惯例（`cpp:39`），推理必须与训练一致。
- 误差来源意识：整数除法导致左右 pad 差 1px；`round` 引入 ±0.5px —— **像素域的几何变换永远有舍入误差**。

延伸实现方式（进阶对比学习）：

- `cv::copyMakeBorder` 替代手工建画布；
- `getAffineTransform` + `warpAffine` 把"缩放 + 平移"打包成 2×3 仿射矩阵一次完成，逆变换还有现成的 `cv::invertAffineTransform`。

---

## §4 `cv::dnn::blobFromImage`：预处理全家桶

```cpp
cv::dnn::blobFromImage(image, blob, scalefactor, size, mean, swapRB, crop, ddepth);
```

一次完成 5 件事，执行顺序：

```
resize（crop=false 时是普通拉伸，不保持长宽比！）
  → 减 mean（单位是原图 0-255 尺度）
  → 乘 scalefactor
  → swapRB（BGR→RGB）
  → 组装成 NCHW 的 4 维 CV_32F blob
```

必须理解的三个点：

1. **它不保持长宽比**。所以本项目要先手工 letterbox，再让 `blobFromImage` 做一次"尺寸恰好也是 640×640"的空 resize。这也解释了为什么 640 在 `include/PedestrianDetector.hpp:34-35` 和 `src/PedestrianDetector.cpp:111` 出现两次 —— 两处必须一致，否则 `LetterBoxInfo` 描述的坐标系与实际输入错配，**且不会报错**，只是框全歪。
2. **归一化公式 `(px - mean) × scalefactor`**：本项目 `mean=0, scale=1/255` → [0,1]。若模型训练时用了 ImageNet 均值，这里要写 `mean=Scalar(0.485*255, ...)`（**0-255 尺度**！高频坑）；若还有 per-channel std，单个 `scalefactor` 表达不了，需手工分通道 `convertTo`。
3. **手工等价实现**（帮助理解）：`resize` → `convertTo(CV_32F, 1/255)` → swapRB → `reshape` 成 NCHW。`blobFromImage` 只是这些操作的封装。

---

## §5 DNN 模块

| API | 要点 |
|---|---|
| `cv::dnn::readNetFromONNX(path)`（`cpp:9`） | 也有一族 `readNetFromCaffe/TF/Torch/Darknet` 和通用 `readNet`；读取 = 解析计算图 + 载入权重，**很贵**，所以放在构造函数里只做一次 |
| `setPreferableBackend` / `setPreferableTarget`（`cpp:11-13`，**已注释**） | 后端：`DNN_BACKEND_OPENCV`（默认/最可移植）、`OPENVINO`、`CUDA` …；目标：`DNN_TARGET_CPU`（默认）、`OPENCL`、`CUDA`、`MYRIAD` …。**注意**：本项目这两行被注释掉，实际生效的是默认值 —— 默认后端即 OpenCV 实现、默认目标即 CPU（`cpp:14` 的注释也写明了这一点）；需要显式指定时取消注释即可，显式写出的好处是"换后端时一眼知道改哪里" + 自文档化。后端 × 目标是一个矩阵，值得系统了解 |
| `net_.setInput(blob)`（`cpp:118`） | 完整签名 `setInput(blob, name="", scalefactor=1, mean=0)`；因为 blob 已归一化，这里用默认值 |
| `net_.forward()`（`cpp:119`） | 默认返回最后一层输出。**本模型（ultralytics 导出）输出是原始头 `[1,84,8400]`，需要手工解码**；若换成 YOLOv5 等多输出模型，要用 `getUnconnectedOutLayersNames()` 拿输出名逐个 forward |
| （延伸）`getPerfProfile()` | 拿各层耗时，学性能分析的好入口 |
| （延伸）`cv::dnn::DetectionModel` | 高层封装，input 尺寸 + 阈值 + NMS 一步到位，可用来对拍本项目的手写流程 |

### `cv::dnn::NMSBoxes`（`cpp:88`）

注意它只是 dnn 模块里的工具函数，与 Net 无关：

```cpp
NMSBoxes(bboxes, scores, score_threshold, nms_threshold, indices, eta=1, top_k=0);
```

- 两阶段：先按 `score_threshold` 过滤，再贪心 IoU 抑制；返回**保留框的索引**（所以本项目有 `nms_indices` 这个中间容器）。
- 强烈建议手写一遍 NMS（IoU 计算 + 按分数排序 + 贪心抑制）—— 它是面试常客，理解了它也就理解了为什么需要 `boxes` / `confidences` 两个平行数组。

---

## §6 后处理里的 core 操作

| API | 学到什么 |
|---|---|
| `reshape(cn, rows)`（`cpp:50`） | 改形状不改数据；`reshape(1, 84)` = 1 通道、84 行；**必须连续**否则抛异常 |
| `cv::transpose`（`cpp:50`） | 转置；本项目用它把「逐属性」变「逐候选框」，行优先布局下遍历更缓存友好 |
| `cv::minMaxLoc`（`cpp:63`） | 找最大最小值 + 位置；位置是 `Point(x=列, y=行)`，1 行矩阵上只有 `.x` 有意义 —— 这就是为什么代码里是 `class_id.x`；只支持单通道；`nullptr` 表示不需要该项 |
| `const_cast` / `reinterpret_cast`（`cpp:52,60`） | Mat 的"包裹外部内存"构造要 `void*`（非 const）所以要 const_cast；`reinterpret_cast<float*>` 转到浮点指针依赖 `type()==CV_32F`。这是"用类型安全换零拷贝"的典型交易 |

**更优雅的等价写法（建议作为练习改写）** —— 用视图代替裸指针：

```cpp
float* row = output_transposed.ptr<float>(i);                // 行指针
cv::Mat scores = output_transposed.row(i).colRange(4, 84);   // 行视图，同样零拷贝
```

---

## §7 绘图 API

| API | 要点 |
|---|---|
| `cv::rectangle(img, rect, color, thickness)`（`main.cpp:24`） | **原地修改**传入的 Mat；颜色用 BGR 序的 `Scalar`；`thickness=-1` 填充 |
| `cv::putText(img, text, org, fontFace, fontScale, color, thickness)`（`main.cpp:29-30`） | `org` 是文字**左下角**（不是左上！）—— 所以用 `y-5` 把标签放到框上方；字体用 `FONT_HERSHEY_SIMPLEX` |
| `cv::Scalar` | 本质是 **4 元素 double 向量**，当颜色用时顺序是 B,G,R,(A) |
| （延伸） | `circle/line/ellipse/polylines`、`addWeighted` 做半透明、`LINE_AA` 抗锯齿 |

---

## §8 基础类型速查

| 类型 | 结构 | 坑 |
|---|---|---|
| `cv::Point` / `Point2f` | `x, y` | — |
| `cv::Size` / `Size2f` | **`width, height`** | 构造是先宽后高，与 `Mat(rows, cols)` 相反 |
| `cv::Rect` | `x, y, width, height` | `tl()/br()/area()/contains()`；width 是右开语义 |
| `cv::Scalar` | 4 元素 double，BGR(A) 序 | 名字带 "Scalar" 但其实是向量 |
| `cv::Mat` | 见 §1 | 高维时别用 rows/cols |

类型宏：`CV_8U` / `CV_32F` 是**深度**；`CV_8UC3` / `CV_32FC1` 是**完整类型**（深度 + 通道数），等价于 `CV_MAKETYPE(CV_8U, 3)` 这种写法。

---

## §9 本项目中的易错点（自查清单）

1. `imread` 失败**不抛异常**、只返回空 Mat —— 必须用 `empty()` 判断。本项目做了两道检查：`main.cpp:14-17`（命令行入口）和 `cpp:19-21`（`preprocess` 内抛 `std::invalid_argument`，由 `main` 的 `try/catch` 兜底）。
2. `640` 硬编码在三处（`hpp:34-35`、`cpp:111`、`scripts/export_onnx.py:19`）—— 必须一致，仍无机制保证（README 路线图中已列为待办）。
3. ROI 是视图不是拷贝（`cpp:40-41`）—— 赋值 ≠ copyTo。
4. 高维 Mat 先 `reshape` 再访问 `rows/cols`（`cpp:50`）。
5. 缩小图用的是**默认** `INTER_LINEAR`（`cpp:36` 省略了插值参数）—— 可实验 `INTER_AREA` 的差异。
6. 边界钳制的区间语义：`right/bottom` 钳到 `width/height`（**上界开区间**，`cpp:79-80`），与 `cv::Rect` 的右开语义一致；若顺手钳成 `size-1`（闭区间），贴边的框会短 1px —— 一个容易写错的细节。
7. 浮点 Mat 直接 `imshow` 会按 [0,1] 映射，调试 blob 时注意。
8. `imshow` 之后必须 `waitKey`，否则窗口不刷新。
9. blob 的 `channels()==1` + `dims==4`，别当普通三通道图处理。
10. `mean` 参数的单位是 0-255 而不是 0-1。
11. 极端长宽比的图缩放后，目标尺寸可能被 `round` 算成 0，而 `resize` 的目标尺寸出现 0 会直接抛异常 —— 本项目把 `new_w/new_h` 钳到 ≥1（`cpp:29-30`）来规避（见 §3.1）。

---

## §10 学习路线

依赖顺序（也是本项目代码的阅读顺序）：

```
Mat 内存模型 → 类型系统 → 像素访问三方式 → imgproc 几何变换
→ highgui / I/O → dnn（blob → Net → forward → 解码 → NMS）→ VideoCapture
```

各阶段对应的项目代码：

| 阶段 | 先读 |
|---|---|
| Mat 内存模型 | `cpp:39-41`（构造 + ROI + copyTo）、`cpp:50-60`（reshape/transpose/视图/空图检查） |
| 类型系统 | `hpp:11-22`、`cpp:60` |
| 几何变换 | `cpp:18-43`（preprocess 全文） |
| highgui / I/O | `main.cpp` 全文（顺带看 `cpp:19-21` 的空图检查） |
| dnn | `cpp:6-15`（构造）、`cpp:100-124`（detect）、`cpp:46-97`（postprocess） |
| 下一步 | `cv::VideoCapture` + 本项目的 detect 循环 = v0.2 |

---

## §11 练习与自测题

### 练习（基于本项目改代码，从易到难）

1. `main.cpp:6-17, 35-38` 已有参数校验、判空与 try/catch。分别用「不带参数」「不存在的图片路径」「不存在的模型路径」运行，确认三条错误路径的输出符合预期；再打印 `image.type()/size()/channels()`。
2. 在 `preprocess` 里 `cv::imwrite` 存下 letterbox 图，肉眼确认 114 灰边和 pad 位置。
3. 极端长宽比实验：造一张 2000×10 的图，先正常运行；再注释掉 `cpp:29-30` 的钳位，观察 `resize` 抛出的异常，理解那两行的意义。
4. 把 `cpp:52` 之后的裸指针访问改写成 `output_transposed.ptr<float>(i)` 版本，再改成 `.row(i)` 视图版本，比较可读性与性能。
5. 在 letterbox 图上用 `cv::rectangle` 画出所有候选框（含被过滤掉的低分框），直观理解坐标变换与过滤过程。
6. 手写 NMS（IoU + 排序 + 贪心）与 `cv::dnn::NMSBoxes` 对拍。
7. 用 `getPerfProfile` 看各层耗时；对比 `INTER_LINEAR` / `INTER_AREA` 的效果与耗时。
8. 用 `cv::VideoCapture` 把 main 改成视频循环（`waitKey(1)` 控制节奏）—— 这就是 v0.2 的雏形；再用 `cv::dnn::DetectionModel` 复现同样功能做对拍。

### 自测题（能全答上说明基础过关）

- `cv::Mat b = a;` 和 `cv::Mat b = a.clone();` 有什么区别？ROI 改写属于哪种？
- `reshape` 为什么要求连续？`transpose` 为什么不要求？
- `Size(640, 480)` 里哪个是宽？和 `Mat(640, 480, ...)` 有何不同？
- `blobFromImage` 会保持长宽比吗？本项目为什么还要自己写 letterbox？
- `cv::Scalar(0, 0, 255)` 是什么颜色？为什么？
- `minMaxLoc` 返回的 `Point` 里 x、y 分别是什么？单行矩阵为什么只看 x？
- `Mat::empty()` 为真意味着什么？为什么 `imread` 失败必须这样检测？
- 把 `new_w/new_h` 钳到至少 1 是为了防什么？
- `setPreferableBackend/Target` 被注释掉后程序行为变了吗？为什么？

---

*本笔记随项目代码同步维护；新增知识点时按 §1~§8 的模块归类追加。*
