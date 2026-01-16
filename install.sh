#!/bin/bash

# 🏁 HIT Edition: Universal Installer
# Auto-detects OS and runs appropriate installation script
# Supports: Linux, Haiku, FreeBSD, and others
# Developed by: Haiku Imposible Team (HIT)

echo "════════════════════════════════════════════════════════════════"
echo "🚀 HIT Universal Installer - Auto-Detection"
echo "════════════════════════════════════════════════════════════════"
echo ""

# Detect OS
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
OS_RELEASE=$(uname -r)

echo "📊 Detected System:"
echo "  OS:      $(uname -s)"
echo "  Release: $OS_RELEASE"
echo "  Kernel:  $(uname -m)"
echo ""

# Route to appropriate installer
case "$OS" in
    linux)
        echo "📍 Detected: Linux"
        echo "Running Linux installer..."
        echo ""
        chmod +x install_linux.sh
        ./install_linux.sh
        ;;
    haiku)
        echo "📍 Detected: Haiku"
        echo "Running Haiku installer..."
        echo ""
        chmod +x install_haiku.sh
        ./install_haiku.sh
        ;;
    freebsd)
        echo "📍 Detected: FreeBSD"
        echo "Running FreeBSD installer..."
        echo ""
        chmod +x install_freebsd.sh
        ./install_freebsd.sh
        ;;
    darwin)
        echo "📍 Detected: macOS (Darwin)"
        echo "ℹ️  macOS support is partial. Using Linux-compatible build."
        echo "Running Linux installer (compatibility mode)..."
        echo ""
        chmod +x install_linux.sh
        ./install_linux.sh
        ;;
    redox)
        echo "📍 Detected: Redox OS"
        echo "Running Linux installer (compatibility mode)..."
        echo ""
        chmod +x install_linux.sh
        ./install_linux.sh
        ;;
    *)
        echo "❌ Unsupported OS: $OS"
        echo ""
        echo "Supported systems:"
        echo "  • Linux (any distro)"
        echo "  • Haiku"
        echo "  • FreeBSD"
        echo "  • macOS (partial, via Linux mode)"
        echo "  • Redox (partial, via Linux mode)"
        echo ""
        echo "To manually install, run:"
        echo "  ./install_linux.sh    (for POSIX-compatible systems)"
        echo "  ./install_haiku.sh    (for Haiku)"
        echo "  ./install_freebsd.sh  (for FreeBSD)"
        exit 1
        ;;
esac

EXIT_CODE=$?

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo "════════════════════════════════════════════════════════════════"
    echo "✅ Installation Complete!"
    echo "════════════════════════════════════════════════════════════════"
else
    echo "════════════════════════════════════════════════════════════════"
    echo "❌ Installation Failed"
    echo "════════════════════════════════════════════════════════════════"
    exit $EXIT_CODE
fi
