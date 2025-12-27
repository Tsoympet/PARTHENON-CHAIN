#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
NODE_BIN="${ROOT_DIR}/build/drachma_node"
DATADIR_BASE="${ROOT_DIR}/.drachma-testnet"
RPC_USER="user"
RPC_PASS="pass"

usage() {
  echo "Usage: $0 [--binary <path>] [--nodes <count>]" >&2
}

NODE_COUNT=2
while [[ $# -gt 0 ]]; do
  case "$1" in
    --binary) NODE_BIN="$2"; shift 2 ;;
    --nodes) NODE_COUNT="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1"; usage; exit 1 ;;
  esac
done

if [[ ! -x "$NODE_BIN" ]]; then
  echo "Fatal: node binary not executable at $NODE_BIN" >&2
  exit 1
fi

mkdir -p "$DATADIR_BASE"

for i in $(seq 1 "$NODE_COUNT"); do
  DATADIR="${DATADIR_BASE}/node${i}"
  PORT=$((19335 + i))
  RPCPORT=$((18332 + i))
  mkdir -p "$DATADIR"
  cat >"${DATADIR}/drachma.conf" <<CFG
regtest=0
testnet=1
server=1
listen=1
port=${PORT}
rpcport=${RPCPORT}
rpcuser=${RPC_USER}
rpcpassword=${RPC_PASS}
addnode=127.0.0.1:$((19336 - i + 1))
CFG
  echo "Starting node ${i} on P2P ${PORT}, RPC ${RPCPORT}"
  "${NODE_BIN}" -conf="${DATADIR}/drachma.conf" -datadir="$DATADIR" >"${DATADIR}/debug.log" 2>&1 &
done

echo "Cluster started. Tail logs in ${DATADIR_BASE}/node*/debug.log"
