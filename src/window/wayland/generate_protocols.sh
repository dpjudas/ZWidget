#!/bin/bash
SCANNER=wayland-scanner
PROTO_DIR=/usr/share/waylandpp/protocols
QT_PROTO_DIR=/usr/share/qt6/wayland/protocols
OUT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

protocols=(
    "$PROTO_DIR/xdg-shell.xml:xdg-shell"
    "$PROTO_DIR/xdg-output-unstable-v1.xml:xdg-output-unstable-v1"
    "$PROTO_DIR/xdg-foreign-unstable-v2.xml:xdg-foreign-unstable-v2"
    "$PROTO_DIR/pointer-constraints-unstable-v1.xml:pointer-constraints-unstable-v1"
    "$PROTO_DIR/xdg-activation-v1.xml:xdg-activation-v1"
    "$PROTO_DIR/xdg-decoration-unstable-v1.xml:xdg-decoration-unstable-v1"
    "$QT_PROTO_DIR/fractional-scale/fractional-scale-v1.xml:fractional-scale-v1"
    "$PROTO_DIR/relative-pointer-unstable-v1.xml:relative-pointer-unstable-v1"
    "$QT_PROTO_DIR/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml:xdg-toplevel-icon-v1"
    "$QT_PROTO_DIR/cursor-shape/cursor-shape-v1.xml:cursor-shape-v1"
    "$QT_PROTO_DIR/xdg-dialog/xdg-dialog-v1.xml:xdg-dialog-v1"
)

for p in "${protocols[@]}"; do
    xml="${p%%:*}"
    base="${p#*:}"
    if [ -f "$xml" ]; then
        echo "Generating $base..."
        $SCANNER client-header "$xml" "$OUT_DIR/$base-client-protocol.h"
        $SCANNER private-code "$xml" "$OUT_DIR/$base-protocol.c"
    else
        echo "Warning: $xml not found!"
    fi
done
