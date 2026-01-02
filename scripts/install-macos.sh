#!/usr/bin/env bash
set -euo pipefail

PREFIX=${PREFIX:-/usr/local}

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target drachmad drachma_cli

install -d "${PREFIX}/bin"
install -m 0755 build/layer1-core/drachmad "${PREFIX}/bin/drachmad"
install -m 0755 build/layer1-core/drachma-cli "${PREFIX}/bin/drachma-cli"
if [ -f build/layer1-core/drachma-qt ]; then
  install -m 0755 build/layer1-core/drachma-qt "${PREFIX}/bin/drachma-qt"
fi
echo "Installed drachmad and tools to ${PREFIX}/bin"
