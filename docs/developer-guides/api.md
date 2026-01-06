# DRACHMA JSON-RPC API (Layer 2)

The Layer 2 RPC server exposes lightweight JSON-RPC methods over HTTP with **Basic Auth**. Requests use `Content-Type: application/json` and respond with a JSON object containing a `result` string or `error` message.

- **Default ports:** 8332 (mainnet), 18332 (testnet)
- **Authentication:** Basic Auth using `--rpcuser` / `--rpcpassword` flags passed to `drachmad` or the Layer 2 service launcher.
- **Networking:** RPC never bypasses validation. Submitted transactions must satisfy consensus and mempool policy before broadcast.

## Quickstart

```bash
curl --user user:pass \
  --data '{"method":"getblockcount","params":[]}' \
  -H 'content-type: application/json' \
  http://127.0.0.1:18332/
```

All examples below omit the outer JSON-RPC envelope for brevity. The RPC server returns serialized strings; callers may parse as JSON where noted.

## Methods

### `getblockcount`
Returns the current best-known block height tracked by the transaction index.

- **Params:** none
- **Result:** integer height as a string

Example:
```json
{"result":"1024"}
```

### `getblock`
Returns the raw serialized block (header + tx count + transactions) for a given block hash.

- **Params:** block hash hex string
- **Result:** JSON string with height and hex payload

Example:
```json
{"result":"{\"height\":1024,\"hex\":\"01000000...\"}"}
```

### `gettransaction`
Checks whether a transaction hash is present in the transaction index and, if known, at which height.

- **Params:** transaction hash hex string
- **Result:** JSON string indicating presence and height

Example:
```json
{"result":"{\"found\":true,\"height\":1008}"}
```

### `sendrawtransaction` / `sendtx`
Submits a hex-encoded transaction to the mempool and broadcasts it to peers if accepted. `sendrawtransaction` is an alias for `sendtx`.

- **Params:** hex-encoded serialized transaction
- **Result:** JSON string reporting acceptance

Example:
```json
{"result":"{\"accepted\":true}"}
```

### `getbalance`
Returns the wallet backend's confirmed spendable balance (in smallest units).

- **Params:** none
- **Result:** integer string representing balance in satoshis of DRM

Example:
```json
{"result":"125000000"}
```

### `estimatefee`
Returns the fee rate estimate at a target percentile of recent mempool fees.

- **Params:** percentile as an integer string (optional, default `50`)
- **Result:** integer string representing fee rate (satoshis per virtual byte)

Example:
```json
{"result":"35"}
```

## Error Handling

If authentication fails or an unknown method is called, the server responds with HTTP 401 or 400 respectively and an `error` field describing the problem. Invalid parameters surface as JSON parse or validation errors.

## Versioning

This API is **testnet ready** and may add new methods ahead of mainnet. Backwards-incompatible changes will be announced in the changelog and release notes.
