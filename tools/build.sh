#!/bin/bash
#
# Build script for AMDGPU_Abstracted
# Based on NVIDIA build patterns
# Supports: Linux, Haiku, FreeBSD
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Detect OS
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
ARCH=$(uname -m)

echo -e "${GREEN}════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}AMDGPU_Abstracted Build System${NC}"
echo -e "${GREEN}════════════════════════════════════════════════════════════${NC}"
echo ""
echo "System Information:"
echo "  OS: $OS"
echo "  Architecture: $ARCH"
echo ""

# Validate environment
check_requirements() {
    echo "Checking requirements..."
    
    if ! command -v gcc &> /dev/null; then
        echo -e "${RED}✗ gcc not found${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ gcc found${NC}"
    
    if ! command -v make &> /dev/null; then
        echo -e "${RED}✗ make not found${NC}"
        exit 1
    fi
    echo -e "${GREEN}✓ make found${NC}"
}

# Build components
build_components() {
    echo ""
    echo "Building components..."
    
    # Create build directory
    mkdir -p build/obj build/bin build/lib
    
    # Compile sources
    echo "  - Compiling shader compiler..."
    gcc -std=c99 -Wall -fPIC -c src/amd/shader_compiler/shader_compiler.c \
        -o build/obj/shader_compiler.o
    
    echo "  - Compiling RADV backend..."
    gcc -std=c99 -Wall -fPIC -c src/amd/radv_backend/radv_backend.c \
        -o build/obj/radv_backend.o
    
    echo "  - Compiling Zink layer..."
    gcc -std=c99 -Wall -fPIC -c src/amd/zink_layer/zink_layer.c \
        -o build/obj/zink_layer.o
    
    echo "  - Compiling HAL..."
    gcc -std=c99 -Wall -fPIC -c src/amd/hal/hal.c \
        -o build/obj/hal.o
    
    echo "  - Compiling IP blocks..."
    gcc -std=c99 -Wall -fPIC -c src/amd/ip_blocks/gmc_v10.c \
        -o build/obj/gmc_v10.o
    gcc -std=c99 -Wall -fPIC -c src/amd/ip_blocks/gfx_v10.c \
        -o build/obj/gfx_v10.o
    
    echo "  - Compiling IPC library..."
    gcc -std=c99 -Wall -fPIC -c src/common/ipc/ipc_lib.c \
        -o build/obj/ipc_lib.o
    
    echo "  - Compiling DRM shim..."
    gcc -std=c99 -Wall -fPIC -c drm/drm_shim.c \
        -o build/obj/drm_shim.o
    
    # Add OS-specific sources
    case "$OS" in
        linux)
            echo "  - Compiling Linux OS primitives..."
            gcc -std=c99 -Wall -fPIC -D__LINUX__ -c src/os/linux/os_primitives_linux.c \
                -o build/obj/os_primitives.o
            ;;
        haiku)
            echo "  - Compiling Haiku OS primitives..."
            gcc -std=c99 -Wall -fPIC -D__HAIKU__ -c src/os/haiku/os_primitives_haiku.c \
                -o build/obj/os_primitives.o
            ;;
        freebsd)
            echo "  - Compiling FreeBSD OS primitives..."
            gcc -std=c99 -Wall -fPIC -D__FreeBSD__ -c src/os/freebsd/os_primitives_freebsd.c \
                -o build/obj/os_primitives.o
            ;;
    esac
    
    echo -e "${GREEN}✓ Components compiled${NC}"
}

# Create library
create_library() {
    echo ""
    echo "Creating shared library..."
    
    OBJS="build/obj/shader_compiler.o build/obj/radv_backend.o build/obj/zink_layer.o"
    OBJS="$OBJS build/obj/hal.o build/obj/gmc_v10.o build/obj/gfx_v10.o"
    OBJS="$OBJS build/obj/ipc_lib.o build/obj/drm_shim.o build/obj/os_primitives.o"
    
    gcc -shared -o build/lib/libamdgpu.so $OBJS -lm -pthread
    cp build/lib/libamdgpu.so libamdgpu.so
    
    echo -e "${GREEN}✓ Library created: libamdgpu.so${NC}"
    ls -lh libamdgpu.so
}

# Run tests
run_tests() {
    echo ""
    echo "Running tests..."
    
    if [ ! -f tests/test_components ]; then
        echo "  - Building test suite..."
        gcc -std=c99 -Wall tests/test_components.c -o tests/test_components
    fi
    
    echo "  - Component tests..."
    ./tests/test_components 2>&1 | tail -10
    
    if [ ! -f tests/unit/test_memory_stress ]; then
        echo "  - Building memory stress tests..."
        gcc -std=c99 -Wall tests/unit/test_memory_stress.c -o tests/unit/test_memory_stress
    fi
    
    echo "  - Memory stress tests..."
    ./tests/unit/test_memory_stress 2>&1 | tail -10
    
    echo -e "${GREEN}✓ Tests completed${NC}"
}

# Display info
show_info() {
    echo ""
    echo "Build Information:"
    echo "  Build directory: $(pwd)/build"
    echo "  Library: libamdgpu.so"
    echo ""
    echo "Usage:"
    echo "  make              - Build using Makefile"
    echo "  make test         - Run all tests"
    echo "  make clean        - Clean build artifacts"
    echo ""
    echo "Run server:"
    echo "  ./rmapi_server &"
    echo ""
    echo "Run client:"
    echo "  ./rmapi_client_demo"
}

# Main flow
main() {
    check_requirements
    build_components
    create_library
    
    if [ "$1" == "--test" ]; then
        run_tests
    fi
    
    show_info
    
    echo -e "${GREEN}════════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}Build complete!${NC}"
    echo -e "${GREEN}════════════════════════════════════════════════════════════${NC}"
}

main "$@"
