#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: sync_verify.sh --nodes URL[,URL...] [--user rpcuser] [--password rpcpass] [--threshold blocks]

Checks multiple DRACHMA nodes via JSON-RPC and reports whether their block
heights are within the configured threshold. URLs should include scheme and
port, e.g. http://127.0.0.1:18332
USAGE
}

if [ $# -eq 0 ]; then
  usage
  exit 1
fi

RPC_USER="user"
RPC_PASS="pass"
THRESHOLD=3
NODES_CSV=""

while [ $# -gt 0 ]; do
  case "$1" in
    --nodes)
      NODES_CSV="$2"; shift 2 ;;
    --user)
      RPC_USER="$2"; shift 2 ;;
    --password)
      RPC_PASS="$2"; shift 2 ;;
    --threshold)
      THRESHOLD="$2"; shift 2 ;;
    -h|--help)
      usage; exit 0 ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 1 ;;
  esac
done

IFS=',' read -r -a NODES <<< "$NODES_CSV"
if [ ${#NODES[@]} -eq 0 ]; then
  echo "No nodes provided" >&2
  exit 1
fi

call_rpc() {
  local url="$1"
  local method="$2"
  local params_json="${3:-[]}" 
  local payload
  payload=$(printf '{"jsonrpc":"1.0","id":"sync","method":"%s","params":%s}' "$method" "$params_json")
  curl -s --fail --user "$RPC_USER:$RPC_PASS" \
    --data-binary "$payload" \
    -H 'content-type:text/plain;' "$url"
}

parse_field() {
  local json_input="$1"
  local expr="$2"
  python3 - "$json_input" "$expr" <<'PY'
import json, sys
resp = json.loads(sys.argv[1])
expr = sys.argv[2]
value = resp
for part in expr.split('.'):
    value = value[part]
print(value)
PY
}

heights=()
for node in "${NODES[@]}"; do
  response=$(call_rpc "$node" getblockchaininfo)
  height=$(parse_field "$response" "result.blocks")
  tiphash=$(parse_field "$response" "result.bestblockhash")
  heights+=("$height")
  printf "%-30s height=%-8s tip=%s\n" "$node" "$height" "$tiphash"
done

top_height=${heights[0]}
for h in "${heights[@]}"; do
  if [ "$h" -gt "$top_height" ]; then
    top_height="$h"
  fi
done

out_of_sync=0
for h in "${heights[@]}"; do
  if [ $((top_height - h)) -gt "$THRESHOLD" ]; then
    out_of_sync=1
    break
  fi
done

if [ "$out_of_sync" -eq 1 ]; then
  echo "WARNING: at least one node lags more than ${THRESHOLD} blocks behind the best peer." >&2
  exit 2
fi

echo "All nodes within ${THRESHOLD} blocks of best height ($top_height)."
