#!/usr/bin/env python3
"""Markdown 内部链接校验：相对路径是否存在、锚点是否真的能被解析。

只检查仓库内部链接（相对路径与 #锚点），不检查 http(s) 外链 ——
外链依赖网络，会让 CI 随机变红；锚点和路径才是文档重构时真正会断的东西。

典型能抓到的问题：
    [README](../README.md#v021-计划)   ← README 里根本没有这个标题
    [笔记](docs/opencv-notes.md)        ← 文件已被删除

另外，对带「## 目录」的文件还会检查目录覆盖率：新增了章节却忘了往目录里加一条，
断链检查是发现不了的（没有链接可断），这里会报「标题未出现在目录中」。
校验层级是自适应的 —— 目录里出现过的最大层级，其内的标题才要求全部收录。

在 GitHub Actions 里运行时会输出 ::error:: 注解，直接标在断链所在的行上。
"""

from __future__ import annotations

import os
import re
import sys
from collections import namedtuple
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parent.parent

SKIP_DIRS = {".git", "build", "venv", ".venv", "node_modules", ".vscode"}

# ![alt](target "title") / [text](target)
LINK_RE = re.compile(r"!?\[[^\]]*\]\(\s*([^)\s]+)(?:\s+\"[^\"]*\")?\s*\)")
HTML_ANCHOR_RE = re.compile(r"<a\s+[^>]*(?:name|id)=\"([^\"]+)\"", re.I)
FENCE_RE = re.compile(r"^\s*(```|~~~)")
HEADING_RE = re.compile(r"^(#{1,6})\s+(.*?)\s*#*\s*$")


def github_slug(heading: str) -> str:
    """近似复刻 GitHub 的标题锚点算法。"""
    text = re.sub(r"!?\[([^\]]*)\]\([^)]*\)", r"\1", heading)  # 链接保留文字
    text = re.sub(r"<[^>]+>", "", text)                        # 去掉 html 标签
    text = text.replace("`", "").replace("*", "")
    text = text.strip().lower()
    # 去掉标点，保留字母数字下划线、空格、连字符以及 CJK
    text = re.sub(r"[^\w\s-]", "", text, flags=re.UNICODE)
    return text.strip().replace(" ", "-")


Heading = namedtuple("Heading", "level text line")


def scan_headings(lines: list[str]) -> list[Heading]:
    """扫出所有真实标题，跳过代码块内的 # 注释。"""
    headings: list[Heading] = []
    in_fence = False
    for number, line in enumerate(lines, start=1):
        if FENCE_RE.match(line):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        match = HEADING_RE.match(line)
        if match:
            headings.append(Heading(len(match.group(1)), match.group(2).strip(), number))
    return headings


def iter_anchored_headings(headings: list[Heading]):
    """按 GitHub 规则逐个配锚点：重名标题依次追加 -1 / -2。"""
    counters: dict[str, int] = {}
    for heading in headings:
        slug = github_slug(heading.text)
        if not slug:
            continue
        if slug in counters:
            counters[slug] += 1
            yield heading, f"{slug}-{counters[slug]}"
        else:
            counters[slug] = 0
            yield heading, slug


def collect_anchors(path: Path) -> set[str]:
    """收集一个 Markdown 文件里所有可跳转的锚点（标题 + 显式 HTML 锚点）。"""
    lines = path.read_text(encoding="utf-8").splitlines()
    anchors = {anchor for _, anchor in iter_anchored_headings(scan_headings(lines))}

    in_fence = False
    for line in lines:
        if FENCE_RE.match(line):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        anchors.update(HTML_ANCHOR_RE.findall(line))
    return anchors


TOC_TITLE = "目录"


