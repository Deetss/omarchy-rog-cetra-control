"""Real QQml bindings with controlled native model replacements; no hardware I/O."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='cetra-bt-qt-', dir='/tmp/opencode') as tmp:
    out = Path(tmp)
    def write(name, value):
        path = out / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(value)
    write('Quickshell/qmldir', 'module Quickshell\nsingleton Quickshell 1.0 Quickshell.qml\n')
    write('Quickshell/Quickshell.qml', 'pragma Singleton\nimport QtQml\nQtObject { function env(name) { return "0" } }\n')
    write('Quickshell/Bluetooth/qmldir', 'module Quickshell.Bluetooth\nsingleton Bluetooth 1.0 Bluetooth.qml\n')
    write('Quickshell/Bluetooth/Bluetooth.qml', '''pragma Singleton
import QtQml
QtObject {
  readonly property QtObject devices: QtObject { property var values: [] }
  readonly property QtObject adapters: QtObject { property var values: [] }
}
''')
    write('Quickshell/Services/Pipewire/qmldir', '''module Quickshell.Services.Pipewire
singleton Pipewire 1.0 Pipewire.qml
singleton PwLinkState 1.0 PwLinkState.qml
singleton PwNodeType 1.0 PwNodeType.qml
PwObjectTracker 1.0 PwObjectTracker.qml
''')
    write('Quickshell/Services/Pipewire/Pipewire.qml', '''pragma Singleton
import QtQml
QtObject {
  readonly property bool ready: true
  readonly property QtObject nodes: QtObject { property var values: [] }
  readonly property QtObject links: QtObject { property var values: [] }
}
''')
    write('Quickshell/Services/Pipewire/PwNodeType.qml', 'pragma Singleton\nimport QtQml\nQtObject { enum Type { AudioSource = 9 } }\n')
    write('Quickshell/Services/Pipewire/PwLinkState.qml', 'pragma Singleton\nimport QtQml\nQtObject { enum State { Active = 6 } }\n')
    write('Quickshell/Services/Pipewire/PwObjectTracker.qml', 'import QtQml\nQtObject { property var objects: [] }\n')
    for name in ('AudioTopology.qml', 'CetraBluetooth.qml'):
        shutil.copy2(root / name, out / name)
    shutil.copy2(root / 'tests/bluetooth/Fixture.qml', out / 'Fixture.qml')
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Quick', 'Qt6Qml', 'Qt6Gui'], text=True))
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', str(root / 'tests/bluetooth/qt.cpp'), '-o', str(out / 'qt'), *flags], check=True, timeout=60)
    subprocess.run([str(out / 'qt'), str(out), str(out / 'Fixture.qml')], check=True, timeout=20,
                   env={**os.environ, 'QT_QPA_PLATFORM':'offscreen', 'QT_LOGGING_RULES':'*.debug=false;*.info=true;*.warning=true;*.critical=true'})
