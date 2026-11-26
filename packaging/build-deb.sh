#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="${CARSONBOOT_VERSION:-0.1.0}"
ARCH="${CARSONBOOT_ARCH:-amd64}"
PACKAGE_NAME="carsonboot"
STAGE_DIR="$ROOT_DIR/build/deb-root"
OUTPUT_DIR="$ROOT_DIR/build/deb"

if [[ ! -f "$ROOT_DIR/build/BOOTX64.EFI" ]]; then
    echo "Missing build/BOOTX64.EFI. Run 'make' first." >&2
    exit 1
fi

rm -rf "$STAGE_DIR"
mkdir -p "$STAGE_DIR/DEBIAN" "$STAGE_DIR/usr/lib/carsonboot" "$OUTPUT_DIR"

install -m 0644 "$ROOT_DIR/build/BOOTX64.EFI" "$STAGE_DIR/usr/lib/carsonboot/BOOTX64.EFI"
install -m 0644 "$ROOT_DIR/packaging/DEBIAN/control" "$STAGE_DIR/DEBIAN/control"

sed -i "s/^Version: .*/Version: $VERSION/" "$STAGE_DIR/DEBIAN/control"
sed -i "s/^Architecture: .*/Architecture: $ARCH/" "$STAGE_DIR/DEBIAN/control"

dpkg-deb --root-owner-group --build "$STAGE_DIR" "$OUTPUT_DIR/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
echo "Package created: $OUTPUT_DIR/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
