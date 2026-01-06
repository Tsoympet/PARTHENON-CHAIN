# DRACHMA RPC API Reference

This reference summarizes common JSON-RPC methods exposed by the Layer 2 services daemon. Endpoints default to `http://127.0.0.1:8332` (mainnet) or `18332` (testnet) and require HTTP basic auth unless otherwise configured.

## Conventions
- All requests use `Content-Type: text/plain` and a JSON-RPC 2.0 envelope.
- Authentication: `--rpcuser` / `--rpcpassword` (or cookie-based auth if enabled).
- Amounts are expressed in DRM.
- Error responses include `code` and `message`; non-zero codes indicate failure.

## Methods

### `getblockchaininfo`
Returns chain status and synchronization state.
```bash
curl --user user:pass \
  --data-binary '{"jsonrpc":"2.0","id":"info","method":"getblockchaininfo","params":[]}' \
  -H 'content-type: text/plain;' http://127.0.0.1:8332/
```
Example response:
```json
{
  "result": {
    "chain": "testnet",
    "blocks": 123456,
    "headers": 123460,
    "difficulty": 152341.102,
    "verificationprogress": 0.9987,
    "initialblockdownload": false
  },
  "error": null,
  "id": "info"
}
```

### `getblock`
Fetch a block by hash. Include transactions with `verbosity=2`.
```bash
HASH=$(curl --user user:pass \
  --data-binary '{"jsonrpc":"2.0","id":"tip","method":"getbestblockhash","params":[]}' \
  -H 'content-type: text/plain;' http://127.0.0.1:8332/ | jq -r .result)

curl --user user:pass \
  --data-binary '{"jsonrpc":"2.0","id":"blk","method":"getblock","params":["'"'"$HASH'"'" ,2]}' \
  -H 'content-type: text/plain;' http://127.0.0.1:8332/
```

### `getnewaddress`
Derive a new bech32m address from the default wallet.
```bash
curl --user user:pass \
  --data-binary '{"jsonrpc":"2.0","id":"addr","method":"getnewaddress","params":[]}' \
  -H 'content-type: text/plain;' http://127.0.0.1:8332/
```

### `sendtoaddress`
Send funds to a DRM address with optional comment and fee parameters.
```bash
curl --user user:pass \
  --data-binary '{"jsonrpc":"2.0","id":"send","method":"sendtoaddress","params":["DRM1abc...", 1.25, "payout", ""]}' \
  -H 'content-type: text/plain;' http://127.0.0.1:8332/
```
Response contains a transaction ID on success.

### `getpeerinfo`
Inspect connected peers for troubleshooting.
```bash
curl --user user:pass \
  --data-binary '{"jsonrpc":"2.0","id":"peers","method":"getpeerinfo","params":[]}' \
  -H 'content-type: text/plain;' http://127.0.0.1:8332/
```

### `getmininginfo`
Return miner-facing data such as difficulty and block templates.
```bash
curl --user user:pass \
  --data-binary '{"jsonrpc":"2.0","id":"mining","method":"getmininginfo","params":[]}' \
  -H 'content-type: text/plain;' http://127.0.0.1:8332/
```

### `getblocktemplate`
Fetch a block template for miners. Set `longpollid` to reduce redundant polling.
```bash
curl --user user:pass \
  --data-binary '{"jsonrpc":"2.0","id":"tmpl","method":"getblocktemplate","params":[{"rules":["segwit"],"longpollid":""}]}' \
  -H 'content-type: text/plain;' http://127.0.0.1:8332/
```

### `submitblock`
Submit a solved block header + transactions.
```bash
curl --user user:pass \
  --data-binary '{"jsonrpc":"2.0","id":"submit","method":"submitblock","params":["<hex_block>"]}' \
  -H 'content-type: text/plain;' http://127.0.0.1:8332/
```

## Errors and Troubleshooting
- `-32601 Method not found`: Ensure the wallet or service exposing the method is enabled (some methods require wallet support).
- `-28 Loading block index...`: The node is still initializing; retry after synchronization progresses.
- `-13 RPC authorization failed`: Verify credentials and that RPC is bound to the interface you are contacting.
- Empty responses or timeouts: confirm firewall rules allow localhost/VPN access and the daemon is running.

## Versioning
RPC surfaces follow semantic versioning with backward-compatible additions where possible. Breaking changes are announced in release notes and tagged in `CHANGELOG.md`.
