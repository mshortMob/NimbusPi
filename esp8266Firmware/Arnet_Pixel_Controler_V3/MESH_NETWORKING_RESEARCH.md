# Mesh Networking Feature — Implementation & Research Notes

This document records the painlessMesh-based "Mesh Fleet" networking mode that
existed in this codebase, why it was removed, and the investigation that led
to that decision. It's kept for reference in case mesh support is revisited.

## What it was

A second `network_mode` (EEPROM field, `0=Standalone`, `1=Mesh Fleet`)
alongside the original standalone/AP mode. In Mesh Fleet mode, devices formed
a [painlessMesh](https://github.com/gmag11/painlessMesh) network with each
other instead of each running its own independent Wi-Fi AP, with one device
optionally flagged `mesh_leader` (`mesh.setRoot(true)`) to speed up/stabilize
mesh formation. Settings changes and, later, per-node-targeted preset/pixel-map
changes were relayed across the mesh so a change made on one device's UI could
apply to some or all of the fleet.

### Node identity & the `nodeInfo` heartbeat

painlessMesh's `mesh.getNodeList()` only returns bare numeric node IDs, so a
small self-announcement protocol was layered on top:

- Each node broadcasts a `nodeInfo` message (`{"hostname":..., "isLeader":...}`)
  on join, on every new connection, and on a periodic ~20s heartbeat.
- Every node keeps an in-RAM `std::map<uint32_t, NodeInfo> knownNodes`,
  populated in `meshReceivedCallback` using the callback's own `from`
  parameter as the key.
- `GET /getMeshNodes` (mesh-mode only) returned the known fleet as
  `{"nodes":[{"nodeId":..., "hostname":..., "isLeader":..., "isSelf":...}]}`,
  which fed the frontend's "Nodes" page node list/selection UI.

### Message envelope & targeting

All mesh traffic used a plain-string JSON envelope (not nested
ArduinoJson documents — see "ArduinoJson v7 is fully dynamic" below):

```
{"type":"<settings|ledPresets|pixelMap|selectedMode|nodeInfo>","src":<nodeId>,"data":{...}}
```

Per-node targeting (the "Nodes" page letting a user push a preset change to
only some fleet members) worked by embedding an optional `"targetNodes":[...]`
array of node IDs inside `data`. The originating device always applied the
change locally only if it was itself in the target list (or the list was
empty, meaning "everyone"), then relayed to the mesh. The **receiving** side
self-filtered: `meshReceivedCallback` checked whether its own
`mesh.getNodeId()` appeared in `data.targetNodes` before applying anything.

## Crashes fixed this session (in order)

Mesh-mode crashes appeared after an unrelated LAN-mode refactor and were
root-caused, one at a time, via `xtensa-lx106-elf-addr2line` against captured
serial exception dumps:

1. **WebSocket reconnect storm.** A single failed WS handshake could fire
   `onerror`+`onclose` *and* a periodic health-check simultaneously, each
   independently scheduling a reconnect. These piled up into several
   concurrent connection attempts, overflowing `WebSocketsServer`'s
   `WEBSOCKETS_SERVER_CLIENT_MAX` (default 5) and crashing the device on the
   5th. Fixed with a single-flight reconnect guard (`scheduleWebSocketReconnect`)
   plus shrinking `WEBSOCKETS_SERVER_CLIENT_MAX` to 2 via a build flag (this
   flag is a general resource-saving change, unrelated to mesh, and was kept).
2. **ArduinoJson v7's fully-dynamic allocation model.** Confirmed via source
   read (`compatibility.hpp`) that in ArduinoJson v7, `StaticJsonDocument<N>`
   and `DynamicJsonDocument(capacity)` are deprecated shims — the size/capacity
   argument is vestigial and every element allocates dynamically from the
   heap regardless. `handleGetLedPresets`/`handleGetPixelMapPresets` built
   their responses via ArduinoJson under mesh mode's already-tighter heap
   headroom, exhausting it. Fixed by rewriting both handlers to build their
   JSON responses via plain `String` concatenation instead.
3. **Cumulative heap fragmentation from an uncompressed `index.html`.**
   Traced via `addr2line` to `AsyncWebServerRequest::_parseReqHeader()` /
   `String::concat()`. Fixed by gzip-compressing `index.html` and serving only
   the `.gz` sibling from the SPIFFS image (`ESPAsyncWebServer`'s gzip
   auto-detection only ever kicks in if the plain file isn't found first).
4. **The mesh `nodeInfo` heartbeat itself.** A recurring (every 20s, plus every
   new connection) small ArduinoJson allocation, worsening fragmentation over
   time on top of everything else contending for the same heap. Rewritten to
   plain string concatenation, matching the main envelope-building approach.
5. **`mesh.sendSingle()` silently failing.** Root-caused via direct
   painlessMesh source review (`router.hpp`): the `send()` overload
   `sendSingle()` uses depends on `findRoute()` finding the destination in the
   local node's routing tree (`layout.subs`), and **silently returns `false`
   with no error** if that tree isn't fully synced at the moment of the call.
   This mesh's connection churn (every captured serial log showed repeated
   "connections changed" events even when a peer never actually left) made
   this unreliable in practice. `mesh.sendBroadcast()` has no such dependency —
   it just iterates the direct connection list directly. Fixed by routing all
   targeted sends through the same broadcast mechanism as untargeted ones,
   with each receiving node self-filtering against the embedded `targetNodes`
   list (see "Message envelope & targeting" above).

## The unresolved bug: stale node IDs from the routing tree

After fix #5, per-node targeting was still unreliable — full/untargeted
broadcasts worked, but targeting one or more specific nodes still silently
failed to reach them. This was root-caused (again via direct source review,
not guessing) to a second, related instance of the *exact same* class of bug:

`GET /getMeshNodes` (which populates the Nodes-page checkboxes) originally
built its list via `mesh.getNodeList(false)`. Reading `mesh.hpp` confirmed
`getNodeList()` is implemented as `layout::asList(this->asNodeTree(), ...)` —
i.e. it's sourced from the **same routing tree** already proven unreliable
under this mesh's connection churn in fix #5. A node ID read from that tree
could be selected on the Nodes page yet never match that peer's own live
`mesh.getNodeId()` in `meshReceivedCallback`'s self-filter — the two would
silently disagree with no error anywhere, exactly matching the observed
symptom (targeted sends reaching no one, broadcasts reaching everyone).

By contrast, tracing the packet layer (`protocol.hpp`, `router.hpp`'s
`routePackage`) confirmed that the `from` parameter delivered to
`mesh.onReceive()` callbacks is embedded once by the *true originating*
sender and relayed unchanged through every hop — genuinely independent of the
routing tree. This is exactly what `knownNodes` (populated via the `nodeInfo`
heartbeat) already used.

**The fix applied:** `/getMeshNodes` was changed to build its list from
`knownNodes` (keyed by the reliable `from`) instead of `mesh.getNodeList()`,
aging out entries not heard from in 3 heartbeat intervals (60s) so a
genuinely-departed node still disappears. This was deployed to all four
devices, but **did not resolve the issue** — the user reported targeting was
still unreliable, alongside continuing crashes. No further root cause was
identified before the decision was made to remove the feature entirely
rather than continue chasing it.

## Why the feature was removed

Given:
- Multiple rounds of crash-fixing that resolved distinct, real bugs but never
  fully stabilized mesh mode,
- A second confirmed instance of "silently reads from the same unreliable
  routing tree" surfacing after the first was fixed, suggesting more of the
  same class of bug could plausibly exist elsewhere in how this feature used
  painlessMesh,
- Continued crashes and unreliable per-node targeting even after the above
  fixes were deployed,

the decision was made to drop the mesh networking feature entirely rather
than keep investing in painlessMesh-specific debugging. Standalone/LAN mode's
own per-node targeting feature (browser-side fan-out to each device's own
address, with no device-to-device relay at all) is unaffected by any of this
and was kept — it never depended on painlessMesh or its routing tree in the
first place.
