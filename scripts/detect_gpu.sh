#!/bin/bash
# GPU Detection Script - Identifies AMD hardware and selects appropriate driver
# Returns: GPU_DRIVER (r300, r600, radeonsi, or unknown)

if ! command -v lspci >/dev/null 2>&1; then
    echo "unknown"
    exit 0
fi

GPU_INFO=$(lspci 2>/dev/null | grep -i "vga\|display" | grep -i "amd\|ati\|radeon")

if [ -z "$GPU_INFO" ]; then
    echo "unknown"
    exit 0
fi

# Detect GPU model from lspci output
if echo "$GPU_INFO" | grep -qi "radeon\|ati.*7[0-9][0-9][0-9]\|radeon.*7[0-9][0-9][0-9]\|wrestler"; then
    # Radeon HD 7000 series (Warrior/GCN Gen 1) - use R600
    echo "r600"
elif echo "$GPU_INFO" | grep -qi "radeon.*6[0-9][0-9][0-9]\|radeon.*5[0-9][0-9][0-9]\|radeon hd [56][0-9][0-9][0-9]"; then
    # Radeon HD 5000/6000 series - use R600
    echo "r600"
elif echo "$GPU_INFO" | grep -qi "r300\|radeon 9[0-9][0-9][0-9]\|radeon x[0-9][0-9][0-9]\|radeon 8[0-9][0-9][0-9]"; then
    # Radeon R300/R400/R500 series
    echo "r300"
elif echo "$GPU_INFO" | grep -qi "vega\|navi\|rdna\|radeon rx\|radeon pro"; then
    # RDNA/RDNA2/RDNA3 GPUs (Vega, Navi, RDNA)
    echo "radeonsi"
elif echo "$GPU_INFO" | grep -qi "radeon"; then
    # Generic Radeon - default to r600
    echo "r600"
else
    echo "unknown"
fi
