#!/usr/bin/env python3
"""deferred_scan.py: detect units whose parse-time symbol numbers fall through .text (MWCC -inline deferred)."""
import json, subprocess, re, os, bisect
units = json.load(open('objdiff.json'))['units']
def tau(o):
    if not o or not os.path.exists(o): return None
    rel = subprocess.run(['build/binutils/powerpc-eabi-objdump','-r','-j','.text',o],capture_output=True,text=True).stdout
    first = {}
    for l in rel.splitlines():
        p = l.split()
        if len(p) < 3: continue
        try: off = int(p[0], 16)
        except ValueError: continue
        name = p[2].split('+')[0]
        m = re.match(r'@(\d+)$', name) or re.match(r'.+\$(\d+)$', name)
        if not m: continue
        first.setdefault(name, (off, int(m.group(1))))
    syms = subprocess.run(['build/binutils/powerpc-eabi-objdump','-t',o],capture_output=True,text=True).stdout
    secof = {}
    for l in syms.splitlines():
        p = l.split()
        if len(p) >= 5: secof[p[-1]] = p[-3]
    pts = [v for k, v in first.items() if secof.get(k) in ('.rodata', '.data', '.bss', '.sbss', '.sdata')]
    if len(pts) < 4: return None
    pts.sort()
    c = d = 0
    for i in range(len(pts)):
        for j in range(i + 1, len(pts)):
            if pts[j][1] > pts[i][1]: c += 1
            elif pts[j][1] < pts[i][1]: d += 1
    return (c - d) / max(1, c + d), len(pts)
for u in units:
    t = tau(u.get('target_path'))
    if t and t[0] < -0.5:
        b = tau(u.get('base_path'))
        print(f"{u['name']:60s} retail tau {t[0]:+.2f} (n={t[1]})  ours {'' if not b else f'{b[0]:+.2f}'}")