def check_toc_coverage(path: Path) -> list[tuple[int, str]]:
    """目录覆盖率：带「## 目录」的文件里，每个标题都该在目录中有条目。

    覆盖层级是自适应的 —— 只看目录里已经出现过的最大层级，
    比如目录里没有 H3 条目，就不会要求 H3 必须收录。
    """
    lines = path.read_text(encoding="utf-8").splitlines()
    headings = scan_headings(lines)

    toc = next((h for h in headings if h.text == TOC_TITLE), None)
    if toc is None:
        return []  # 没有目录的文件不适用本项检查

    # 目录区段：目录标题之后，到下一个同级或更高级标题为止
    end = next(
        (h.line for h in headings if h.line > toc.line and h.level <= toc.level),
        len(lines) + 1,
    )

    listed: set[str] = set()
    for line in lines[toc.line:end - 1]:
        for target in LINK_RE.findall(line):
            target = unquote(target.strip())
            if target.startswith("#"):
                listed.add(target[1:])

    if not listed:
        return []

    level_of = {anchor: heading.level for heading, anchor in iter_anchored_headings(headings)}
    covered = {level_of[anchor] for anchor in listed if anchor in level_of}
    max_level = max(covered) if covered else toc.level

    problems: list[tuple[int, str]] = []
    for heading, anchor in iter_anchored_headings(headings):
        if heading.line == toc.line:  # 目录标题自身不要求自引用
            continue
        if heading.level > max_level:
            continue
        if anchor not in listed:
            problems.append((heading.line, f"标题未出现在目录中：{heading.text}"))
    return problems


def iter_markdown_files() -> list[Path]:
    return sorted(
        path
        for path in ROOT.rglob("*.md")
        if not any(part in SKIP_DIRS for part in path.relative_to(ROOT).parts)
    )


def report(message: str, path: Path, line: int) -> None:
    rel = path.relative_to(ROOT)
    if os.environ.get("GITHUB_ACTIONS"):
        print(f"::error file={rel},line={line}::{message}")
    else:
        print(f"错误：{rel}:{line}  {message}")


def main() -> int:
    files = iter_markdown_files()
    anchor_cache: dict[Path, set[str]] = {}
    broken_links = 0
    toc_gaps = 0
    checked = 0

    for path in files:
        in_fence = False
        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
            if FENCE_RE.match(line):
                in_fence = not in_fence
                continue
            if in_fence:
                continue

            for raw_target in LINK_RE.findall(line):
                target = unquote(raw_target.strip())
                if re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", target):
                    continue  # http: https: mailto: 等外链一律跳过

                checked += 1
                file_part, _, fragment = target.partition("#")

                # 定位被链接的文件
                if file_part:
                    linked = (path.parent / file_part).resolve()
                    try:
                        linked.relative_to(ROOT)
                    except ValueError:
                        report(f"链接指向仓库之外的路径：{target}", path, number)
                        broken_links += 1
                        continue
                    if not linked.exists():
                        report(f"链接目标不存在：{target}", path, number)
                        broken_links += 1
                        continue
                else:
                    linked = path

                if not fragment:
                    continue

                if linked.suffix.lower() != ".md":
                    continue  # 非 Markdown 目标不做锚点校验

                if linked not in anchor_cache:
                    anchor_cache[linked] = collect_anchors(linked)
                if fragment not in anchor_cache[linked]:
                    report(f"锚点不存在：{target}", path, number)
                    broken_links += 1

        # 断链之外的第二类问题：新增了章节但忘了往目录里加
        gaps = check_toc_coverage(path)
        for number, message in gaps:
            report(message, path, number)
        toc_gaps += len(gaps)

    if broken_links or toc_gaps:
        summary = []
        if broken_links:
            summary.append(f"{broken_links} 处断链")
        if toc_gaps:
            summary.append(f"{toc_gaps} 处目录遗漏")
        print(f"\n共发现 {'、'.join(summary)}（已检查 {len(files)} 个文件 / {checked} 条内部链接）")
        return 1

    print(f"链接检查通过：{len(files)} 个文件 / {checked} 条内部链接")
    return 0


if __name__ == "__main__":
    sys.exit(main())
