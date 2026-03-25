#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${ROOT_DIR}/build/debug/voidchain"
LOG_DIR="${ROOT_DIR}/build/cluster-logs"

if [[ ! -x "${BIN}" ]]; then
  echo "Missing binary: ${BIN}"
  echo "Run ./build.sh first."
  exit 1
fi

mkdir -p "${LOG_DIR}"

PIDS=()

cleanup() {
  echo ""
  echo "Stopping cluster..."
  for pid in "${PIDS[@]:-}"; do
    if kill -0 "${pid}" 2>/dev/null; then
      kill "${pid}" 2>/dev/null || true
    fi
  done
  wait || true
}

trap cleanup EXIT INT TERM

echo "Launching 3-node VoidChain cluster..."

PORT=18169 PEERS="ws://localhost:18170/ws,ws://localhost:18171/ws" \
  "${BIN}" >"${LOG_DIR}/node-18169.log" 2>&1 &
PIDS+=("$!")

PORT=18170 PEERS="ws://localhost:18169/ws,ws://localhost:18171/ws" \
  "${BIN}" >"${LOG_DIR}/node-18170.log" 2>&1 &
PIDS+=("$!")

PORT=18171 PEERS="ws://localhost:18169/ws,ws://localhost:18170/ws" \
  "${BIN}" >"${LOG_DIR}/node-18171.log" 2>&1 &
PIDS+=("$!")

sleep 2

echo "Cluster started."
echo "Node endpoints:"
echo "  http://localhost:18169"
echo "  http://localhost:18170"
echo "  http://localhost:18171"
echo ""
echo "Logs:"
echo "  ${LOG_DIR}/node-18169.log"
echo "  ${LOG_DIR}/node-18170.log"
echo "  ${LOG_DIR}/node-18171.log"
echo ""
echo "Press Ctrl+C to stop all nodes."

wait
