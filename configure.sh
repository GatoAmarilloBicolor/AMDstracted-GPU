#!/bin/bash

# Configure script for AMD GPU Driver
# Detects OS and generates config.h

echo "Running configure script..."

# Detect OS
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
echo "Detected OS: $OS"

# Generate config.h
cat > config.h << EOF
#ifndef CONFIG_H
#define CONFIG_H

// OS Detection
EOF

case $OS in
    linux)
        echo "#define OS_LINUX 1" >> config.h
        ;;
    freebsd)
        echo "#define OS_FREEBSD 1" >> config.h
        ;;
    haiku)
        echo "#define OS_HAIKU 1" >> config.h
        ;;
    darwin)
        echo "#define OS_DARWIN 1" >> config.h
        ;;
    *)
        echo "#define OS_UNKNOWN 1" >> config.h
        ;;
esac

echo "#endif // CONFIG_H" >> config.h

echo "Generated config.h"
echo "Run 'make' to build"