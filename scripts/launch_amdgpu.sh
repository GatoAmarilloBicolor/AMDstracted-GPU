#!/bin/bash
# AMDGPU_Abstracted Launcher Script for Haiku
# This script sets up the environment and launches applications with AMD GPU acceleration

# Check OS
OS="$(uname -s)"
if [ "$OS" = "Haiku" ]; then
    AMD_GPU_BIN="/boot/home/config/non-packaged/bin"
    AMD_GPU_LIB="/boot/home/config/non-packaged/lib"
    # Haiku defaults to software rendering due to hardware access limitations
    DEFAULT_SOFTWARE=true
elif [ "$OS" = "Linux" ]; then
    AMD_GPU_BIN="$HOME/.local/bin"
    AMD_GPU_LIB="$HOME/.local/lib"
    DEFAULT_SOFTWARE=false
else
    echo "❌ Unsupported OS: $OS"
    echo "Supported: Haiku, Linux"
    exit 1
fi

# Function to check if server is running
check_server() {
    # Haiku doesn't have pgrep, use ps instead
    if ps | grep -q "amd_rmapi_server" | grep -v grep > /dev/null; then
        return 0
    else
        return 1
    fi
}

# Function to start server
start_server() {
    echo "🚀 Starting AMD RMAPI Server..."
    if [ "$DEBUG_MODE" = true ]; then
        echo "🐛 Debug: Binary: $AMD_GPU_BIN/amd_rmapi_server"
        echo "🐛 Debug: LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
        echo "🐛 Debug: Running server in foreground for 5 seconds..."
        timeout 5 "$AMD_GPU_BIN/amd_rmapi_server" 2>&1 || true
        echo "🐛 Debug: Server test complete. Now trying background start."
    fi

    # Try to run server in background directly first
    "$AMD_GPU_BIN/amd_rmapi_server" > /tmp/amd_rmapi.log 2>&1 &
    SERVER_PID=$!
    sleep 2
    
    if check_server; then
        echo "✅ Server started successfully (PID: $SERVER_PID)"
        return 0
    elif ps -p $SERVER_PID > /dev/null 2>&1; then
        # Process still running, may be initializing
        echo "✅ Server started successfully (PID: $SERVER_PID, initializing...)"
        return 0
    else
        # Process died, capture output for error reporting
        SERVER_OUTPUT=$(cat /tmp/amd_rmapi.log 2>/dev/null)
        SERVER_EXIT_CODE=$?
        
        if [ $SERVER_EXIT_CODE -ne 0 ] || [ -z "$SERVER_OUTPUT" ]; then
        echo "❌ Failed to start server (exit code: $SERVER_EXIT_CODE)"
        echo "📋 System Information:"
        echo "   OS: $(uname -s) $(uname -r)"
        echo "   User: $(whoami)"
        echo "   Working Dir: $(pwd)"
        echo "   Binary Path: $AMD_GPU_BIN/amd_rmapi_server"
        echo "   Library Path: $LD_LIBRARY_PATH"
        echo ""
            echo "📋 Full error details:"
            echo "----------------------------------------"
            echo "$SERVER_OUTPUT"
            echo "----------------------------------------"
        fi
        echo ""
        echo "💡 Troubleshooting tips:"
        echo "  - On Haiku: MMIO requires special permissions. Try running as root or check kernel driver."
        echo "  - On Linux: Hardware access needs root. Use 'sudo' or '--software' mode."
        echo "  - Try software mode: ./scripts/launch_amdgpu.sh launch --software <command>"
        echo "  - Check libraries: ld $AMD_GPU_BIN/amd_rmapi_server"
        echo "  - Enable debug: ./scripts/launch_amdgpu.sh launch --debug <command>"
        echo "  - Report issue with this full output at GitHub issues."
        return 1
    fi
}

# Function to stop server
stop_server() {
    if check_server; then
        echo "🛑 Stopping AMD RMAPI Server..."
        # Kill by process name
        ps | grep "amd_rmapi_server" | grep -v grep | awk '{print $1}' | xargs kill -9 2>/dev/null
        sleep 1
        echo "✅ Server stopped"
    else
        echo "ℹ️  Server not running"
    fi
}

