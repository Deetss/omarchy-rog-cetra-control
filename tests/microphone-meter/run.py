"""Exercise the real QML meter and timers with a fake process, never capture audio."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='cetra-meter-qt-') as temp:
    out = Path(temp)
    def write(name, data):
        p = out / name
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(data)
    write('Quickshell/Io/qmldir', 'module Quickshell.Io\nProcess 1.0 Process.qml\nSplitParser 1.0 SplitParser.qml\nsingleton Probe 1.0 Probe.qml\n')
    write('Quickshell/Io/Probe.qml', 'pragma Singleton\nimport QtQml\nQtObject { property var process: null }\n')
    write('Quickshell/Io/Process.qml', '''import QtQuick
Item {
 property bool running: false
 property bool stdinEnabled: false
 property var stdout
 property var command: []
 property int processId: 0
 function signal(number) { running = false }
 Component.onCompleted: Probe.process = this
}
''')
    write('Quickshell/Io/SplitParser.qml', 'import QtQml\nQtObject { signal read(string line) }\n')
    shutil.copy2(root / 'MicrophoneMeter.qml', out / 'MicrophoneMeter.qml')
    shutil.copy2(root / 'tests/microphone-meter/Fixture.qml', out / 'Fixture.qml')
    flags = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'Qt6Quick', 'Qt6Qml', 'Qt6Gui'], text=True))
    subprocess.run(['c++','-std=c++17','-Wall','-Wextra','-Werror',str(root/'tests/microphone-meter/qt.cpp'),'-o',str(out/'qt'),*flags],check=True,timeout=60)
    subprocess.run([str(out/'qt'),str(out)],check=True,timeout=12,env={**os.environ,'QT_QPA_PLATFORM':'offscreen'})
