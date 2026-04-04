#!/bin/bash
# test_concurrent_debug.sh - shows request details

PORT=${1:-8080}
SERVER="localhost:$PORT"

echo "Testing concurrent requests to $SERVER..."
echo ""

# Launch 5 requests in background, showing timing
for i in {1..5}; do
    (
        echo "[Request $i] Starting at $(date +%s.%N)"
        curl -s "http://$SERVER/index.html" | head -c 50
        echo ""
        echo "[Request $i] Completed at $(date +%s.%N)"
    ) &
done

echo "Waiting for all requests to complete..."
wait
echo "All requests completed!"
