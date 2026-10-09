# 把 PyTorch 格式的 YOLO 模型 .pt 转换成 ONNX 格式的 .onnx
from pathlib import Path
import shutil
from ultralytics import YOLO

# 脚本从 scripts/ 定位到项目根 project/
ROOT_DIR = Path(__file__).resolve().parent.parent
# 拼接 PyTorch 模型路径
PT_PATH = ROOT_DIR / "models" / "face_detection" / "yolov8n.pt"
# 拼接最终 ONNX 保存路径
TARGET_ONNX_PATH = ROOT_DIR / "models" / "yolov8n.onnx"

# 加载 YOLO 模型
print(f"Loading the YOLO model: {PT_PATH}")
model = YOLO(str(PT_PATH))

# 导出 ONNX 模型
print(f"Exporting the ONNX (image size = 640)...")
exported_path = Path(model.export(format="onnx", imgsz=640))

if TARGET_ONNX_PATH.exists():
    TARGET_ONNX_PATH.unlink()

# 移动 ONNX 文件到对应的路径下
shutil.move(str(exported_path), str(TARGET_ONNX_PATH))
print(f"Export successful, saved to: {TARGET_ONNX_PATH}")
