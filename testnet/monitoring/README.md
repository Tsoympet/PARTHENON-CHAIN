# Testnet Monitoring

Prometheus and Grafana configurations for observing DRACHMA testnet nodes. The
node binary should expose metrics or be paired with an exporter that exposes
process metrics on port 9311 by default.

- `prometheus.yml`: Scrape config targeting multiple testnet nodes.
- `grafana_dashboard.json`: Minimal dashboard with block height, peers, and mempool
  utilization panels.

## Usage
1. Start Prometheus with `prometheus --config.file=prometheus.yml`.
2. Import `grafana_dashboard.json` into Grafana and point the datasource to your
   Prometheus instance.
3. Add alerting rules for block height stalls and high orphan rates.
