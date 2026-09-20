// Independent real Qt fixture for CetraTelemetry without hardware or second Quickshell.
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QDateTime>
#include <QDebug>
#include <QVariant>
#include <QVariantMap>
#include <QJSValue>
#include <QMetaObject>
#include <iostream>
#include <cassert>
#include <memory>

static int warnings = 0;
static void messages(QtMsgType type, const QMessageLogContext &, const QString &message) {
  std::cerr << message.toStdString() << "\n";
  if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
    ++warnings;
  }
}

class MockSplitParser : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString split READ split WRITE setSplit NOTIFY splitChanged)
public:
  explicit MockSplitParser(QObject *parent = nullptr)
    : QObject(parent), m_split(QStringLiteral("\n")) {}

  QString split() const { return m_split; }
  void setSplit(const QString &s) {
    if (m_split != s) {
      m_split = s;
      emit splitChanged();
    }
  }

  Q_INVOKABLE void feed(const QString &data) {
    emit read(data);
  }
  Q_INVOKABLE void clear() {}
  Q_INVOKABLE void streamEnded(const QString &) {}

signals:
  void splitChanged();
  void read(const QString &data);

private:
  QString m_split;
};

class MockProcess : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
  Q_PROPERTY(QVariant command READ command WRITE setCommand NOTIFY commandChanged)
  Q_PROPERTY(QObject* stdout READ getStdout WRITE setStdout NOTIFY stdoutChanged)
  Q_PROPERTY(QObject* stderr READ getStderr WRITE setStderr NOTIFY stderrChanged)
  Q_PROPERTY(QString workingDirectory READ workingDirectory WRITE setWorkingDirectory NOTIFY workingDirectoryChanged)
  Q_PROPERTY(QVariantMap environment READ environment WRITE setEnvironment NOTIFY environmentChanged)
  Q_PROPERTY(int processId READ processId NOTIFY processIdChanged)

public:
  explicit MockProcess(QObject *parent = nullptr)
    : QObject(parent)
    , m_running(false)
    , m_command()
    , m_stdout(nullptr)
    , m_stderr(nullptr)
    , m_workingDirectory()
    , m_environment()
    , m_pid(0)
    , m_launchCount(0)
    , m_signals()
  {
    s_liveInstances++;
    s_totalCreated++;
    s_latest = this;
  }

  ~MockProcess() override {
    s_liveInstances--;
    if (s_latest == this) {
      s_latest = nullptr;
    }
  }

  bool isRunning() const { return m_running; }

  void setRunning(bool r) {
    if (r == m_running) return;
    if (r) {
      m_running = true;
      m_pid = 12000 + (++m_launchCount);
      emit started();
      emit runningChanged();
      emit processIdChanged();
    } else {
      // Per Quickshell process.cpp: setRunning(false) sends terminate()
      // but getter remains true until reaped.
      signal(15);
    }
  }

  QVariant command() const { return m_command; }
  void setCommand(const QVariant &c) {
    if (m_command != c) {
      m_command = c;
      emit commandChanged();
    }
  }

  QObject *getStdout() const { return m_stdout; }
  void setStdout(QObject *s) {
    if (m_stdout != s) {
      m_stdout = s;
      emit stdoutChanged();
    }
  }

  QObject *getStderr() const { return m_stderr; }
  void setStderr(QObject *s) {
    if (m_stderr != s) {
      m_stderr = s;
      emit stderrChanged();
    }
  }

  QString workingDirectory() const { return m_workingDirectory; }
  void setWorkingDirectory(const QString &w) {
    if (m_workingDirectory != w) {
      m_workingDirectory = w;
      emit workingDirectoryChanged();
    }
  }

  QVariantMap environment() const { return m_environment; }
  void setEnvironment(const QVariantMap &e) {
    if (m_environment != e) {
      m_environment = e;
      emit environmentChanged();
    }
  }

  int processId() const { return m_running ? m_pid : 0; }

  Q_INVOKABLE void signal(int sig) {
    m_signals.append(sig);
  }

  Q_INVOKABLE void kill() {
    signal(9);
  }

  Q_INVOKABLE void terminate() {
    signal(15);
  }

  Q_INVOKABLE void finish(int exitCode, const QString &stdoutLine = QString()) {
    if (!stdoutLine.isEmpty()) {
      feedStdout(stdoutLine);
    }
    // Silent getter change to false before exited per Quickshell process semantics
    m_running = false;
    emit exited(exitCode, 0);
    emit runningChanged();
    emit processIdChanged();
  }

  Q_INVOKABLE void failStart() {
    // Quickshell onErrorOccurred(FailedToStart): emit runningChanged() without exited
    m_running = false;
    emit runningChanged();
    emit processIdChanged();
  }

  void feedStdout(const QString &line) {
    if (m_stdout) {
      QMetaObject::invokeMethod(m_stdout, "read", Q_ARG(QString, line));
    }
  }

  int launchCount() const { return m_launchCount; }
  const QList<int> &receivedSignals() const { return m_signals; }
  void clearSignals() { m_signals.clear(); }

  static int liveInstances() { return s_liveInstances; }
  static int totalCreated() { return s_totalCreated; }
  static MockProcess *latest() { return s_latest; }

