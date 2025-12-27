#!/usr/bin/env bash
set -euo pipefail

# Helper to launch a production DRACHMA mainnet node with reproducible
# configuration defaults. Expects binaries installed in PATH.

CONFIG_PATH=${CONFIG_PATH:-$HOME/.drachma/drachma.conf}
DATA_DIR=${DATA_DIR:-$HOME/.drachma}
LOGFILE=${LOGFILE:-$DATA_DIR/drachma.log}

if [ ! -f "$CONFIG_PATH" ]; then
  echo "Configuration missing at $CONFIG_PATH" >&2
  echo "Copy mainnet/config.sample.conf and edit RPC credentials, peers, and pruning." >&2
  exit 1
fi

mkdir -p "$(dirname "$LOGFILE")"

echo "Starting DRACHMA mainnet node..."
exec drachmad -conf="$CONFIG_PATH" -datadir="$DATA_DIR" -daemon=0 -logfile="$LOGFILE" "$@"
