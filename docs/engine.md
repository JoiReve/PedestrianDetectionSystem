# OpenCV DNN 引擎选型与限制

> 环境：OpenCV 5.0.0，Release，`WITH_ONNXRUNTIME=OFF`，`WITH_CUDA=OFF`。
> 固定输入：`assets/VideoSample.mp4`（参数与 sha256 见 [assets/frozen-test-input.md](../assets/frozen-test-input.md)）。

## 1. 引擎枚举

| 值 | 枚举 | 行为 |
|---|---:|---|
| 3 | `ENGINE_AUTO` | 默认；先尝试 NEW，加载失败回退 CLASSIC |
| 2 | `ENGINE_NEW` | 新图执行器；当前仅支持 CPU 后端 |
| 1 | `ENGINE_CLASSIC` | 4.x 延续引擎；支持非 CPU 后端，CUDA 走这条路 |
| 4 | `ENGINE_ORT` | OpenCV 内置 ONNX Runtime 包装；需要编译期 `WITH_ONNXRUNTIME=ON` |

项目通过 `--engine auto|new|classic|ort` 选择；默认 `auto`。

## 2. 关键约束

1. **engine 在模型加载期锁定**：`Net` 没有公开的 engine setter/getter，加载后不能从 NEW 切换到 CLASSIC。
2. **`ENGINE_NEW` 当前仅 CPU**：在 NEW 下调用 `setPreferableBackend` / `setPreferableTarget` 会打 warning，不会生效。上 CUDA 必须显式 `--engine classic` 重新加载。
3. **`ENGINE_ORT` 当前构建不可用**：传 `ort` 不抛异常，OpenCV 打一条 warning 后回退 `ENGINE_AUTO`；不能用“加载成功”推断 ORT 可用，要用 `cv::getBuildInformation()`。
4. **`OPENCV_FORCE_DNN_ENGINE` 优先于 `--engine`**：取值为 1/2/3/4，函数内 static、每进程只读一次；做矩阵时每个引擎单独起进程。
5. **逐层 profile 不跨引擎可比**：NEW 会做算子融合，层数和层名与 CLASSIC 不同；逐层耗时只在同引擎内比较。

## 3. 实测

命令：

```bash
OPENCV_LOG_LEVEL=WARNING ./build-release/pedestrian_detection_system \
  --no-show --engine <classic|new|auto|ort> \
  --dump /tmp/engine.csv \
  ./models/yolov8n.onnx assets/VideoSample.mp4
```

3 轮独立进程；1 cold + 371 steady；丢弃前 20 个稳态帧，每轮 351 帧参与统计。

| Engine | layers | first Reason (ms) | steady Reason med/P95 (ms) | steady InferChain med/P95 (ms) | WARN |
|---|---:|---:|---:|---:|---:|
| classic | 181 | 58.15 | 24.93 / 26.26 | 28.25 / 29.86 | 0 |
| new | 164 | 16.23 | 14.08 / 15.23 | 17.34 / 18.71 | 0 |
| auto | 164 | 16.64 | 14.01 / 15.22 | 17.24 / 18.76 | 0 |
| ort | N/A | — | — | — | 1（回退 AUTO→NEW） |

四个 CSV 的 sha256 均为 `a1a8441833c3be93c3d8d4400da88a3e46a292f667a1cd439e8ae73c9655ecfd`。口径是 `--dump` 坐标取整、分数 3 位精度；底层浮点是否逐位一致未验证。

## 4. 结论

1. **AUTO 命中 NEW**：auto layers=164 = new layers=164；steady Reason 14.01 ≈ 14.08 ms。
2. **NEW 比 CLASSIC 快约 43.5%**：24.93 ms → 14.08 ms；差距远大于轮次抖动（≤2.3%）。
3. **CLASSIC 冷启动代价更高**：first Reason 58.15 ms vs NEW 16.23 ms（约 3.6 倍）。
4. **检测框一致**：classic / new / auto / ort（回退）的 CSV sha256 相同。
5. **`layers` 可区分引擎**：classic=181，new/auto=164。
6. **ORT 当前不可用**：传 `ort` 会 warning 回退 AUTO，退出码 0。

## 5. 已知局限

- OpenCV 5.0 没有公开 API 查询“当前实例实际引擎”，本文用 `layers` 作为代理。
- 矩阵只覆盖 YOLOv8n + `VideoSample.mp4`。
- 当前构建没有 CUDA / ONNXRuntime，GPU 表现未验证。
- 散热/功耗未监测，数字未做频率归一化。

## 6. 复现

```bash
python3 scripts/benchmark_engines.py
```

脚本会按 `classic / new / auto` 各跑 3 轮，输出 median/P95、layers 和 CSV sha256。
