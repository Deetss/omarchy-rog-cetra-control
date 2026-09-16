import QtQuick
import Quickshell.Services.Pipewire

// One service-owned observer. Traversal follows actual links, never target hints.
Item {
  id: topology
  required property bool active
  property bool usbAvailable: true
  property string bluetoothAddress: ""
  readonly property var nodes: active ? Pipewire.nodes.values : []
  readonly property var links: active ? Pipewire.links.values : []
  readonly property bool withinBudget: nodes.length <= 512 && links.length <= 2048
  readonly property var source: selectSource(nodes)
  readonly property var observation: active && usbAvailable && Pipewire.ready && withinBudget
    ? observe(source, nodes, links) : ({ capture: "unknown", communication: "unknown" })
  readonly property var audio: active && Pipewire.ready && withinBudget
    ? audioState(nodes, links, bluetoothAddress) : ({ output: "unknown", capture: "unknown", profile: "unknown", sink: false, microphone: false })
  PwObjectTracker {
    objects: topology.active && topology.withinBudget ? topology.nodes.concat(topology.links) : []
  }

  function selectSource(values) {
    var matches = values.filter(function (node) {
      return node && node.isStream === false && node.isSink === false
        && (node.type & PwNodeType.AudioSource) === PwNodeType.AudioSource
        && /^alsa_input\.usb-ASUSTek_ROG_CETRA_TRUE_WIRELESS_SPEEDNOVA_[^.]+\./.test(node.name)
    })
    return matches.length === 1 ? matches[0] : null
  }

  function identity(node) {
    var p = node.properties || {}
    return [node.name, p["application.name"], p["application.id"], p["application.process.binary"],
      p["pipewire.access.portal.app_id"], p["media.name"], p["media.filename"]].join(" ")
  }

  function isMonitor(node) {
    if (!node) return true
    var p = node.properties || {}
    return p["application.id"] === "io.github.pavellizunov.rog-cetra-control.peak"
      || /quickshell[ _-]peak|peak detect/i.test(identity(node))
      || p["media.category"] === "Monitor" || p["stream.monitor"] === true || p["stream.monitor"] === "true"
  }

  function isEndpoint(node) {
    var p = node.properties || {}
    return node.isStream === true && node.isSink === false && !isMonitor(node)
      && !/easy[ _-]?effects|keepalive|\/dev\/null|voxtype|recognition/i.test(identity(node))
      && !/^(DSP|Filter)$/i.test(p["media.role"] || "")
      && p["pulse.corked"] !== true && p["pulse.corked"] !== "true"
  }

  function isCommunication(node) {
    var p = node.properties || {}
    var role = p["media.role"] || ""
    // Explicit non-call roles take precedence over application-name heuristics.
    if (role) return /^(phone|communication)$/i.test(role)
    return /(^|[^a-z0-9_])(webrtc|discord|vesktop|steam(webhelper)?|telegram|zoom)([^a-z0-9_]|$)/i.test(identity(node))
  }

  function observe(mic, values, edges) {
    if (!mic || values.length > 512 || edges.length > 2048)
      return { capture: "unknown", communication: "unknown" }
    var incoming = new Map()
    var uncertain = !mic.ready
    for (var i = 0; i < edges.length; i++) {
      var edge = edges[i]
      if (!edge.source || !edge.target) { uncertain = true; continue }
      if (edge.state !== PwLinkState.Active) continue
      if (!incoming.has(edge.target)) incoming.set(edge.target, [])
      incoming.get(edge.target).push(edge.source)
    }
    var capture = false, communication = false
    for (var n = 0; n < values.length; n++) {
      var endpoint = values[n]
      if (!endpoint.ready) { uncertain = true; continue }
      if (!isEndpoint(endpoint)) continue
      var queue = [endpoint], visited = new Set([endpoint]), reachesMic = false, otherInput = false, incomplete = false
      for (var pos = 0; pos < queue.length; pos++) {
        var node = queue[pos]
        if (visited.size > 512) { incomplete = true; break }
        if (!node.ready) { incomplete = true; continue }
        if (node === mic) { reachesMic = true; continue }
        // A second physical source makes processed audio attribution ambiguous.
        if (!node.isStream && /^(alsa_input|bluez_input)\./.test(node.name)) { otherInput = true; continue }
        var parents = incoming.get(node) || []
        for (var j = 0; j < parents.length; j++)
          if (!visited.has(parents[j])) { visited.add(parents[j]); queue.push(parents[j]) }
      }
      if (reachesMic && !otherInput && !incomplete) {
        capture = true
        if (isCommunication(endpoint)) communication = true
      } else if (reachesMic || incomplete) uncertain = true
    }
    return { capture: capture ? "active" : uncertain ? "unknown" : "inactive",
      communication: communication ? "active" : uncertain ? "unknown" : "inactive" }
  }

  function bluetoothNodeAddress(node, values) {
    var p = node.properties || {}, direct = p["api.bluez5.address"]
    if (p["device.api"] === "bluez5" && typeof direct === "string" && /^(?:[0-9a-f]{2}:){5}[0-9a-f]{2}$/i.test(direct)) return direct.toUpperCase()
    // PipeWire's public HFP loopback source has no address property. Its device.id
    // relates it to the internal BlueZ source/sink; names alone do not identify it.
    if (p["bluez5.loopback"] !== true && p["bluez5.loopback"] !== "true") return ""
    if (p["device.id"] === undefined || p["device.id"] === null) return ""
    var matches = new Set()
    for (var i = 0; i < values.length; i++) {
      var other = values[i], info = other.properties || {}, candidate = info["api.bluez5.address"]
      if (other.ready && info["device.api"] === "bluez5" && String(info["device.id"]) === String(p["device.id"])
          && typeof candidate === "string" && /^(?:[0-9a-f]{2}:){5}[0-9a-f]{2}$/i.test(candidate)) matches.add(candidate.toUpperCase())
    }
    return matches.size === 1 ? Array.from(matches)[0] : ""
  }

  function transport(node, address, values) {
    var p = node.properties || {}
    if (node.isStream) return ""
    if (/^alsa_(input|output)\.usb-ASUSTek_ROG_CETRA_TRUE_WIRELESS_SPEEDNOVA_[^.]+\./.test(node.name)) return "usb"
    if (p["device.api"] === "bluez5" || /^bluez_(input|output)\./.test(node.name)) {
      var nativeAddress = bluetoothNodeAddress(node, values || [])
      return !nativeAddress ? "unknown" : address && nativeAddress === address.toUpperCase() ? "bluetooth" : "other"
    }
    return /^alsa_(input|output)\./.test(node.name) || p["device.api"] === "alsa" ? "other" : ""
  }

  function route(values, edges, address, playback) {
    if (values.length > 512 || edges.length > 2048) return "unknown"
    var adjacency = new Map(), uncertain = false, routes = new Set()
    for (var i = 0; i < edges.length; i++) {
      var edge = edges[i]
      if (!edge.source || !edge.target) { uncertain = true; continue }
      if (edge.state !== PwLinkState.Active) continue
      var from = playback ? edge.source : edge.target, to = playback ? edge.target : edge.source
      if (!adjacency.has(from)) adjacency.set(from, [])
      adjacency.get(from).push(to)
    }
    for (var n = 0; n < values.length; n++) {
      var endpoint = values[n], p = endpoint.properties || {}
      if (!endpoint.ready) { uncertain = true; continue }
      if (!endpoint.isStream || endpoint.isSink !== playback || isMonitor(endpoint) || /\/Internal$/.test(p["media.class"] || "")
          || /easy[ _-]?effects/i.test(identity(endpoint)) || /^(DSP|Filter)$/i.test(p["media.role"] || "")
          || p["pulse.corked"] === true || p["pulse.corked"] === "true" || !adjacency.has(endpoint)) continue
      var queue = [endpoint], visited = new Set([endpoint]), found = false
      for (var pos = 0; pos < queue.length; pos++) {
        var node = queue[pos]
        if (!node.ready) { uncertain = true; continue }
        var kind = transport(node, address, values)
        if (kind === "unknown") { uncertain = true; continue }
        if (kind) { routes.add(kind); found = true; continue }
        var next = adjacency.get(node) || []
        if (!next.length) uncertain = true
        for (var j = 0; j < next.length; j++) {
          if (!visited.has(next[j])) { visited.add(next[j]); queue.push(next[j]) }
        }
        if (visited.size > 512) { uncertain = true; break }
      }
      if (!found) uncertain = true
    }
    if (uncertain) return "unknown"
    return routes.size > 1 ? "mixed" : routes.size === 1 ? Array.from(routes)[0] : "inactive"
  }

  function audioState(values, edges, address) {
    var profiles = new Set(), sink = false, microphone = false
    for (var i = 0; i < values.length; i++) {
      var node = values[i], p = node.properties || {}
      if (!node.ready || transport(node, address, values) !== "bluetooth") continue
      if (p["media.class"] === "Audio/Sink") sink = true
      if (p["media.class"] === "Audio/Source") microphone = true
      var profile = p["api.bluez5.profile"] || ""
      if (/^a2dp-/.test(profile)) profiles.add("a2dp")
      else if (/^(headset-head-unit|hfp-hf|hsp-hs|headset-audio-gateway|hfp-ag|hsp-ag)/.test(profile)) profiles.add("hfp")
    }
    return { output: route(values, edges, address, true), capture: route(values, edges, address, false),
      profile: profiles.size === 1 ? Array.from(profiles)[0] : "unknown", sink: sink, microphone: microphone }
  }

}
