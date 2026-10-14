#!/usr/bin/env python3
"""Extrai todos os arquivos e diretórios de assets dos discos GameCube para data/"""
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
from extract_orig import Gcm


def extract_disc(gcm_path, dest_dir):
    print(f"Extraindo {gcm_path} -> {dest_dir}...")
    g = Gcm(gcm_path)
    count = 0
    total_bytes = 0
    for path, offset, size in g.files.values():
        out_path = os.path.join(dest_dir, path)
        os.makedirs(os.path.dirname(out_path), exist_ok=True)
        if not os.path.exists(out_path) or os.path.getsize(out_path) != size:
            g.f.seek(offset)
            with open(out_path, 'wb') as f:
                f.write(g.f.read(size))
            count += 1
            total_bytes += size
    print(f"Extraídos {count} arquivos ({total_bytes / (1024 * 1024):.1f} MB) de {gcm_path}")


if __name__ == '__main__':
    dest = sys.argv[1] if len(sys.argv) > 1 else 'data'
    os.makedirs(dest, exist_ok=True)
    for disc in ['orig/G4BE08/re4_disc1.gcm', 'orig/G4BE08/re4_disc2.gcm']:
        if os.path.exists(disc):
            extract_disc(disc, dest)
