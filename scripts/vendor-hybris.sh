#!/usr/bin/env bash
set -euo pipefail
# vendor-hybris.sh — clone latest libhybris into hybris/ (read-only vendor, cleaned)
set -x
SRC="https://github.com/libhybris/libhybris"
TMP="/tmp/libhybris-latest-$$"
echo "[vendor-hybris] cloning $SRC -> $TMP"
git clone --depth 1 "$SRC" "$TMP" 2>&1 | tail -20
echo "[vendor-hybris] rsync to hybris/ (keep .git for rev)"
mkdir -p hybris
rsync -a --delete --exclude '.git' "$TMP/" hybris/ 2>&1 | tail -20
if [ -d "$TMP/.git" ]; then
  git -C "$TMP" rev-parse --short HEAD > hybris/.hybris_rev
  git -C "$TMP" describe --tags --always > hybris/.hybris_desc 2>&1 || true
fi
rm -rf "$TMP"
echo "[vendor-hybris] done — hybris/.hybris_rev: $(cat hybris/.hybris_rev 2>&1)"
# Note: cleaning happens via CMake — only hybris/common (hooks/wrappers + single n/q) is built
ls -lh hybris/README* 2>&1 | head -5
