#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTimer>
#include <QVariant>
#include <memory>
#include <iostream>
static int warnings = 0;
static void messages(QtMsgType type, const QMessageLogContext &, const QString &message) {
  std::cerr << message.toStdString() << "\n";
  if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) ++warnings;
}
int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  qInstallMessageHandler(messages);
  QQmlEngine engine;
  engine.addImportPath(QString::fromLocal8Bit(argv[1]));
  QQmlComponent component(&engine, QUrl::fromLocalFile(QString::fromLocal8Bit(argv[1])+"/Fixture.qml"));
  std::unique_ptr<QObject> fixture(component.create());
  if (!fixture) { std::cerr << component.errorString().toStdString(); return 1; }
  if (!QMetaObject::invokeMethod(fixture.get(), "run")) return 2;
  QTimer::singleShot(6800,[&]{
    const bool pass=fixture->property("done").toBool() && !warnings;
    std::cout << (pass?"PASS":"FAIL") << " Qt meter: immediate admission, cooldown, source replacement, zero, freshness, stop; fake process only\n";
    app.exit(pass?0:3);
  });
  return app.exec();
}
