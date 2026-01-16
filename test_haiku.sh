#!/bin/bash

# 🛡️ test_haiku.sh - The Ultimate Sanity Check
# Automates the "Pull -> Build -> Run Brain -> Test Graphics" loop.

LOG_FILE="hit_test_results.log"

echo "==================================================" | tee $LOG_FILE
echo "🧪 AMDstracted-GPU: HIT Verification Suite" | tee -a $LOG_FILE
echo "==================================================" | tee -a $LOG_FILE

# 1. Clean & Rebuild
echo "[1/4] 🔨 Rebuilding Driver Stack..." | tee -a $LOG_FILE
./install_haiku.sh >> $LOG_FILE 2>&1
if [ $? -ne 0 ]; then
    echo "❌ BUILD FAILED! Check $LOG_FILE for details." | tee -a $LOG_FILE
    exit 1
fi
echo "✅ Build Successful." | tee -a $LOG_FILE

# 2. Restart Brain (rmapi_server)
echo "[2/4] 🧠 Restarting RMAPI Server (The Brain)..." | tee -a $LOG_FILE
pkill rmapi_server
sleep 1
./rmapi_server > /dev/null 2>&1 &
SERVER_PID=$!
sleep 1

if ps -p $SERVER_PID > /dev/null; then
    echo "✅ Brain is ALIVE (PID: $SERVER_PID)." | tee -a $LOG_FILE
else
    echo "❌ Brain DIED on startup! Check logs." | tee -a $LOG_FILE
    exit 1
fi

# 3. Verify Device Nodes
echo "[3/4] 🔍 Checking Device Nodes..." | tee -a $LOG_FILE
if [ -e /dev/graphics/amdgpu_hit ]; then
    echo "✅ /dev/graphics/amdgpu_hit FOUND." | tee -a $LOG_FILE
else
    echo "⚠️  /dev/graphics/amdgpu_hit MISSING. (Is the addon loaded?)" | tee -a $LOG_FILE
    # Try to force load via checkfs or just warn
fi

# 4. Test Graphics & Vulkan Stubs
echo "[4/4] 🎮 Running GLInfo (The Moment of Truth)..." | tee -a $LOG_FILE
GL_OUTPUT=$(GLInfo 2>&1)
echo "$GL_OUTPUT" >> $LOG_FILE

if echo "$GL_OUTPUT" | grep -q "AMDGPU Abstracted"; then
    echo "✅ SUCCESS: GLInfo sees 'AMDGPU Abstracted'!" | tee -a $LOG_FILE
    echo "🎉 The Driver is WORKING." | tee -a $LOG_FILE
elif echo "$GL_OUTPUT" | grep -q "llvmpipe"; then
    echo "⚠️  WARNING: GLInfo still sees 'llvmpipe' (Software Rasterizer)." | tee -a $LOG_FILE
    echo "   -> Did the Accelerant clone handle fail?" | tee -a $LOG_FILE
else
    echo "❓ UNKNOWN STATE: Check GLInfo output manually." | tee -a $LOG_FILE
fi

# Summary
echo "==================================================" | tee -a $LOG_FILE
echo "📝 Full log saved to $LOG_FILE"
echo "=================================================="
