#!/bin/bash

echo "=== Cache Performance Test ==="
echo ""

echo "1. Cache Miss (first request)"
time curl -s "http://localhost:8000/home.html" -x http://localhost:8080 -o /dev/null

echo ""
echo "2. Cache Hit (second request, same file)"
time curl -s "http://localhost:8000/home.html" -x http://localhost:8080 -o /dev/null

echo ""
echo "3. Different file (cache miss)"
time curl -s "http://localhost:8000/tiny.c" -x http://localhost:8080 -o /dev/null

echo ""
echo "4. Back to first file (should be cache hit)"
time curl -s "http://localhost:8000/home.html" -x http://localhost:8080 -o /dev/null

echo ""
echo "=== Test Complete ==="
echo "Cache hits should be ~10-50x faster than misses"