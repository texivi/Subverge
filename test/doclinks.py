#!/usr/bin/env python3
# link check
import glob, os, re, sys
pages = {os.path.basename(f)[:-3] for f in glob.glob('doc/*.md')} | {'word-index', 'a-to-z'}
bad = 0
for f in sorted(glob.glob('doc/*.md')):
    for m in re.finditer(r'\]\(([^)]*)\)', open(f).read()):
        g = m.group(1)
        if g[0] in '?>' or g.startswith('http'):
            continue
        p = g.split('#')[0]
        p = p[:-3] if p.endswith('.md') else p
        if g[0] in '@!' or (p and p not in pages):
            print(f'{f}: {g}')
            bad += 1
print(f'{bad} problems')
sys.exit(bad > 0)
