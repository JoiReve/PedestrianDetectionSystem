#!/usr/bin/env python3
import hashlib
import os
import pathlib
import re
import statistics
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
BIN = ROOT / "build-release" / "pedestrian_detection_system"
MODEL = ROOT / "models" / "yolov8n.onnx"
VIDEO = ROOT / "assets" / "VideoSample.mp4"

RUNS = 3
WARMUP = 20
ENGINES = ["classic", "new", "auto"]

BLOCK = re.compile(r">>> \[(Cold Start|Steady State)\] Frame (\d+)")
ROW = re.compile(
    r"^\s*(EndToEnd|Read|InferChain|Preprocess|Reason|Postprocess|Draw|DisplayNWait)"
    r"\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)%"
)


def parse(text):
    cold = {}
    steady = []
    mode = None
    for line in text.splitlines():
        m = BLOCK.search(line)
        if m:
            mode = "cold" if m.group(1) == "Cold Start" else "steady"
            if mode == "steady":
                steady.append({})
            continue
        m = ROW.match(line)
        if not m or mode is None:
            continue
        stage, _total, avg, _ratio = m.group(1), float(m.group(2)), float(m.group(3)), float(m.group(4))
        if mode == "cold":
            cold[stage] = avg
        else:
            steady[-1][stage] = avg
    return cold, steady


def p95(values):
    values = sorted(values)
    return values[min(len(values) - 1, int(0.95 * (len(values) - 1)))]


def main():
    env = os.environ.copy()
    env.pop("OPENCV_FORCE_DNN_ENGINE", None)  # 避免环境变量覆盖 --engine
    env["OPENCV_LOG_LEVEL"] = "WARNING"

    tmp = pathlib.Path(tempfile.mkdtemp(prefix="pds-bench-"))
    result = {}

    for run in range(RUNS):
        # 每轮轮换顺序，减少热漂移偏差
        order = ENGINES[run % len(ENGINES):] + ENGINES[:run % len(ENGINES)]
        for eng in order:
            csv = tmp / f"{eng}_{run}.csv"
            cmd = [
                str(BIN), "--no-show", "--engine", eng,
                "--dump", str(csv), str(MODEL), str(VIDEO),
            ]
            proc = subprocess.run(cmd, cwd=ROOT, env=env, text=True, capture_output=True)
            if proc.returncode != 0:
                raise SystemExit(f"{eng} run {run} failed:\n{proc.stderr}")

            cold, steady = parse(proc.stdout)
            d = result.setdefault(eng, {
                "cold_reason": [], "steady_reason": [], "steady_chain": [],
                "hash": set(), "layers": set(), "warn": 0,
            })
            if "Reason" in cold:
                d["cold_reason"].append(cold["Reason"])
            d["steady_reason"] += [b["Reason"] for b in steady[WARMUP:] if "Reason" in b]
            d["steady_chain"] += [b["InferChain"] for b in steady[WARMUP:] if "InferChain" in b]
            d["hash"].add(hashlib.sha256(csv.read_bytes()).hexdigest())
            m = re.search(r"layers\s*=\s*(\d+)", proc.stdout)
            if m:
                d["layers"].add(int(m.group(1)))
            d["warn"] += sum("WARN" in line for line in proc.stderr.splitlines())

    print("| Engine | layers | first Reason (ms) | steady Reason med/P95 (ms) | steady InferChain med/P95 (ms) | stderr WARN | CSV sha256 |")
    print("|---|---:|---:|---:|---:|---:|---|")
    for eng in ENGINES:
        d = result[eng]
        hash_text = sorted(d["hash"])[0] if len(d["hash"]) == 1 else "MISMATCH"
        cold_str = f"{statistics.median(d['cold_reason']):.2f}" if d['cold_reason'] else "N/A"
        print(
            f"| {eng} | {sorted(d['layers'])[0] if len(d['layers']) == 1 else 'MISMATCH'} "
            f"| {cold_str} "
            f"| {statistics.median(d['steady_reason']):.2f} / {p95(d['steady_reason']):.2f} "
            f"| {statistics.median(d['steady_chain']):.2f} / {p95(d['steady_chain']):.2f} "
            f"| {d['warn']} | {hash_text} |"
        )


if __name__ == "__main__":
    main()
