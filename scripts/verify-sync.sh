#!/bin/bash
set -euo pipefail

RPC_URL=${RPC_URL:-"http://127.0.0.1:18332"}
RPC_USER=${RPC_USER:-"user"}
RPC_PASS=${RPC_PASS:-"pass"}
EXPECTED_HEIGHT=${EXPECTED_HEIGHT:-0}

call_rpc() {
  local method=$1
  local params=${2:-"[]"}
  curl -s --user "${RPC_USER}:${RPC_PASS}" --data-binary "{\"jsonrpc\":\"1.0\",\"id\":\"sync\",\"method\":\"${method}\",\"params\":${params}}" -H 'content-type: text/plain;' "${RPC_URL}" | jq -r '.result'
}

HEIGHT=$(call_rpc getblockcount)
TIP_HASH=$(call_rpc getbestblockhash)

if [[ -z "$HEIGHT" || "$HEIGHT" == "null" ]]; then
  echo "Unable to query node height" >&2
  exit 1
fi

echo "Node height: ${HEIGHT}, tip: ${TIP_HASH}"
if [[ "$HEIGHT" -lt "$EXPECTED_HEIGHT" ]]; then
  echo "Node is behind expected height ${EXPECTED_HEIGHT}" >&2
  exit 2
fi

echo "Sync verification passed"
