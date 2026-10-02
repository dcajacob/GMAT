# Private Wayland portal FileChooser evidence — 2026-10-02

The closed private session `isolated-wayland-p8bnkx9c` proves actual Qt
xdg-desktop-portal Open cancellation, accepted Save, and output-directory
cancellation/selection in its owned software Wayland environment. Those four
cases come from correlated FileChooser request tokens and Request.Response
signals on the owned bus, supported by saved bytes and compositor screenshots.
The outer harness ended with a response-parsing failure; later action labels
are not accepted as results. No new GUI session was run to prepare this record.

Raw worker actions, all 16 screenshots, portal monitor, launch identities,
private bus activation details and final saved source remain unchanged under
ignored `build/example-qualification/20261002/wayland/portal-first-attempt/isolated-wayland-p8bnkx9c/`.
The adjacent preservation index hashes all regular files; package-preparation
retains the original official package, cached metadata, extraction, linkage and
full copyright/license. first-attempt-disposition.json separately records the
parent-observed outer errors because the old helper did not persist them.

## Runtime and temporary plugin

| Launched file | SHA256 |
| --- | --- |
| `GmatQt-R2026a` | `7b4a0ae720bb074c71ac05e73d351e987beae5c7fcda524e105d7fb1d55fdae3` |
| `libGmatBase.so.R2026a` | `cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016` |
| `libGmatUtil.so.R2026a` | `1e4e7fe6ba83701266ef780588003c3ae607b6ebc56ecc040f9cfac399777fcc` |
| Helper, before protocol repair | `789b0bbd3a5c07b003313d9cc1e61715c49eda8947b997a824441c1537f1c127` |
| Selected startup | `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a` |
| Isolated startup, OUTPUT_PATH only changed | `85a6b6f87261253c0b9422a996acf2a22e458beba156f2a70b12710433b6bfd0` |
| Qt portal theme | `8abfe7b383ccecc39ae357bc05a98c35b92b483e05a147582f23bca2622cd7b8` |
| Copied/final saved fixture, 5371 bytes | `2d30468533a50b6495871e381803045d687fc0429b504b4d6739f2b8cd37e8a4` |

