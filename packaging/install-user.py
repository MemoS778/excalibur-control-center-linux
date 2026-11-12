#!/usr/bin/env python3
"""Install the built GTK application and a desktop entry for the current user."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

if os.geteuid() == 0:
    raise SystemExit('Bu kurulumu sudo olmadan, kendi kullanıcınızla çalıştırın.')
repo = Path(__file__).resolve().parent.parent
home = Path.home()
data = Path(os.environ.get('XDG_DATA_HOME') or home / '.local/share')
if not data.is_absolute():
    raise SystemExit('XDG_DATA_HOME mutlak bir dizin olmalıdır.')
app = home / '.local/lib/excalibur-rgb-control'
applications = data / 'applications'
icons = data / 'icons/hicolor/scalable/apps'
for directory in (app, applications, icons):
    directory.mkdir(parents=True, exist_ok=True)

def copy_atomic(source, target, mode):
    fd, name = tempfile.mkstemp(dir=target.parent)
    os.close(fd)
    try:
        shutil.copyfile(source, name)
        os.chmod(name, mode)
        os.replace(name, target)
    finally:
        Path(name).unlink(missing_ok=True)

copy_atomic(repo / 'ecc', app / 'ecc', 0o755)
copy_atomic(repo / 'packaging/launch.sh', app / 'launch.sh', 0o755)
copy_atomic(repo / 'packaging/excalibur-rgb-control.svg', icons / 'excalibur-rgb-control.svg', 0o644)
# Desktop Exec has its own quoting rules, followed by desktop-string unescaping.
def desktop_arg(value):
    value = str(value).replace('%', '%%')
    value = ''.join('\\' + c if c in '\\"`$' else c for c in value)
    return ('"' + value + '"').replace('\\', '\\\\')
entry = applications / 'io.github.MemoS778.ExcaliburRGB.desktop'
entry.write_text(f'''[Desktop Entry]
Type=Application
Version=1.0
Name=Excalibur RGB Control
Comment=Excalibur klavye rengi, efektleri ve parlaklığı
Exec={desktop_arg(app / 'launch.sh')}
Icon=excalibur-rgb-control
Terminal=true
Categories=Settings;HardwareSettings;
Keywords=Excalibur;RGB;keyboard;klavye;aydınlatma;
StartupNotify=false
''')
entry.chmod(0o644)
if shutil.which('desktop-file-validate'):
    subprocess.run(['desktop-file-validate', str(entry)], check=True)
if shutil.which('update-desktop-database'):
    subprocess.run(['update-desktop-database', str(applications)], check=True)
print(f'Uygulama menüsüne eklendi: {entry}')
print(f'Uygulama dosyaları: {app}')
