#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DEFAULT="${ROOT_DIR}/build/drachma_node"
CONFIG_DEFAULT="${ROOT_DIR}/drachma.conf"
DATA_DEFAULT="${HOME}/.drachma"

usage() {
  cat <<USAGE
Usage: $0 [-b <binary>] [-c <config>] [-d <datadir>] [-- test-args...]
  -b <binary>   Path to the full node executable (default: $BIN_DEFAULT)
  -c <config>   Configuration file path (default: $CONFIG_DEFAULT)
  -d <datadir>  Data directory override (default: $DATA_DEFAULT)

The script validates that the node binary exists, sets sane defaults, and
executes the node with the provided arguments. Any parameters after '--'
are passed through untouched.
USAGE
}

BINARY="$BIN_DEFAULT"
CONFIG="$CONFIG_DEFAULT"
DATADIR="$DATA_DEFAULT"

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    -b) BINARY="$2"; shift 2 ;;
    -c) CONFIG="$2"; shift 2 ;;
    -d) DATADIR="$2"; shift 2 ;;
    --) shift; break ;;
    *) echo "Unknown option: $1"; usage; exit 1 ;;
  esac
done

if [[ ! -x "$BINARY" ]]; then
  echo "Fatal: node binary not found at $BINARY" >&2
  echo "Build with scripts/build.sh and ensure the executable name matches."
  exit 1
fi

mkdir -p "$DATADIR"
exec "$BINARY" -conf="$CONFIG" -datadir="$DATADIR" "$@"
