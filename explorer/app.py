from flask import Flask, jsonify, render_template, request
import os
import requests

RPC_URL = os.environ.get("DRACHMA_RPC_URL", "http://localhost:18443")
RPC_AUTH = (os.environ.get("DRACHMA_RPC_USER", "user"), os.environ.get("DRACHMA_RPC_PASS", "pass"))

def rpc_call(method, params=None):
    payload = {"jsonrpc": "2.0", "id": "explorer", "method": method, "params": params or []}
    resp = requests.post(RPC_URL, json=payload, auth=RPC_AUTH, timeout=5)
    resp.raise_for_status()
    data = resp.json()
    return data.get("result")


def create_app():
    app = Flask(__name__)

    @app.route("/")
    def index():
        stats = network_stats()
        return render_template("index.html", stats=stats)

    @app.route("/block/<height>")
    def block_height(height):
        try:
            h = int(height)
            block_hash = rpc_call("getblockhash", [h])
            return block_hash_view(block_hash)
        except Exception as exc:
            return jsonify({"error": str(exc)}), 400

    @app.route("/block/hash/<block_hash>")
    def block_hash_view(block_hash):
        block = rpc_call("getblock", [block_hash, True])
        return render_template("block.html", block=block)

    @app.route("/tx/<txid>")
    def tx_view(txid):
        tx = rpc_call("getrawtransaction", [txid, True])
        return render_template("tx.html", tx=tx)

    @app.route("/address/<address>")
    def address_view(address):
        history = rpc_call("getaddresstxids", [{"addresses": [address]}]) or []
        balance = rpc_call("getaddressbalance", [{"addresses": [address]}]) or {}
        return render_template("address.html", address=address, history=history, balance=balance)

    @app.route("/api/block/<height>")
    def api_block_height(height):
        h = int(height)
        return jsonify(rpc_call("getblock", [rpc_call("getblockhash", [h]), True]))

    @app.route("/api/block/hash/<block_hash>")
    def api_block_hash(block_hash):
        return jsonify(rpc_call("getblock", [block_hash, True]))

    @app.route("/api/tx/<txid>")
    def api_tx(txid):
        return jsonify(rpc_call("getrawtransaction", [txid, True]))

    @app.route("/api/address/<address>")
    def api_address(address):
        return jsonify({
            "balance": rpc_call("getaddressbalance", [{"addresses": [address]}]),
            "txids": rpc_call("getaddresstxids", [{"addresses": [address]}])
        })

    @app.route("/api/network")
    def api_network():
        return jsonify(network_stats())

    def network_stats():
        info = rpc_call("getmininginfo", []) or {}
        net = rpc_call("getnetworkhashps", [])
        info["networkhashps"] = net
        return info

    return app


if __name__ == "__main__":
    app = create_app()
    app.run(host="0.0.0.0", port=int(os.environ.get("PORT", "8080")))
