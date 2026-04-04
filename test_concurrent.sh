#!/bin/bash
# test_concurrent.sh
PROXY_PORT=$1
TINY_PORT=${2:-8000}  # Default to 8000 if not specified

if [ -z "$PROXY_PORT" ]; then
    echo "Usage: $0 <proxy_port> [tiny_port]"
    exit 1
fi

echo "Testing concurrent requests..."
echo "Proxy: localhost:$PROXY_PORT"
echo "Tiny Server: localhost:$TINY_PORT"
echo "Launching 5 concurrent requests..."

# Launch 5 requests in background, all roughly simultaneous
for i in {1..5}; do
    echo "  Request $i..." 
    curl -s "http://localhost:$TINY_PORT/index.html" -x http://localhost:$PROXY_PORT -o /dev/null &
done
wait
echo "All requests completed!"