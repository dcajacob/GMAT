# Saved GNOME popup null-resource fault — 2026-10-02

Read-only analysis of the previously recorded GNOME Shell crash identifies its
immediate fault: Mutter calls `xdg_popup_send_popup_done` with a null Wayland
resource. This is new evidence from the saved event, not a new reproduction.
Client attribution, the exact triggering interleaving, a repair and safe host
GNOME/physical-GPU acceptance remain unqualified.

The [original crash record](resource-category-menu-compositor-crash.txt) is
unchanged. PID 15518's saved event timestamp is 2026-10-01 11:34:41 MDT; the
historical journal/service report completes at 11:34:45 MDT. These are distinct
recorded timestamps, not two crashes. The
[filtered saved-core trace](compositor-popup-null-resource-20261002-trace.txt)
and [metadata/checksum receipt](compositor-popup-null-resource-20261002-metadata.json)
retain the new result without publishing the raw desktop core or full debugger
output.

## Matching binaries and observed fault

Recorded Mutter 50.1-0ubuntu2.4 build ID
`23093aa9e294df23a746c513b1597f376b0b8f1e` matches both the module metadata and
public Ubuntu debug symbols. The original archive and Ubuntu patch archive were
checked against the official DSC SHA256 entries; all 30 distribution patches
were applied only to temporary source. No package installation, build, system
configuration change or PGP verification is claimed.

The saved stack resolves to `finish_popup_setup`, exact patched Ubuntu
`meta-wayland-xdg-shell.c:1332`, through popup/surface application, transaction
application and DMA-buffer completion. At `wl_resource_post_event+108`, RDI and
R11 are both zero, RSI is 1 and the faulting instruction dereferences R11.
The matching Wayland entry sequence copies its resource argument from RDI to
R11 without replacing it before this fault. Thus the null event resource is
observed, not inferred merely from a function name. The popup object and its
owner fields are optimized out; no GMAT client PID was recovered.

The first saved-core debugger attempt failed because of symbol/frame setup;
the second resolves the stack and null register, and the third confirms the
inline call, exact source and register values. Those are debugger reads, not
application tests. Mutter and Wayland module identities match; the current
GNOME executable and some other libraries differ from their saved versions,
so their unresolved frames are not attributed from current binaries.

## Source path and upstream check

Exact Ubuntu source exposes a matching unguarded path. Popup destruction
clears the protocol resource but retains pending setup fields (594–608).
Transactions can retain a surface after protocol-resource destruction;
DMA-buffer completion can resume their application. Popup apply calls setup
when `setup.parent_surface` remains (1473–1474). If the grab serial is rejected,
setup sends `popup_done` without checking the resource (1330–1332).
That destruction/transaction sequence is a source-backed explanation; the
saved optimized core does not establish its precise execution or client owner.

No matching public fix was located in the official upstream check. The
[inspected September 28 source](https://gitlab.gnome.org/GNOME/mutter/-/blob/290a4466c91a6d51451382de720a1007306b8e36/src/wayland/meta-wayland-xdg-shell.c#L1360)
still contains the unguarded invalid-seat branch. The earlier
[popup NULL guards](https://gitlab.gnome.org/GNOME/mutter/-/commit/a19f557f12470ed4d89f57c5ca38a502bf093b16)
cover four other sites and are already in 50.1.
[MR4740](https://gitlab.gnome.org/GNOME/mutter/-/merge_requests/4740)
addresses retained surface-client lookup during delayed window creation,
rather than this event send; its merged repair is also already represented
in 50.1. The searched/inspected changes are bounded evidence, not proof that
no fix exists anywhere.

## Acceptance consequence

No GMAT code change follows from this diagnostic alone. Earlier Qt popup
lifetime changes and private software desktop passes remain bounded evidence;
they do not repair or qualify this host fault. Live host testing remains
stopped. Closing that gate requires a compositor repair or other established
safe host condition, followed by the actual outstanding host/GPU operations.
Unchanged tutorial, corpus and application tests were not repeated, and the
rebuilt application remains SHA 989d3499… / base SHA 2082a341….
