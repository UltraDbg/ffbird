#!/usr/bin/env bash
set -euo pipefail
# vendor-bionic.sh — clone latest AOSP Bionic into bionic/ (read-only vendor)
# Usage: ./scripts/vendor-bionic.sh [rev]
# Default: aosp-main latest
set -x
REV="${1:-aosp-main}"
SRC="https://android.googlesource.com/platform/bionic"
TMP="/tmp/bionic-latest-$$"
echo "[vendor-bionic] cloning $SRC @ $REV -> $TMP"
git clone --depth 1 --branch "$REV" "$SRC" "$TMP" 2>&1 | tail -20
# Fallback to single branch without --branch if not found
if [ ! -d "$TMP/bionic" ] && [ ! -f "$TMP/Android.bp" ]; then
  echo "[vendor-bionic] retry without branch"
  rm -rf "$TMP"
  git clone --depth 1 "$SRC" "$TMP"
fi
echo "[vendor-bionic] rsync to bionic/ (keep .git for rev)"
mkdir -p bionic
rsync -a --delete --exclude '.git' "$TMP/" bionic/ 2>&1 | tail -20
# Keep rev
if [ -d "$TMP/.git" ]; then
  git -C "$TMP" rev-parse --short HEAD > bionic/.aosp_rev
  git -C "$TMP" describe --tags --always > bionic/.aosp_desc 2>&1 || true
  git -C "$TMP" log --oneline -3 > bionic/.aosp_log 2>&1 || true
fi
rm -rf "$TMP"
echo "[vendor-bionic] done — bionic/.aosp_rev: $(cat bionic/.aosp_rev 2>&1)"
cat bionic/.aosp_desc 2>&1 | head -5
ls -lh bionic/README* 2>&1 | head -5
