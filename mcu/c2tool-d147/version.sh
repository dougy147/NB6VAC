#!/bin/sh

VERSION="0.01"
OUT="$1"

v="$VERSION"

echo '#include "c2tool.h"' > "$OUT"
echo "const char c2tool_version[] = \"$v\";" >> "$OUT"
