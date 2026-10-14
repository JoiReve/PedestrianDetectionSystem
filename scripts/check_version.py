#!/usr/bin/env python3
"""版本号校验：保证版本号只有一个真源。

真源：CMakeLists.txt 里 project(... VERSION x.y.z)
镜像：README.md 首屏的 **vx.y.z** 一行（给人看的那句）

两者的版本号必须一致，否则退出码 1。
README 其余位置如果又冒出别的版本号字面量，只提示、不判失败 ——
历史版本应写进 CHANGELOG.md，README 只描述"当前是什么"。

在 GitHub Actions 里运行时会输出 ::error:: 注解，直接标在文件行上。
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CMAKE_LISTS = ROOT / "CMakeLists.txt"
README = ROOT / "README.md"

# README 的镜像行只允许出现在前若干行内，避免误抓正文里的历史版本号
README_HEAD_LINES = 15

VERSION_IN_CMAKE = re.compile(r"project\s*\([^)]*?VERSION\s+(\d+\.\d+\.\d+)", re.S)
VERSION_ANYWHERE = re.compile(r"\bv(\d+\.\d+\.\d+)\b")


def report_error(message: str, path: Path | None = None, line: int | None = None) -> None:
    if os.environ.get("GITHUB_ACTIONS") and path is not None:
        where = f" file={path.relative_to(ROOT)}"
        if line is not None:
            where += f",line={line}"
        print(f"::error{where}::{message}")
    else:
        location = f"{path.relative_to(ROOT)}" if path else ""
        if line is not None:
            location += f":{line}"
        print(f"错误：{location} {message}" if location else f"错误：{message}")


def main() -> int:
    for path in (CMAKE_LISTS, README):
        if not path.is_file():
            report_error(f"找不到文件：{path}")
            return 1

    cmake_text = CMAKE_LISTS.read_text(encoding="utf-8")
    match = VERSION_IN_CMAKE.search(cmake_text)
    if not match:
        report_error("未能从 project(...) 中解析出版本号", CMAKE_LISTS)
        return 1
    cmake_version = match.group(1)
    cmake_line = cmake_text[: match.start()].count("\n") + 1

    # README 首屏找 **vX.Y.Z**
    readme_lines = README.read_text(encoding="utf-8").splitlines()
    readme_version: str | None = None
    readme_line: int | None = None
    for index, text in enumerate(readme_lines[:README_HEAD_LINES], start=1):
        found = VERSION_ANYWHERE.search(text)
        if found:
            readme_version = found.group(1)
            readme_line = index
            break

    if readme_version is None:
        report_error(
            f"README 前 {README_HEAD_LINES} 行里找不到版本号（期望形如 **v{cmake_version}**）",
            README,
        )
        return 1

    if readme_version != cmake_version:
        report_error(
            f"版本号不一致：CMakeLists.txt 是 {cmake_version}，README.md 是 {readme_version}",
            README,
            readme_line,
        )
        report_error(
            f"CMakeLists.txt 的真源是 {cmake_version}",
            CMAKE_LISTS,
            cmake_line,
        )
        return 1

    # 通过后，顺带提示 README 里是否还有多余版本号字面量
    extras = [
        (number, text.strip())
        for number, text in enumerate(readme_lines, start=1)
        if VERSION_ANYWHERE.search(text) and number != readme_line
    ]
    print(f"版本号一致：v{cmake_version}（CMakeLists.txt:{cmake_line} / README.md:{readme_line}）")
    if extras:
        print(f"提示：README.md 另有多余的版本号字面量 {len(extras)} 处，建议移入 CHANGELOG.md：")
        for number, text in extras:
            snippet = text if len(text) <= 60 else text[:57] + "..."
            print(f"  README.md:{number}  {snippet}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
