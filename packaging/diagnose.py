#!/usr/bin/env python3
"""Read-only RGB inventory; no EC access, sensor queries or serial numbers."""
import hashlib
import json
from pathlib import Path

repo = Path(__file__).resolve().parent.parent

def read(path):
    try:
        return path.read_text().strip()
    except OSError as error:
        return f'unavailable: {error.strerror}'

def binary(path):
    try:
        return {'path': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
    except OSError as error:
        return {'path': str(path), 'error': error.strerror}

print(json.dumps({
    'dmi': {name: read(Path('/sys/class/dmi/id') / name)
            for name in ('product_name', 'board_name', 'bios_version')},
    'wmi_devices': sorted(p.name for p in Path('/sys/bus/wmi/devices').glob('*')),
    'rgb_led_nodes': sorted(str(p) for p in Path('/sys/class/leds').glob('*')
                            if 'casper' in p.name or 'excalibur' in p.name),
    'project_binary': binary(repo / 'ecc'),
    'installed_binary': binary(Path.home() / '.local/lib/excalibur-rgb-control/ecc'),
}, ensure_ascii=False, indent=2))
