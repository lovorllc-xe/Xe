#!/usr/bin/env python3
"""Generates libraries/vkscene.xel with the SPIR-V of scripts/shaders/lit.{vert,frag} embedded.
Needs glslc (Vulkan SDK / shaderc). Usage: scripts/gen-vkscene.py"""
import os, subprocess, tempfile
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
def spirv(name):
    with tempfile.NamedTemporaryFile(suffix=".spv") as f:
        subprocess.check_call(["glslc", os.path.join(root, "scripts/shaders", name), "-o", f.name])
        return open(f.name, "rb").read()
def hex_lines(data):
    h = data.hex()
    return "\n".join('        text := str.append(text, "%s")' % h[i:i + 96] for i in range(0, len(h), 96))
template = open(os.path.join(root, "scripts/vkscene.template")).read()
out = template.replace("@@VERT@@", hex_lines(spirv("lit.vert"))).replace("@@FRAG@@", hex_lines(spirv("lit.frag")))
open(os.path.join(root, "libraries/vkscene.xel"), "w").write(out)
print("wrote libraries/vkscene.xel")
