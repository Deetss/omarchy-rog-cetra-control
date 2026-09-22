// Exercise real QColor/variant argument passing without loading the plugin or a helper.
#include <QFile>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickItem>
#include <QQuickWindow>
#include <QImage>
#include <QKeyEvent>
#include <iostream>
#include <memory>

static QString functions(const QString &file) {
  QFile source(file);
  if (!source.open(QIODevice::ReadOnly)) qFatal("Cannot open production source");
  const QString text = QString::fromUtf8(source.readAll());
  auto matches = QRegularExpression(R"(^  function \w+\([^)]*\) \{[\s\S]*?^  \})",
    QRegularExpression::MultilineOption).globalMatch(text);
  QString result;
  while (matches.hasNext()) result += matches.next().captured() + "\n";
  return result;
}

static QString extract(const QString &file, const QString &pattern) {
  QFile source(file);
  if (!source.open(QIODevice::ReadOnly)) qFatal("Cannot open production source");
  auto match = QRegularExpression(pattern, QRegularExpression::MultilineOption)
                 .match(QString::fromUtf8(source.readAll()));
  if (!match.hasMatch()) qFatal("Cannot extract production binding/function");
  return match.captured();
}

int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);
  if (argc != 2) return 2;
  const QString dir = QString::fromLocal8Bit(argv[1]);
  const QString widget = dir + "/CetraViewModel.qml";
  const QString service = dir + "/CetraService.qml";
  QString qml = R"QML(import QtQuick
Item {
  id: root
  property color accent: Qt.rgba(0.2, 0.4, 0.6, 1)
  property bool useThemeColor: true
  property string lightingFeedback: ""
  property var lightingRgb: [17, 34, 51]
)QML";
  qml += extract(widget, R"(^  readonly property var selectedLightingColor: \{\n[\s\S]*?^  \})");
  qml += R"QML(
  property QtObject service: QtObject {
    property bool connected: true
    property string lighting: "unknown"
    property string sessionLightingEffect: ""
    property string lastLightingPayload: ""
    property QtObject themeColorDelay: QtObject { function stop() {} }
    property QtObject deviceWatchProc: QtObject {
      property bool running: true
      property var writes: []
      function write(text) { writes = writes.concat([text]) }
    }
)QML";
  qml += extract(service, R"(^  function setLighting\([^)]*\) \{[\s\S]*?^  \})");
  qml += "\n  }\n";
  qml += extract(widget, R"(^  function setLighting\([^)]*\) \{[\s\S]*?^  \})");
  qml += R"QML(
  function check(ok, reason) { if (!ok) throw new Error(reason) }
  function run() {
    check(typeof selectedLightingColor.r === "number", "QColor r is not numeric")
    check(typeof selectedLightingColor.g === "number", "QColor g is not numeric")
    check(typeof selectedLightingColor.b === "number", "QColor b is not numeric")
    for (var effect of ["static", "breathing", "strobing"]) {
      check(setLighting(effect) === true, "QColor rejected")
      check(service.deviceWatchProc.writes.slice(-1)[0] === "lighting " + effect + " 51 102 153\n", "QColor payload")
    }
    useThemeColor = false
    for (var channel = 0; channel <= 255; channel++) {
      lightingRgb = [channel, 255 - channel, channel]
      check(setLighting("static") === true, "Manual RGB rejected")
      check(service.deviceWatchProc.writes.slice(-1)[0] === "lighting static " + channel + " " + (255 - channel) + " " + channel + "\n", "Manual RGB payload")
    }
    lightingRgb = [null, 34, 51]
    check(setLighting("static") === false, "Invalid manual RGB accepted")
    useThemeColor = true
    accent = Qt.rgba(0, 0.5, 1, 1)
    check(setLighting("static") === true, "Changed QColor rejected")
    check(service.deviceWatchProc.writes.slice(-1)[0] === "lighting static 0 128 255\n", "Changed QColor payload")
    check(service.lighting === "unknown", "Optimistic hardware state")
    return true
  }
})QML";
  QQmlEngine engine;
  QQmlComponent component(&engine);
  component.setData(qml.toUtf8(), QUrl("file:///tmp/cetra-offline-color.qml"));
  std::unique_ptr<QObject> object(component.create());
  if (!object) { qCritical() << component.errors(); return 1; }
  QVariant result;
  if (!QMetaObject::invokeMethod(object.get(), "run", Q_RETURN_ARG(QVariant, result))
      || !result.toBool()) return 1;

  // Keep the host's derived-binding/signal ordering real: this reproduces a
  // lagging public snapshot in Qt, unlike the synchronous Node host fixtures.
  QString settingsQml = R"QML(import QtQuick