signals:
  void runningChanged();
  void commandChanged();
  void stdoutChanged();
  void stderrChanged();
  void workingDirectoryChanged();
  void environmentChanged();
  void processIdChanged();
  void started();
  void exited(int exitCode, int exitStatus);

private:
  bool m_running;
  QVariant m_command;
  QObject *m_stdout;
  QObject *m_stderr;
  QString m_workingDirectory;
  QVariantMap m_environment;
  int m_pid;
  int m_launchCount;
  QList<int> m_signals;

  static int s_liveInstances;
  static int s_totalCreated;
  static MockProcess *s_latest;
};

int MockProcess::s_liveInstances = 0;
int MockProcess::s_totalCreated = 0;
MockProcess *MockProcess::s_latest = nullptr;

static QVariant snapshotValue(QObject *telemetry) {
  const QVariant value = telemetry->property("snapshot");
  return value.metaType() == QMetaType::fromType<QJSValue>()
    ? value.value<QJSValue>().toVariant() : value;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <import-path>\n";
    return 1;
  }

  QGuiApplication app(argc, argv);
  qInstallMessageHandler(messages);

  // Register controlled Quickshell.Io mock types
  qmlRegisterType<MockProcess>("Quickshell.Io", 1, 0, "Process");
  qmlRegisterType<MockSplitParser>("Quickshell.Io", 1, 0, "SplitParser");
  qmlRegisterType<MockProcess>("Quickshell.Io", 0, 1, "Process");
  qmlRegisterType<MockSplitParser>("Quickshell.Io", 0, 1, "SplitParser");
  qmlRegisterType<MockProcess>("Quickshell.Io", 0, 3, "Process");
  qmlRegisterType<MockSplitParser>("Quickshell.Io", 0, 3, "SplitParser");

  QQmlEngine engine;
  QString outDir = QString::fromLocal8Bit(argv[1]);
  engine.addImportPath(outDir);

  QQmlComponent component(&engine, QUrl::fromLocalFile(outDir + "/CetraTelemetry.qml"));
  if (!component.isReady()) {
    qCritical() << component.errors();
    return 1;
  }

  std::unique_ptr<QObject> telemetry(component.create());
  if (!telemetry) {
    qCritical() << component.errors();
    return 1;
  }

  MockProcess *proc = telemetry->findChild<MockProcess*>("btProcess");
  if (!proc) {
    proc = MockProcess::latest();
  }
  assert(proc != nullptr);

  QObject *btPoll = telemetry->findChild<QObject*>("btPoll");
  QObject *btWatchdog = telemetry->findChild<QObject*>("btWatchdog");
  QObject *btKill = telemetry->findChild<QObject*>("btKill");
  QObject *btCooldown = telemetry->findChild<QObject*>("btCooldown");
  QObject *btFreshness = telemetry->findChild<QObject*>("btFreshness");
  assert(btPoll != nullptr);
  assert(btWatchdog != nullptr);
  assert(btKill != nullptr);
  assert(btCooldown != nullptr);
  assert(btFreshness != nullptr);

  // Initial inactive state
  assert(proc->launchCount() == 0);
  assert(telemetry->property("state").toString() == "idle");
  assert(telemetry->property("busy").toBool() == false);

  // 1. Activate + address coalesced: exactly one launch after processEvents
  telemetry->setProperty("active", true);
  telemetry->setProperty("address", "AA:BB:CC:DD:EE:FF");
  telemetry->setProperty("deviceGeneration", 1);
  assert(proc->launchCount() == 0);
  QCoreApplication::processEvents();
  assert(proc->launchCount() == 1);
  assert(proc->isRunning() == true);
  assert(telemetry->property("busy").toBool() == true);
  assert(telemetry->property("state").toString() == "loading");

  // 2. Stage good JSON not accepted until finished 0
  QString goodJson = QStringLiteral("{\"status\":\"ok\",\"address\":\"AA:BB:CC:DD:EE:FF\",\"left\":89,\"right\":91,\"case\":100,\"mode\":\"off\",\"left_charging\":false,\"right_charging\":true,\"case_charging\":true}");
  proc->feedStdout(goodJson);
  QCoreApplication::processEvents();
  assert(snapshotValue(telemetry.get()).isNull());
  assert(telemetry->property("state").toString() == "loading");

  proc->finish(0);
  QCoreApplication::processEvents();
  assert(telemetry->property("state").toString() == "ready");
  assert(telemetry->property("busy").toBool() == false);
  assert(telemetry->property("canRefresh").toBool() == false); // 2-second success cooldown
  QVariant snap = snapshotValue(telemetry.get());
  assert(!snap.isNull() && snap.isValid());
  assert(snap.toMap().value("right").toInt() == 91);

  // 3. right 91 -> null -> 93 across manual refresh (cooldown stop by QObject invoke stop) and all nullable
  QMetaObject::invokeMethod(btCooldown, "stop");
  QMetaObject::invokeMethod(telemetry.get(), "refresh");
  QCoreApplication::processEvents();
  assert(proc->launchCount() == 2);
  assert(proc->isRunning() == true);

  QString nullJson = QStringLiteral("{\"status\":\"ok\",\"address\":\"AA:BB:CC:DD:EE:FF\",\"left\":null,\"right\":null,\"case\":null,\"mode\":\"unknown\",\"left_charging\":null,\"right_charging\":null,\"case_charging\":null}");
  proc->finish(0, nullJson);
  QCoreApplication::processEvents();
  snap = snapshotValue(telemetry.get());
  assert(!snap.isNull());
  {
    QVariantMap m = snap.toMap();
    assert(m.value("right").isNull());
    assert(m.value("left").isNull());
    assert(m.value("case").isNull());
    assert(m.value("left_charging").isNull());
    assert(m.value("right_charging").isNull());
    assert(m.value("case_charging").isNull());
  }

  QMetaObject::invokeMethod(btCooldown, "stop");
  QMetaObject::invokeMethod(telemetry.get(), "refresh");
  QCoreApplication::processEvents();
  assert(proc->launchCount() == 3);
  assert(proc->isRunning() == true);

  QString json93 = QStringLiteral("{\"status\":\"ok\",\"address\":\"AA:BB:CC:DD:EE:FF\",\"left\":80,\"right\":93,\"case\":50,\"mode\":\"anc\",\"left_charging\":true,\"right_charging\":false,\"case_charging\":false}");
  proc->finish(0, json93);
  QCoreApplication::processEvents();
  snap = snapshotValue(telemetry.get());
  assert(snap.toMap().value("right").toInt() == 93);

  // 4. USB equivalent active=false immediate clear + TERM old, then no new launch until finished
  QMetaObject::invokeMethod(btCooldown, "stop");
  QMetaObject::invokeMethod(telemetry.get(), "refresh");
  QCoreApplication::processEvents();
  assert(proc->launchCount() == 4);
  assert(proc->isRunning() == true);
  proc->clearSignals();

  telemetry->setProperty("active", false);
  QCoreApplication::processEvents();
  assert(snapshotValue(telemetry.get()).isNull());
  assert(proc->receivedSignals().contains(15));
  assert(proc->launchCount() == 4);

  proc->finish(0);
  QCoreApplication::processEvents();
  assert(proc->launchCount() == 4);

  // 5. New address old stdout rejected and new launch after real exit
  telemetry->setProperty("active", true);
  telemetry->setProperty("address", "11:22:33:44:55:66");
  QCoreApplication::processEvents();
  assert(proc->launchCount() == 5);
  assert(proc->isRunning() == true);
  proc->clearSignals();

  telemetry->setProperty("address", "77:88:99:AA:BB:CC");
  QCoreApplication::processEvents();
  assert(proc->receivedSignals().contains(15));
  assert(proc->launchCount() == 5);

  proc->finish(0, QStringLiteral("{\"status\":\"ok\",\"address\":\"11:22:33:44:55:66\",\"left\":11,\"right\":11,\"case\":11,\"mode\":\"off\",\"left_charging\":false,\"right_charging\":false,\"case_charging\":false}"));
  QCoreApplication::processEvents();
  snap = snapshotValue(telemetry.get());
  if (!snap.isNull() && snap.isValid()) {
    assert(snap.toMap().value("address").toString() != "11:22:33:44:55:66");
  }
  assert(proc->launchCount() == 6);
  assert(proc->isRunning() == true);

  proc->finish(0, QStringLiteral("{\"status\":\"ok\",\"address\":\"77:88:99:AA:BB:CC\",\"left\":77,\"right\":77,\"case\":77,\"mode\":\"off\",\"left_charging\":false,\"right_charging\":false,\"case_charging\":false}"));
  QCoreApplication::processEvents();
  snap = snapshotValue(telemetry.get());
  assert(!snap.isNull());
  assert(snap.toMap().value("address").toString() == "77:88:99:AA:BB:CC");
  assert(snap.toMap().value("left").toInt() == 77);

  // 6. Same address generation reconnect
  int gen = telemetry->property("deviceGeneration").toInt();
  telemetry->setProperty("deviceGeneration", gen + 1);
  QCoreApplication::processEvents();
  assert(proc->launchCount() == 7);
  assert(proc->isRunning() == true);
  proc->finish(0, QStringLiteral("{\"status\":\"ok\",\"address\":\"77:88:99:AA:BB:CC\",\"left\":78,\"right\":78,\"case\":78,\"mode\":\"off\",\"left_charging\":false,\"right_charging\":false,\"case_charging\":false}"));
  QCoreApplication::processEvents();
  snap = snapshotValue(telemetry.get());
  assert(snap.toMap().value("left").toInt() == 78);

  // 7. Duplicate/malformed + valid reject
  QMetaObject::invokeMethod(btCooldown, "stop");
  QMetaObject::invokeMethod(telemetry.get(), "refresh");
  QCoreApplication::processEvents();
  assert(proc->launchCount() == 8);
  proc->feedStdout("INVALID_MALFORMED_JSON_FRAME{{");
  proc->finish(0);
  QCoreApplication::processEvents();
  assert(snapshotValue(telemetry.get()).isNull());
  assert(telemetry->property("state").toString() == "unavailable");

  // 8. Nonzero exit reject
  QMetaObject::invokeMethod(btCooldown, "stop");
  QMetaObject::invokeMethod(telemetry.get(), "refresh");
  QCoreApplication::processEvents();
  assert(proc->launchCount() == 9);
  proc->finish(1, QStringLiteral("{\"status\":\"ok\",\"address\":\"77:88:99:AA:BB:CC\",\"left\":80,\"right\":80,\"case\":80,\"mode\":\"off\",\"left_charging\":false,\"right_charging\":false,\"case_charging\":false}"));
  QCoreApplication::processEvents();
  assert(snapshotValue(telemetry.get()).isNull());
  assert(telemetry->property("state").toString() == "unavailable");

  // 9. Failed start runningChanged without exited -> unavailable + cooldown, panel toggle must not retry
  int curGen = telemetry->property("deviceGeneration").toInt();
  telemetry->setProperty("deviceGeneration", curGen + 1);
  QCoreApplication::processEvents();
  assert(proc->launchCount() == 10);
  assert(proc->isRunning() == true);

  proc->failStart();
  QCoreApplication::processEvents();
  assert(telemetry->property("state").toString() == "unavailable");
  assert(snapshotValue(telemetry.get()).isNull());
  assert(telemetry->property("canRefresh").toBool() == false);

  QMetaObject::invokeMethod(btCooldown, "stop");
  QCoreApplication::processEvents();

  int launchBeforeToggle = proc->launchCount();
  bool p = telemetry->property("panelOpen").toBool();
  telemetry->setProperty("panelOpen", !p);
  QCoreApplication::processEvents();
  telemetry->setProperty("panelOpen", p);
  QCoreApplication::processEvents();
  assert(proc->launchCount() == launchBeforeToggle);

  // 10. WatchdogTimer via invokeMethod triggered -> TERM then killtimer -> 9 then exit
  telemetry->setProperty("deviceGeneration", telemetry->property("deviceGeneration").toInt() + 1);
  QCoreApplication::processEvents();
  assert(proc->launchCount() == 11);
  assert(proc->isRunning() == true);
  proc->clearSignals();

  QMetaObject::invokeMethod(btWatchdog, "triggered");
  QCoreApplication::processEvents();
  assert(proc->receivedSignals().contains(15));

  QMetaObject::invokeMethod(btKill, "triggered");
  QCoreApplication::processEvents();
  assert(proc->receivedSignals().contains(9));

  proc->finish(137);
  QCoreApplication::processEvents();
  assert(telemetry->property("state").toString() == "unavailable");

  // 11. A spurious watchdog after completion must not launch another request
  QMetaObject::invokeMethod(btWatchdog, "triggered");
  QCoreApplication::processEvents();

  // 12. Snapshot expiry via adjust snapshotAcceptedAt if property exists else wait not acceptable
  QMetaObject::invokeMethod(btCooldown, "stop");
  telemetry->setProperty("deviceGeneration", telemetry->property("deviceGeneration").toInt() + 1);
  QCoreApplication::processEvents();
  proc->finish(0, QStringLiteral("{\"status\":\"ok\",\"address\":\"77:88:99:AA:BB:CC\",\"left\":55,\"right\":55,\"case\":55,\"mode\":\"off\",\"left_charging\":false,\"right_charging\":false,\"case_charging\":false}"));
  QCoreApplication::processEvents();
  assert(telemetry->property("state").toString() == "ready");

  assert(telemetry->property("snapshotAcceptedAt").isValid());
  qint64 pastAccepted = QDateTime::currentMSecsSinceEpoch() - 190000;
  if (telemetry->property("snapshotAcceptedAt").isValid()) {
    telemetry->setProperty("snapshotAcceptedAt", pastAccepted);
    QMetaObject::invokeMethod(btFreshness, "triggered");
    QCoreApplication::processEvents();
    assert(snapshotValue(telemetry.get()).isNull());
    assert(telemetry->property("state").toString() == "stale");
  } else if (telemetry->property("_snapshotAcceptedAt").isValid()) {
    telemetry->setProperty("_snapshotAcceptedAt", pastAccepted);
    QMetaObject::invokeMethod(btFreshness, "triggered");
    QCoreApplication::processEvents();
    assert(snapshotValue(telemetry.get()).isNull());
    assert(telemetry->property("state").toString() == "stale");
  }

  // 13. Clock rollback via future acceptedAt
  QMetaObject::invokeMethod(btCooldown, "stop");
  telemetry->setProperty("deviceGeneration", telemetry->property("deviceGeneration").toInt() + 1);
  QCoreApplication::processEvents();
  proc->finish(0, QStringLiteral("{\"status\":\"ok\",\"address\":\"77:88:99:AA:BB:CC\",\"left\":55,\"right\":55,\"case\":55,\"mode\":\"off\",\"left_charging\":false,\"right_charging\":false,\"case_charging\":false}"));
  QCoreApplication::processEvents();
  assert(telemetry->property("state").toString() == "ready");

  qint64 futureAccepted = QDateTime::currentMSecsSinceEpoch() + 60000;
  if (telemetry->property("snapshotAcceptedAt").isValid()) {
    telemetry->setProperty("snapshotAcceptedAt", futureAccepted);
    QMetaObject::invokeMethod(btFreshness, "triggered");
    QCoreApplication::processEvents();
    assert(snapshotValue(telemetry.get()).isNull());
    assert(telemetry->property("state").toString() == "stale");
  } else if (telemetry->property("_snapshotAcceptedAt").isValid()) {
    telemetry->setProperty("_snapshotAcceptedAt", futureAccepted);
    QMetaObject::invokeMethod(btFreshness, "triggered");
    QCoreApplication::processEvents();
    assert(snapshotValue(telemetry.get()).isNull());
    assert(telemetry->property("state").toString() == "stale");
  }

  // 15. Check no new helper after component destroy
  int instancesBeforeDestroy = MockProcess::totalCreated();
  telemetry.reset();
  QCoreApplication::processEvents();
  assert(MockProcess::totalCreated() == instancesBeforeDestroy);
  assert(MockProcess::liveInstances() == 0);

  // 16. Fail on unexpected warnings
  if (warnings > 0) {
    std::cerr << "FAILED: " << warnings << " unexpected Qt warnings\n";
    return 1;
  }

  qInfo("PASS Qt BluetoothTelemetry: coalesced launch, JSON staging, nullability, USB priority, address change, reconnect, validation, watchdog term/kill, expiry, clock rollback, teardown");
  return 0;
}

#include "qt.moc"