The plugin is extracted from the official Ubuntu resolute/universe package
[qt6-xdgdesktopportal-platformtheme 6.10.2+dfsg-7 amd64](https://archive.ubuntu.com/ubuntu/pool/universe/q/qt6-base/qt6-xdgdesktopportal-platformtheme_6.10.2+dfsg-7_amd64.deb).
The 69966-byte .deb SHA256 is d6a5106fe15f37e1e63c06fdccbc446640da5578e6dd150d04263bca9d636537,
matching the pre-existing apt package metadata and installed Qt6.10.2/private ABI.
No host installation or apt update occurred. Preparation provenance's
application_tested=false is its historical pre-launch state, not this later
session's disposition. Full notices are retained; package default licensing is
LGPL-3 or GPL-2 with per-file exceptions/terms. The copyright file SHA256 is
9bc6203f2b6fba7a47138f8370afeff96fff8d67c44ee2752ea4b4e02438d961.

Only the SHA-pinned plugin file was bound read-only at
`/portal-plugins/platformthemes/libqxdgdesktopportal.so`. GmatQt alone received
QT_PLUGIN_PATH=/portal-plugins and QT_QPA_PLATFORMTHEME=xdgdesktopportal. The owned
bus activation environment set WAYLAND_DISPLAY=gmat-private, GDK_BACKEND=wayland,
private runtime, and the same owned session-bus system-API alias. The owned
dbus-monitor filters captured FileChooser methods and Request.Response; its PID
inside the namespace was 8. Strict runtime/bus/physical-device/GPU isolation from
[the native GTK lifecycle record](private-wayland-20261002.md) remained in force.

## Correlated results

The Qt caller is :1.35. Accepted FileChooser responses below use exact paths
`/org/freedesktop/portal/desktop/request/1_35/<token>`. Unrelated `/1_57/t`,
`/1_76/t` and `/1_83/t` Response 0/session_handle signals are not FileChooser
acceptance evidence.

| Token / portal method | Actual Response and evidence |
| --- | --- |
| qt1297827529 / OpenFile, directory=false | Response 1, empty uris. Screenshot 002 shows the actual Open chooser. This is Open Cancel. |
| qt3845851032 / SaveFile | Response 0, `file:///evidence/portal-authored.script`, GMAT script filter. Screenshot 004 returns to the main editor titled portal-authored.script; final saved file equals fixture byte for byte, 5371 bytes with the full SHA256 recorded above. This proves accepted Save. |
| qt147538978 / OpenFile, directory=true | Response 1, empty uris. Output directory remains `/evidence/output` in screenshot 010. This proves directory Cancel/path retention. |
| qt3382534876 / OpenFile, directory=true | Response 0, `file:///evidence/output`. Screenshot 009 shows the actual directory chooser; 011 shows its selected path in Set paths. This proves selection of the existing directory, not Apply or changed-path mission execution. |
| qt1804082445 / OpenFile, directory=false | Response 1. Screenshot 012 “portal-reopened” still shows Open; no reopened-source claim. |
| qt348899347 / OpenFile, directory=false | Response 1. Screenshots 014/015 “Open accepted”/“Save cancel” still show Open. No second SaveFile request exists, so these labels do not prove either claim. |

A real resource-tree leaf click at 229.595 seconds and Return at 229.784 seconds opens the
GroundStation1 Setup editor in screenshot 013, showing StationId, min elevation
21.5, Earth/Cartesian. That proves actual native keyboard activation after the
new tree fix; it is not a new GroundStation creation, Apply, calculation or
Help tutorial walkthrough. The source fixture was previously independently
GUI-authored and loaded before this session; no new mission execution occurred.

Selected unchanged compositor PNGs:

- [Actual portal Open chooser](private-portal-20261002/portal-open.png)
- [Actual portal Save chooser](private-portal-20261002/portal-save.png)
- [Actual directory chooser](private-portal-20261002/portal-directory.png)

All result screenshots, including saved main title 004, directory Cancel/Select
010/011 and actual resource Return 013, remain in the ignored raw evidence.
`selected-screenshots.json` beside the preservation index records exact selected
PNG origins, bytes and SHA256.

## First failure, repair and limits

Parent observed an early `Expecting value: line 1` and a final outer CLI fatal
`dict object has no attribute removeprefix`. Raw actions show the worker recorded
a fixture dictionary, followed by a valid screenshot 016 path, while the outer
controller interpreted the fixture reply as a screenshot. The old protocol
accepted every stdout line as JSON and used the next reply without request ID
or action correlation. The precise earlier malformed-line origin is unknown
because raw stdout was not retained; do not claim a proven partial-read cause.

The repaired helper SHA256 is 307c121fdd493909a1f20c9ad3cd4ebcde3eca27943cff244c7c9ec5a2887611.
It uses explicit reply frames and sequential request IDs/action correlation,
checks screenshot type/owned path, retains raw stdout/noise/protocol errors and
persists fatal failure in session.json. A new synthetic pipe-only regression
passed nine cases, with zero compositor/GUI launches. Its reusable script and
recorded result are retained as `protocol-check.py` and
`protocol-check-result.json` beside the ignored first-attempt evidence; no new
source test was added. This validates the transport contract; a repaired real
portal rerun has not occurred here.

The original closed session preserves sandbox exit 0 (worker cleanup) separately
from the parent's observed outer fatal error. Owned compositor/PipeWire exits
are 0; app and monitor are -15 during bounded cleanup. Host GNOME PID 399961/start
ticks 38989406 remained unchanged. This does not qualify graceful app Quit, host
GNOME/portals, physical GPUs or a fix for the earlier desktop crash. The four
private portal cases narrow only this exact matching-plugin/software/private-bus
configuration. Open acceptance and Save cancellation are not established by
this session. Wider lifecycle/driver and full Linux replacement gates remain
open. This record makes no GUI-built Help tutorial claim; walkthrough state is
maintained separately. Windows/macOS and MATLAB remain deferred.
