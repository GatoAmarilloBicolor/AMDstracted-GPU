#!/bin/bash
# Build script for agnóstic GPU driver
# Usage: ./build_agnosis.sh [linux|haiku|freebsd|all]

set -e

if [ -z "$1" ]; then
    echo "Usage: ./build_agnosis.sh [linux|haiku|freebsd|all]"
    echo ""
    echo "Examples:"
    echo "  ./build_agnosis.sh linux      # Build for Linux"
    echo "  ./build_agnosis.sh haiku      # Build for Haiku"
    echo "  ./build_agnosis.sh freebsd    # Build for FreeBSD"
    echo "  ./build_agnosis.sh all        # Build for all platforms"
    exit 1
fi

TARGET="$1"
RESULTS_FILE="build_results.txt"

echo "════════════════════════════════════════════════════════════════"
echo "🏗️  AMD GPU Driver - Agnóstic Build Script"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Function to build for a specific OS
build_for_os() {
    local os=$1
    echo "────────────────────────────────────────────────────────────────"
    echo "📦 Building for $os..."
    echo "────────────────────────────────────────────────────────────────"
    
    if make OS=$os 2>&1 | tee -a "$RESULTS_FILE"; then
        echo "✅ $os build SUCCESS"
        echo "[✅] $os: SUCCESS" >> "$RESULTS_FILE"
    else
        echo "❌ $os build FAILED"
        echo "[❌] $os: FAILED" >> "$RESULTS_FILE"
        return 1
    fi
    echo ""
}

# Clear previous results
> "$RESULTS_FILE"

# Build for specified target(s)
case "$TARGET" in
    linux)
        build_for_os linux
        ;;
    haiku)
        build_for_os haiku
        ;;
    freebsd)
        build_for_os freebsd
        ;;
    all)
        build_for_os linux
        build_for_os haiku
        build_for_os freebsd
        ;;
    *)
        echo "❌ Unknown target: $TARGET"
        exit 1
        ;;
esac

echo "════════════════════════════════════════════════════════════════"
echo "📋 Build Summary:"
echo "════════════════════════════════════════════════════════════════"
cat "$RESULTS_FILE"
echo "════════════════════════════════════════════════════════════════"
echo ""
echo "✅ All builds completed!"
