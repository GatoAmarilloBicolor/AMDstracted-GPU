#!/bin/bash

# Yo! This is the HIT Universal Build Script.
# It detects your OS and helps you build the AMDGPU_Abstracted driver properly.

echo "🚀 Starting the HIT Edition build process..."

# 1. Detect OS
OS_NAME=$(uname -s | tr '[:upper:]' '[:lower:]')
echo "🌍 Detected System: $OS_NAME"

# 2. Set Build Mode
# We recommend USERLAND_MODE=1 for testing and development.
# If you want real kernel access, set it to 0.
USER_MODE=${USERLAND_MODE:-1}
echo "🔧 Build Mode: USERLAND_MODE=$USER_MODE"

if [ "$USER_MODE" -eq 1 ]; then
    echo "💡 Note: You are building in SIMULATION mode. Great for testing without crashes!"
else
    echo "⚠️ Note: You are building in KERNEL mode. This needs real hardware headers!"
fi

# 3. Clean previous builds
echo "🧹 Cleaning up old files..."
make clean > /dev/null 2>&1

# 4. Build based on OS
case "$OS_NAME" in
    linux)
        echo "🐧 Optimizing for Linux..."
        make OS=linux USERLAND_MODE=$USER_MODE rmapi_server rmapi_client_demo
        ;;
    haiku)
        echo "🌀 Optimizing for Haiku OS..."
        # On Haiku, we ensure we use the right compiler and library flags
        make OS=haiku USERLAND_MODE=$USER_MODE rmapi_server rmapi_client_demo
        ;;
    *)
        echo "🌍 System not officially optimized, but trying with Linux fallback..."
        make OS=linux USERLAND_MODE=$USER_MODE rmapi_server rmapi_client_demo
        ;;
esac

# 5. Check if it worked
if [ $? -eq 0 ]; then
    echo "✅ Success! Everything is built and ready to go."
    echo "----------------------------------------------------"
    echo "To start the driver, run: ./rmapi_server &"
    echo "To run the demo, run:     ./rmapi_client_demo"
    echo "----------------------------------------------------"
else
    echo "❌ Aw man, something went wrong during the build."
    echo "Check the errors above. If you're on Haiku, make sure you have the Haiku SDK installed!"
fi
