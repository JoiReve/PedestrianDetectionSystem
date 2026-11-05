# 测试输入（冻结）

改动任何视频必须更新 sha256，并说明原因。

## 当前输入

| 项 | 值 |
|---|---|
| 路径 | `assets/VideoSample.mp4` |
| 分辨率 | 1280x720 |
| 帧率 | 24/1（avg 8928/373 ≈ 23.935 fps） |
| 帧数 | 372 |
| 时长 | 15.541667 s |
| 大小 | 3977281 B |
| sha256 | `f992f28f0b62ab647c9406ec59c1871441b777a4eac44684e0e10dae5e0739e4` |

## 复现

```bash
OPENCV_LOG_LEVEL=WARNING OPENCV_FORCE_DNN_ENGINE=3 \
  ./build-release/pedestrian_detection_system \
  --no-show --dump /tmp/auto.csv \
  ./models/yolov8n.onnx assets/VideoSample.mp4
```

> 视频走位置参数 `<Source Path>`，无 `--video`；顺序 `<Model> <Source>`。

## 预期基线（Release / OpenCV 5.0.0）

| 项 | 期望 |
|---|---|
| exit | 0 |
| 表头 | `id,x,y,w,h,score` |
| 数据行 | 2284 |
| 唯一 id | 372（0..371 连续） |
| ENGINE=1/2/3 sha256 | `a1a8441833c3be93c3d8d4400da88a3e46a292f667a1cd439e8ae73c9655ecfd` |
| ENGINE=4 sha256 | 同上；stderr 含 1 条 WARN（ORT 回退 AUTO） |

## 更新流程

```bash
sha256sum <video>
ffprobe -v error -select_streams v:0 \
  -show_entries stream=width,height,nb_frames,r_frame_rate,avg_frame_rate,duration \
  -of default=nw=1 <video>
ffprobe -v error -count_frames -select_streams v:0 \
  -show_entries stream=nb_read_frames -of default=nw=1 <video>
```
输出更新进上表。