# Function to setup environment
setup_environment() {
    echo "🔧 Setting up AMD GPU environment..."

    export LIBRARY_PATH="$AMD_GPU_LIB:$LIBRARY_PATH"
    export LD_LIBRARY_PATH="$AMD_GPU_LIB:$LD_LIBRARY_PATH"

    # Vulkan configuration
    if [ "$OS" = "Haiku" ]; then
        # Haiku Vulkan paths
        export VK_ICD_FILENAMES="/boot/home/config/non-packaged/share/vulkan/icd.d/radeon_icd.x86_64.json"
        export VK_DRIVER_FILES="/boot/home/config/non-packaged/share/vulkan/icd.d/radeon_icd*.json"
        export LIBGL_DRIVERS_PATH="/boot/home/config/non-packaged/lib/dri"
        
        # OpenGL configuration for Haiku
        export MESA_LOADER_DRIVER_OVERRIDE="zink"
        export LIBGL_DEBUG="verbose"
        
        # Add Haiku-specific Mesa libraries
        export LD_LIBRARY_PATH="/boot/home/config/non-packaged/lib:$LD_LIBRARY_PATH"
        
    elif [ "$OS" = "Linux" ]; then
        # Linux Vulkan paths
        export VK_ICD_FILENAMES="$AMD_GPU_LIB/vulkan/icd.d/radeon_icd.x86_64.json"
        export LIBGL_DRIVERS_PATH="$AMD_GPU_LIB/dri"
        export MESA_LOADER_DRIVER_OVERRIDE="zink"
    fi
    
    export VK_LOADER_DEBUG="error"
    
    # OpenGL Zink configuration (GL over Vulkan)
    export MESA_LOADER_DRIVER_OVERRIDE="zink"
    export LIBGL_ALWAYS_INDIRECT=1

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
    if [ -f "$AMD_GPU_LIB/libdrm_amdgpu.so" ]; then
        export LD_PRELOAD="$AMD_GPU_LIB/libdrm_amdgpu.so:$LD_PRELOAD"
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
            ps | grep "amd_rmapi_server" | grep -v grep
        else
            echo "❌ AMD RMAPI Server is not running"
        fi
        ;;
    "launch")
        shift  # Remove 'launch' from args
        # Check for flags
        SOFTWARE_MODE=$DEFAULT_SOFTWARE
        DEBUG_MODE=false
        while [ $# -gt 0 ]; do
            case "$1" in
                --software)
                    SOFTWARE_MODE=true
                    shift
                    ;;
                --debug)
                    DEBUG_MODE=true
                    shift
                    ;;
                *)
                    break
                    ;;
            esac
        done
        app_command="$*"
        setup_environment
        if [ "$SOFTWARE_MODE" = true ]; then
            echo "🔧 Software rendering mode enabled (CPU rendering)"
            export LIBGL_ALWAYS_SOFTWARE=1
            export GALLIUM_DRIVER=llvmpipe
        fi
        if [ "$DEBUG_MODE" = true ]; then
            echo "🐛 Debug mode enabled"
        fi
        if ! check_server; then
            if start_server; then
                echo "✅ Server started"
            else
                echo "⚠️  Server failed, forcing software mode..."
                export LIBGL_ALWAYS_SOFTWARE=1
                export GALLIUM_DRIVER=llvmpipe
                SOFTWARE_MODE=true
            fi
        fi
        launch_app "$app_command"
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
            if start_server; then
                echo "✅ Server started"
            else
                echo "⚠️  Server failed, running demo in software mode..."
                export LIBGL_ALWAYS_SOFTWARE=1
                export GALLIUM_DRIVER=llvmpipe
            fi
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
        echo "  $0 launch --software <cmd>  Launch with CPU software rendering"
        echo "  $0 launch --debug <cmd>     Launch with debug output"
        echo "  $0 env             Setup environment variables only"
        echo "  $0 test            Run the test suite"
        echo "  $0 demo            Run the demo client"
        echo "  $0 help            Show this help"
        echo ""
        echo "EXAMPLES:"
        echo "  $0 start"
        echo "  $0 launch 'glinfo'"
        echo "  $0 launch --software 'glinfo'  # Force software rendering"
        echo "  $0 launch 'vulkaninfo'"
        echo ""
        echo "REQUIREMENTS:"
        echo "  - Mesa with Zink and RADV: pkgman install mesa_devel"
        echo "  - Vulkan ICD: Ensure /boot/home/config/non-packaged/lib/vulkan/icd.d/radeon_icd.x86_64.json exists"
        echo "  - DRM library: libdrm_amdgpu.so in /boot/home/config/non-packaged/lib/"
        echo ""
        echo "TROUBLESHOOTING:"
        echo "  - If MMIO fails: Check PCI access permissions"
        echo "  - If GL/VK errors: Verify Mesa installation and ICD paths"
        echo "  - If symbol errors: Ensure all required libraries are installed"
        ;;
esac