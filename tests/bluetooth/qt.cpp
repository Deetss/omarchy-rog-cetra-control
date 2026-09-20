// Instantiate complete production observers with event-driven host model fixtures.
// No Quickshell session, PipeWire client, Bluetooth operation or HID reader.
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QDebug>
#include <QQuickItem>
#include <QTimer>
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
  QQmlComponent component(&engine, QUrl::fromLocalFile(QString::fromLocal8Bit(argv[2])));
  std::unique_ptr<QObject> fixture(component.create());
  if (!fixture) { qCritical() << component.errors(); return 1; }
  QVariant result;
  if (!QMetaObject::invokeMethod(fixture.get(), "run", Q_RETURN_ARG(QVariant, result)) || !result.toBool()) return 1;
  QCoreApplication::processEvents();
  qInfo("PASS Qt Bluetooth: complete production observers, reactive models, property events, removal/reconnect, USB gate and teardown");
  return warnings ? 1 : 0;
}
