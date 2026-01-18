#!/bin/bash
# AMDGPU_Abstracted Launcher Script for Haiku
# This script sets up the environment and launches applications with AMD GPU acceleration

# Check if we're on Haiku
if [ "$(uname -s)" != "Haiku" ]; then
    echo "❌ This script is designed for Haiku OS"
    echo "For Linux usage, see the project documentation"
    exit 1
fi

# Function to check if server is running
check_server() {
    if pgrep -f "amd_rmapi_server" > /dev/null; then
        return 0
    else
        return 1
    fi
}

# Function to start server
start_server() {
    echo "🚀 Starting AMD RMAPI Server..."
    /boot/home/config/non-packaged/bin/amd_rmapi_server &
    SERVER_PID=$!
    sleep 2

    if check_server; then
        echo "✅ Server started successfully (PID: $SERVER_PID)"
        return 0
    else
        echo "❌ Failed to start server"
        return 1
    fi
}

# Function to stop server
stop_server() {
    if check_server; then
        echo "🛑 Stopping AMD RMAPI Server..."
        pkill -f "amd_rmapi_server"
        sleep 1
        echo "✅ Server stopped"
    else
        echo "ℹ️  Server not running"
    fi
}

# Function to setup environment
setup_environment() {
    echo "🔧 Setting up AMD GPU environment..."

    # Haiku paths
    export AMD_GPU_BIN="/boot/home/config/non-packaged/bin"
    export AMD_GPU_LIB="/boot/home/config/non-packaged/lib"
    export LIBRARY_PATH="$AMD_GPU_LIB:$LIBRARY_PATH"
    export LD_LIBRARY_PATH="$AMD_GPU_LIB:$LD_LIBRARY_PATH"

    # Vulkan configuration
    export VK_ICD_FILENAMES="/boot/home/config/non-packaged/lib/vulkan/icd.d/radeon_icd.x86_64.json"
    export VK_LOADER_DEBUG="error"

    # OpenGL configuration (if Mesa is available)
    export LIBGL_DRIVERS_PATH="/boot/home/config/non-packaged/lib/dri"
    export MESA_LOADER_DRIVER_OVERRIDE="zink"

    # Add to PATH
    export PATH="$AMD_GPU_BIN:$PATH"

    echo "✅ Environment configured"
}

# Function to launch application
launch_app() {
    local app_command="$1"

    if [ -z "$app_command" ]; then
        echo "❌ No application specified"
        echo "Usage: $0 launch <application command>"
        return 1
    fi

    echo "🎮 Launching: $app_command"

    # Pre-load DRM shim if available
    if [ -f "/boot/home/config/non-packaged/lib/libdrm_amdgpu.so" ]; then
        export LD_PRELOAD="/boot/home/config/non-packaged/lib/libdrm_amdgpu.so:$LD_PRELOAD"
        echo "🔌 DRM shim loaded"
    fi

    # Launch the application
    eval "$app_command"
}

# Main script logic
case "$1" in
    "start")
        setup_environment
        if check_server; then
            echo "ℹ️  Server already running"
        else
            start_server
        fi
        ;;
    "stop")
        stop_server
        ;;
    "restart")
        stop_server
        sleep 1
        setup_environment
        start_server
        ;;
    "status")
        if check_server; then
            echo "✅ AMD RMAPI Server is running"
            pgrep -f "amd_rmapi_server"
        else
            echo "❌ AMD RMAPI Server is not running"
        fi
        ;;
    "launch")
        setup_environment
        if ! check_server; then
            start_server
        fi
        shift
        launch_app "$*"
        ;;
    "env")
        setup_environment
        echo "🔧 Environment variables set. Run your applications now."
        ;;
    "test")
        setup_environment
        if ! check_server; then
            start_server
        fi
        echo "🧪 Running AMD GPU tests..."
        amd_test_suite
        ;;
    "demo")
        setup_environment
        if ! check_server; then
            start_server
        fi
        echo "🎮 Running AMD GPU demo..."
        amd_rmapi_client_demo
        ;;
    "help"|*)
        echo "🎯 AMDGPU_Abstracted Launcher for Haiku"
        echo ""
        echo "USAGE:"
        echo "  $0 start           Start the AMD RMAPI server"
        echo "  $0 stop            Stop the AMD RMAPI server"
        echo "  $0 restart         Restart the AMD RMAPI server"
        echo "  $0 status          Check if server is running"
        echo "  $0 launch <cmd>    Launch application with AMD GPU acceleration"
        echo "  $0 env             Setup environment variables only"
        echo "  $0 test            Run the test suite"
        echo "  $0 demo            Run the demo client"
        echo "  $0 help            Show this help"
        echo ""
        echo "EXAMPLES:"
        echo "  $0 start"
        echo "  $0 launch 'glinfo'"
        echo "  $0 launch 'vulkaninfo'"
        echo "  $0 launch 'my_vulkan_app'"
        echo ""
        echo "NOTE: Make sure Mesa with Zink and RADV are installed for OpenGL/Vulkan support"
        ;;
esac