Item {
  id: test
  property var shellConfig: ({version: 1, bar: {layout: {right: [{id: "cetra", locale: "system", alwaysCallContext: false}]}}})
  readonly property var barConfig: shellConfig.bar
  property bool reject: false
  onShellConfigChanged: {
    api.barConfig = JSON.parse(JSON.stringify(barConfig))
    a.settings = barConfig.layout.right[0]
    b.settings = barConfig.layout.right[0]
  }
  QtObject {
    id: api
    property var barConfig: ({})
    function updateEntryInline(id, entry) {
      if (test.reject) return false
      test.shellConfig = {version: 1, bar: {layout: {right: [entry]}}}
      return true
    }
  }
  Item {
    id: owner
    property var shell: api
    property var manifest: ({id: "cetra"})
    property bool hostReady: true
    property var inlineSettings: null
    property var pendingPreferences: ({})
    property QtObject preferenceReadback: QtObject {
      function restart() {}
      function stop() {}
    }
)QML";
  for (const QString &name : {QString("settings"), QString("hostSettings")})
    settingsQml += extract(dir + "/CetraPreferences.qml", "^  readonly property var " + name + R"(: (?:\{\n[\s\S]*?^  \}|[^\n]+))") + "\n";
  settingsQml += functions(dir + "/CetraPreferences.qml") + functions(service) + "\n}\n";
  settingsQml += R"QML(
  component View: Item {
    id: root
    property var service: owner
    property var settings: ({})
    property string moduleName: "cetra"
    property var bar: ({shell: api})
)QML";
  settingsQml += functions(widget);
  settingsQml += R"QML(
  }
  View { id: a }
  View { id: b }
  function check(ok, reason) { if (!ok) throw new Error(reason) }
  function run() {
    api.barConfig = JSON.parse(JSON.stringify(barConfig))
    a.settings = barConfig.layout.right[0]
    b.settings = a.settings
    owner.applySavedConfig(JSON.stringify(shellConfig))
    a.setLocaleSetting("ru")
    check(api.barConfig.layout.right[0].locale === "system", "Must reproduce lagging host snapshot")
    b.setLightingSetting("useThemeColor", false)
    a.setLightingSetting("lightingRed", 17)
    check(barConfig.layout.right[0].locale === "ru", "Locale was rolled back")
    check(!b.preference("useThemeColor", true), "Color preference was rolled back")
    check(a.preference("lightingRed", 255) === 17, "RGB not shared")
    return true
  }
  function afterInjection() {
    owner.applySavedConfig(JSON.stringify({version: 1, bar: api.barConfig}))
    check(owner.settings.locale === "ru" && owner.settings.lightingRed === 17, "Deferred injection rolled back settings")
    owner.applySavedConfig(JSON.stringify(shellConfig))
    check(Object.keys(owner.pendingPreferences).length === 0, "Saved preferences not acknowledged")
    test.reject = true
    check(a.setLocaleSetting("de") === false, "Rejected write reported success")
    check(b.preference("locale", "system") === "ru", "Failed write changed selection")
    test.reject = false
    // An external host edit must supersede the plugin's shared preferences.
    shellConfig = {version: 1, bar: {layout: {right: [{id: "cetra", locale: "fr", alwaysCallContext: false}]}}}
    owner.applySavedConfig(JSON.stringify(shellConfig))
    return true
  }
  function afterExternalEdit() {
    check(a.preference("locale", "system") === "fr", "External locale not propagated")
    check(b.preference("useThemeColor", true), "External preference removal not propagated")
    check(a.preference("lightingRed", 255) === 255, "Removed preference survived external edit")
    return true
  }
  function selectLanguage(code) {
    a.setLocaleSetting(code)
    return true
  }
  function verifyLanguage(code) {
    check(a.preference("locale", "system") === code && b.preference("locale", "system") === code, "Language reverted after event delivery")
    owner.applySavedConfig(JSON.stringify({version: 1, bar: api.barConfig}))
    check(a.preference("locale", "system") === code, "Old disk snapshot reverted selection")
    owner.applySavedConfig(JSON.stringify(shellConfig))
    return true
  }
})QML";
  QQmlComponent settingsComponent(&engine);
  settingsComponent.setData(settingsQml.toUtf8(), QUrl("file:///tmp/cetra-offline-settings.qml"));
  std::unique_ptr<QObject> settingsObject(settingsComponent.create());
  if (!settingsObject) { qCritical() << settingsComponent.errors(); return 1; }
  for (const auto *step : {"run", "afterInjection", "afterExternalEdit"}) {
    if (!QMetaObject::invokeMethod(settingsObject.get(), step, Q_RETURN_ARG(QVariant, result)) || !result.toBool()) {
      std::cerr << "Settings check failed: " << step << '\n';
      return 1;
    }
    QCoreApplication::processEvents();
  }
  for (const auto *code : {"ru", "de", "en", "ja", "system", "ru"}) {
    const QVariant language = QString::fromLatin1(code);
    for (const auto *step : {"selectLanguage", "verifyLanguage"}) {
      if (!QMetaObject::invokeMethod(settingsObject.get(), step, Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, language)) || !result.toBool()) {
        std::cerr << "Language check failed: " << step << ' ' << code << '\n';
        return 1;
      }
      QCoreApplication::processEvents();
    }
  }

  // Load the complete production palette; only host tokens/model are substituted.
  QFile paletteFile(dir + "/LightingColorField.qml");
  if (!paletteFile.open(QIODevice::ReadOnly)) return 1;
  QString paletteQml = QString::fromUtf8(paletteFile.readAll());
  paletteQml.replace("import qs.Commons", "");
  paletteQml.replace("Style.", "style.");
  paletteQml.replace("required property var root", R"QML(
  width: 360
  property QtObject style: QtObject {
    function space(value) { return value }
    property QtObject font: QtObject { property int caption: 12 }
  }
  property QtObject root: QtObject {
    property var lightingDraftRgb: [51, 102, 153]
    property bool lightingColorExpanded: true
    property bool connected: true
    property color accent: Qt.rgba(0.5,0.7,0.9,1)
    property color dim: Qt.rgba(0.4,0.4,0.4,1)
    property color foreground: "white"
    property string fontFamily: "monospace"
    function tr(key, fallback) { return fallback }
    function moveFocus(direction, wrap) {}
    function focusControl(target) { target.forceActiveFocus() }
    function setLightingDraftChannel(index, value) {
      var rgb = lightingDraftRgb.slice(); rgb[index] = value; lightingDraftRgb = rgb
    }
  }
  function check(ok, why) { if (!ok) throw new Error(why) }
  function checkRgb(rgb) {
    for (var i = 0; i < 3; i++) check(Math.abs(root.lightingDraftRgb[i] - rgb[i]) <= 1, "RGB mismatch " + root.lightingDraftRgb)
  }
  function run() {
    resetFromDraft(false); publishDraft(); checkRgb([51,102,153])
    var r = wheel.width / 2
    choosePoint(r*2,r); checkRgb([153,0,0])
    setBrightness(1); checkRgb([255,0,0])
    hue = 1/3; saturation = 1; publishDraft(); checkRgb([0,255,0])
    setBrightness(0); checkRgb([0,0,0]); check(Math.abs(hue-1/3)<0.001, "Black erased hue")
    setBrightness(1); choosePoint(r,r); checkRgb([255,255,255]); check(Math.abs(hue-1/3)<0.001, "Grey erased hue")
    for (var text of ["#000000", "FFFFFF", "aB80cD"]) {
      check(setHex(text), "Valid HEX rejected")
      check(hexValue.toLowerCase() === ("#" + text.replace(/^#/, "")).toLowerCase(), "HEX roundtrip")
    }
    var saved = JSON.stringify(root.lightingDraftRgb)
    for (var bad of ["", "#", "123", "12345z", "1234567", "#12345678", "red", null]) {
      check(!setHex(bad), "Invalid HEX accepted")
      check(JSON.stringify(root.lightingDraftRgb) === saved, "Invalid HEX mutated draft")
    }
    setChannel(0,17); setChannel(1,34); setChannel(2,51); checkRgb([17,34,51])
    check(hexValue === "#112233", "RGB/HEX mismatch")
    root.connected = false; check(!setHex("ffffff"), "Offline HEX accepted"); setBrightness(1); checkRgb([17,34,51])
    root.connected = true; root.lightingColorExpanded = false
    check(!setHex("ffffff"), "Closed HEX accepted"); choosePoint(r*2,r); checkRgb([17,34,51])
    root.lightingColorExpanded = true
    setHex("#7F7F7F"); publishDraft(); checkRgb([127,127,127])
    return true
  }
)QML");
  QQmlComponent paletteComponent(&engine);
  paletteComponent.setData(paletteQml.toUtf8(), QUrl("file:///tmp/cetra-offline-palette.qml"));
  std::unique_ptr<QObject> paletteObject(paletteComponent.create());
  if (!paletteObject) { qCritical() << paletteComponent.errors(); return 1; }
  if (!QMetaObject::invokeMethod(paletteObject.get(), "run", Q_RETURN_ARG(QVariant, result)) || !result.toBool()) return 1;
  // Ancestor visibility must reinitialize the same production editor on reopening.
  QQuickWindow visibilityWindow;
  visibilityWindow.resize(360,174);
  QQuickItem hiddenParent(visibilityWindow.contentItem());
  visibilityWindow.show();
  QCoreApplication::processEvents();
  auto *paletteItem = qobject_cast<QQuickItem *>(paletteObject.get());
  paletteItem->setParentItem(&hiddenParent);
  hiddenParent.setVisible(false);
  auto *paletteModel = paletteObject->property("root").value<QObject *>();
  paletteModel->setProperty("lightingDraftRgb", QVariantList{255,0,255});
  hiddenParent.setVisible(true);
  if (qAbs(paletteObject->property("hue").toDouble() - 5.0/6.0) > 0.001) {
    std::cerr << "Palette reopen: hue=" << paletteObject->property("hue").toDouble() << " visible=" << paletteItem->isVisible() << " parent=" << hiddenParent.isVisible() << "\n"; return 1;
  }
  paletteItem->setParentItem(nullptr);
  if (!qEnvironmentVariableIsEmpty("CETRA_PALETTE_CAPTURE")) {
    QQuickWindow window;
    window.setColor(Qt::transparent);
    window.resize(360, 210);
    auto *item = qobject_cast<QQuickItem *>(paletteObject.get());
    item->setParentItem(window.contentItem());
    window.show();
    for (int i = 0; i < 20; ++i) QCoreApplication::processEvents();
    if (!window.grabWindow().save(qEnvironmentVariable("CETRA_PALETTE_CAPTURE"))) return 1;
    item->setParentItem(nullptr);
  }
  qInfo("PASS complete Qt palette: HSV/RGB, black/grey hue preservation, closed/USB draft guards");

  // Exercise the production HEX TextField handlers under a parent key catcher.
  QString inputQml = R"QML(import QtQuick
import QtQuick.Controls
Item {
  id: frame
  property int leaked: 0
  property int cancelled: 0
  property QtObject style: QtObject { property QtObject font: QtObject { property int bodySmall: 12 } }
  property QtObject root: QtObject {
    property string fontFamily: "monospace"
    property color foreground: "white"
    property color accent: "white"
    function tr(key, fallback) { return fallback }
    function moveFocus(direction, tab) {}
    function focusControl(target) { target.forceActiveFocus() }
    function cancelLightingEdit() { frame.cancelled++ }
  }
  property QtObject section: QtObject { property bool hexInvalid: false }
  property QtObject colorField: QtObject {
    property string hexValue: "#123456"
    function setHex(text) { return /^#?[0-9a-fA-F]{6}$/.test(text) }
  }
  Keys.onPressed: function(event) { leaked++; event.accepted = true }
  Item { id: chooseColorButton }
)QML";
  QString input = extract(dir + "/LightingPalette.qml", R"(^        TextField \{
[\s\S]*?^        \})");
  input.replace("Style.", "style.");
  input.replace("id: hexInput", "id: hexInput; objectName: \"hexInput\"; property color foreground; property color accent");
  inputQml += input + "\n}";
  QQmlComponent inputComponent(&engine);
  inputComponent.setData(inputQml.toUtf8(), QUrl("file:///tmp/cetra-offline-hex.qml"));
  std::unique_ptr<QObject> inputObject(inputComponent.create());
  if (!inputObject) { std::cerr << inputComponent.errorString().toStdString(); return 1; }
  QQuickWindow inputWindow;
  inputWindow.resize(360,100);
  auto *inputRoot = qobject_cast<QQuickItem *>(inputObject.get());
  inputRoot->setParentItem(inputWindow.contentItem()); inputRoot->setWidth(360);
  inputWindow.show(); QCoreApplication::processEvents();
  auto *hex = inputObject->findChild<QQuickItem *>("hexInput");
  hex->forceActiveFocus(); QMetaObject::invokeMethod(hex, "selectAll");
  for (const QChar c : QString("A3B5C7")) {
    QKeyEvent key(QEvent::KeyPress, c.unicode(), Qt::NoModifier, QString(c));
    QCoreApplication::sendEvent(&inputWindow, &key);
  }
  if (hex->property("text").toString() != "A3B5C7" || inputObject->property("leaked").toInt() != 0
      || inputObject->property("section").value<QObject *>()->property("hexInvalid").toBool()) {
    std::cerr << "HEX typing leaked to parent shortcuts or failed to edit\n"; return 1;
  }
  for (int i = 0; i < 3; i++) {
    QKeyEvent extra(QEvent::KeyPress, Qt::Key_F, Qt::NoModifier, "F");
    QCoreApplication::sendEvent(&inputWindow, &extra);
  }
  if (inputObject->property("leaked").toInt() != 0) return 1;
  QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
  QCoreApplication::sendEvent(&inputWindow, &escape);
  if (inputObject->property("cancelled").toInt() != 1 || inputObject->property("leaked").toInt() != 0) return 1;
  inputRoot->setParentItem(nullptr);
  std::cout << "PASS Qt HEX input: A/F/digits stay in text editor; Escape cancels draft\n";

  // Exercise the production wrapping Text and height binding in real QtQuick.
  // Palette tokens are supplied locally; no Quickshell host or HID is started.
  QString labelQml = R"QML(import QtQuick
Item {
  id: controlButton
  property string label: ""
  property real horizontalPadding: 9
  property real verticalPadding: 5
  property bool selected: false
  property bool leftAlign: false
  property color foreground: "transparent"
  property color accent: "transparent"
  property string fontFamily: "monospace"
  property real fontSize: 9
  property QtObject style: QtObject {
    property int normalBorderWidth: 1
    function selectedStateColor(foreground, accent) { return foreground }
  }
)QML";
  QString height = extract(dir + "/ControlButton.qml", R"(^  implicitHeight: buttonLabel[^\n]+)");
  QString label = extract(dir + "/ControlButton.qml", R"(^  Text \{\n    textFormat: Text.PlainText\n    id: buttonLabel[\s\S]*?^  \})");
  // Only replace the environment-owned singleton reference, never text/geometry.
  labelQml += height.replace("Style.", "style.") + "\n";
  labelQml += label.replace("Style.", "style.") + "\n}";
  QQmlComponent labelComponent(&engine);
  labelComponent.setData(labelQml.toUtf8(), QUrl("file:///tmp/cetra-offline-label.qml"));
  std::unique_ptr<QObject> labelObject(labelComponent.create());
  if (!labelObject) { qCritical() << labelComponent.errors(); return 1; }
  auto *button = qobject_cast<QQuickItem *>(labelObject.get());
  auto *text = button->childItems().at(0);
  QFile indexFile(dir + "/locales/index.json");
  if (!indexFile.open(QIODevice::ReadOnly)) return 1;
  const auto registry = QJsonDocument::fromJson(indexFile.readAll()).object();
  int labelsChecked = 0;
  for (auto it = registry.begin(); it != registry.end(); ++it) {
    QFile file(dir + "/locales/" + it.value().toObject()["file"].toString());
    if (!file.open(QIODevice::ReadOnly)) return 1;
    const auto catalog = QJsonDocument::fromJson(file.readAll()).object();
    for (const auto *key : {"voice.english", "voice.chinese", "voice.beeps", "lighting.static",
                           "lighting.breathing", "lighting.strobing", "language.system"}) {
      for (const int width : {69, 102}) for (const int fontSize : {9, 14, 20}) {
        button->setWidth(width);
        button->setProperty("fontSize", fontSize);
        button->setProperty("label", catalog[key].toString());
        QCoreApplication::processEvents();
        if (text->property("contentWidth").toDouble() > text->width() + 1
            || button->height() < text->implicitHeight() + 10) {
          std::cerr << "Label overflow: " << it.key().toStdString() << ' ' << key << '\n';
          return 1;
        }
        labelsChecked++;
      }
    }
  }
  std::cout << "PASS Qt settings: lagging host, two views, rejected write, external edit; "
            << labelsChecked << " production label layouts\n";
  qInfo("PASS Qt QColor/variant: numeric channels, theme changes, all manual bytes and exact mock payloads (no HID)");
  return 0;
}
