#!/usr/bin/env python3
"""将 templates/index.html 转换为 C 头文件 index_html.h
在编译前执行: python3 gen_index_h.py
"""
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
HTML_PATH = os.path.join(SCRIPT_DIR, "templates", "index.html")
OUT_PATH  = os.path.join(SCRIPT_DIR, "index_html.h")

with open(HTML_PATH, "r", encoding="utf-8") as f:
    html = f.read()

lines = ['/* 自动生成 - 勿手动编辑 */',
         '#pragma once',
         'static const char INDEX_HTML[] =']

for line in html.split('\n'):
    escaped = line.replace('\\', '\\\\').replace('"', '\\"')
    lines.append(f'"{escaped}\\n"')

lines.append(';')

with open(OUT_PATH, "w", encoding="utf-8") as f:
    f.write('\n'.join(lines) + '\n')

print(f"生成: {OUT_PATH} ({len(lines)} 行)")
