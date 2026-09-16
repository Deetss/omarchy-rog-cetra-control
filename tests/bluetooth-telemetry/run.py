"""Real QQml bindings for CetraTelemetry with controlled native process mock; no hardware I/O."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
temp_dir = '/tmp/opencode' if os.path.exists('/tmp/opencode') else None
with tempfile.TemporaryDirectory(prefix='cetra-telemetry-qt-', dir=temp_dir) as tmp:
    out = Path(tmp)

    def write(name, value):
        path = out / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(value)

    # Quickshell base mocks
    write('Quickshell/qmldir', 'module Quickshell\nsingleton Quickshell 1.0 Quickshell.qml\n')
    write('Quickshell/Quickshell.qml', '''pragma Singleton
import QtQml
QtObject {
  function env(name) { return "0" }
}
''')

    write('Quickshell/Io/qmldir', 'module Quickshell.Io\n')

    # Copy CetraTelemetry.qml into fixture environment
    src_telemetry = root / 'CetraTelemetry.qml'
    if not src_telemetry.exists():
        for candidate in [root / 'src' / 'CetraTelemetry.qml', root / 'components' / 'CetraTelemetry.qml']:
            if candidate.exists():
                src_telemetry = candidate
                break
    shutil.copy2(src_telemetry, out / 'CetraTelemetry.qml')

    # Locate moc binary
    moc = shutil.which('moc') or shutil.which('moc-qt6')
    if not moc:
        for candidate in ['/usr/lib/qt6/moc', '/usr/lib/qt6/libexec/moc', '/usr/lib/qt6/bin/moc', '/usr/bin/moc-qt6', '/usr/bin/moc']:
            if os.path.exists(candidate):
                moc = candidate
                break
    if not moc:
        try:
            host_bins = subprocess.check_output(['pkg-config', '--variable=host_bins', 'Qt6Core'], text=True).strip()
            candidate = os.path.join(host_bins, 'moc')
            if os.path.exists(candidate):
                moc = candidate
        except Exception:
            pass
    if not moc:
        moc = 'moc'

    qt_cpp = root / 'tests/bluetooth-telemetry/qt.cpp'
    subprocess.run([moc, str(qt_cpp), '-o', str(out / 'qt.moc')], check=True, timeout=30)

    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Quick', 'Qt6Qml', 'Qt6Gui'], text=True))
    subprocess.run(['c++', '-fPIC', '-std=c++17', '-Wall', '-Wextra', '-Werror', f'-I{out}', str(qt_cpp), '-o', str(out / 'qt'), *flags], check=True, timeout=60)
    subprocess.run([str(out / 'qt'), str(out)], check=True, timeout=20,
                   env={**os.environ, 'QT_QPA_PLATFORM': 'offscreen', 'QT_LOGGING_RULES': '*.debug=false;*.info=true;*.warning=true;*.critical=true'})
