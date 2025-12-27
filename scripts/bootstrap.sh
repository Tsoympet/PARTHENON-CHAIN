#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DATA_DIR="${HOME}/.drachma"
OUTPUT="${ROOT_DIR}/bootstrap.dat"

usage() {
  cat <<USAGE
Usage: $0 [-d <data-dir>] [-o <output>]
  -d <data-dir>   Source data directory containing blocks/ (default: $DATA_DIR)
  -o <output>     Destination bootstrap file (default: $OUTPUT)

The script concatenates raw block files in height order into a deterministic
bootstrap.dat usable for offline sync. It never fetches data from the
network; the operator is responsible for providing vetted blocks.
USAGE
}

while getopts "d:o:h" opt; do
  case "$opt" in
    d) DATA_DIR="$OPTARG" ;;
    o) OUTPUT="$OPTARG" ;;
    h) usage; exit 0 ;;
    *) usage; exit 1 ;;
  esac
done

BLOCK_DIR="$DATA_DIR/blocks"
if [[ ! -d "$BLOCK_DIR" ]]; then
  echo "Block directory not found: $BLOCK_DIR" >&2
  exit 1
fi

tmp="${OUTPUT}.tmp"
rm -f "$tmp"
touch "$tmp"

# Concatenate blk*.dat files in lexical order for deterministic output.
for f in "$BLOCK_DIR"/blk*.dat; do
  [[ -e "$f" ]] || continue
  cat "$f" >> "$tmp"
done

mv "$tmp" "$OUTPUT"
echo "Bootstrap written to $OUTPUT"
