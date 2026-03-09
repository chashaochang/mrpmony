#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

status() {
  printf '%-32s %s\n' "$1" "$2"
}

check_file() {
  local path="$1"
  if [ -e "$path" ]; then
    status "$path" "OK"
  else
    status "$path" "MISSING"
    return 1
  fi
}

FAILED=0

echo "[MRP HarmonyOS Init Check]"
echo "root=$ROOT_DIR"

echo "\n# Project shell files"
check_file "AppScope/app.json5" || FAILED=1
check_file "build-profile.json5" || FAILED=1
check_file "hvigorfile.ts" || FAILED=1
check_file "oh-package.json5" || FAILED=1
check_file "entry/oh-package.json5" || FAILED=1
check_file "entry/build-profile.json5" || FAILED=1
check_file "entry/src/main/module.json5" || FAILED=1
check_file "entry/src/main/resources" || FAILED=1
check_file "entry/src/main/ets" || FAILED=1
check_file "entry/src/main/cpp" || FAILED=1

echo "\n# Wrapper / runtime tools"
if [ -x "./hvigorw" ]; then
  status "./hvigorw" "OK"
else
  status "./hvigorw" "NOT_EXECUTABLE"
  FAILED=1
fi

if command -v hvigor >/dev/null 2>&1; then
  status "hvigor" "FOUND"
else
  status "hvigor" "MISSING"
  FAILED=1
fi

if command -v node >/dev/null 2>&1; then
  status "node" "FOUND: $(node -v)"
else
  status "node" "MISSING"
  FAILED=1
fi

echo "\n# Local machine files"
if [ -f "local.properties" ]; then
  status "local.properties" "FOUND"
  HWSDK_DIR="$(sed -n 's/^hwsdk\.dir=//p' local.properties | tail -1)"
  if [ -z "$HWSDK_DIR" ]; then
    status "hwsdk.dir" "EMPTY"
    FAILED=1
  elif [[ "$HWSDK_DIR" == "/absolute/path/to/HarmonyOS/Sdk" ]]; then
    status "hwsdk.dir" "PLACEHOLDER"
    FAILED=1
  elif [ -d "$HWSDK_DIR" ]; then
    status "hwsdk.dir" "CONFIGURED"
  else
    status "hwsdk.dir" "PATH_NOT_FOUND"
    FAILED=1
  fi
else
  status "local.properties" "MISSING"
  FAILED=1
fi

echo "\n# Conclusion"
if [ "$FAILED" -eq 0 ]; then
  echo "INIT_PREREQUISITES_OK"
else
  echo "INIT_PREREQUISITES_BLOCKED"
  exit 2
fi
