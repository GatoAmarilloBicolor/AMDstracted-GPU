#include "../core/rmapi/rmapi.h"
#include "../core/ipc/ipc_lib.h"
#include "../core/ipc/ipc_protocol.h"
#include "../core/hal/hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 🌀 HIT Edition: RMAPI Client Demo
 *
 * This little app demonstrates how to talk to our consolidated
 * RMAPI server using the new HIT Subway signals (IPC protocol).
 */

// Helper function to get ASIC type name
const char* get_asic_type_name(enum amd_asic_type asic_type) {
    switch (asic_type) {
        case AMD_ASIC_WRESTLER: return "Wrestler (APU)";
        case AMD_ASIC_NAVI10: return "Navi10 (RDNA)";
        case AMD_ASIC_R600: return "R600 (Legacy HD 2000-4000)";
        case AMD_ASIC_EVERGREEN: return "Evergreen (Legacy HD 5000-6000)";
        case AMD_ASIC_NI: return "Northern Islands (Legacy HD 6000-7000)";
        default: return "Unknown";
    }
}

// Helper function to determine GPU architecture
const char* get_gpu_architecture(uint32_t family, enum amd_asic_type asic_type) {
    // Modern RDNA GPUs
    if (asic_type == AMD_ASIC_NAVI10) {
        return "RDNA (Ray Tracing, Mesh Shaders, VRS)";
    }

    // GCN-based GPUs (family-based detection)
    if (family >= 0x60 && family <= 0x7F) {
        return "GCN 1.0 (Volcanic Islands HD 7000)";
    } else if (family >= 0x80 && family <= 0x8F) {
        return "GCN 1.1 (Polaris HD 400/500)";
    } else if (family >= 0x90 && family <= 0x9F) {
        return "GCN 1.2 (Vega)";
    } else if (family >= 0xA0 && family <= 0xAF) {
        return "GCN 1.3 (Navi pre-RDNA)";
    }

    // Legacy VLIW architectures
    if (family >= 0x40 && family <= 0x5F) {
        return "VLIW (TeraScale HD 5000-6000)";
    } else if (family >= 0x20 && family <= 0x3F) {
        return "VLIW (TeraScale HD 2000-4000)";
    }

    return "Unknown Architecture";
}

// Helper function to get shader model
const char* get_shader_model(uint32_t family) {
    if (family >= 0x60) {
        return "Shader Model 6.0+ (DX12/Vulkan)";
    } else if (family >= 0x50) {
        return "Shader Model 5.1 (DX11)";
    } else if (family >= 0x40) {
        return "Shader Model 5.0 (DX11)";
    } else {
        return "Shader Model 4.0 (DX10)";
    }
}

// Helper function to get compute units estimate
uint32_t estimate_compute_units(uint32_t device_id) {
    // This is a rough estimate based on known GPU configurations
    // In a real driver, this would come from hardware registers

    uint32_t dev_id = device_id & 0xFFFF;

    // Navi10/20/30 series (RDNA)
    if (dev_id >= 0x7310 && dev_id <= 0x73FF) {
        if (dev_id == 0x7310) return 40; // RX 5700 XT
        if (dev_id == 0x7312) return 36; // RX 5600 XT
        if (dev_id == 0x7318) return 32; // RX 5700
        if (dev_id == 0x731F) return 36; // RX 5600 XT
        return 32; // Other Navi10
    }

    // Navi21 (RDNA2)
    if (dev_id >= 0x73A0 && dev_id <= 0x73FF) {
        if (dev_id == 0x73A0) return 32; // RX 6800
        if (dev_id == 0x73A2) return 36; // RX 6800 XT
        if (dev_id == 0x73A4) return 40; // RX 6900 XT
        return 32;
    }

    // Vega series (GCN 1.2)
    if (dev_id >= 0x6860 && dev_id <= 0x687F) {
        if (dev_id == 0x6860) return 64; // Vega 64
        if (dev_id == 0x6861) return 56; // Vega 56
        if (dev_id == 0x6862) return 64; // Vega Frontier
        if (dev_id == 0x6863) return 64; // Radeon Pro WX 9100
        return 60; // Other Vega
    }

    // Polaris series (GCN 1.1)
    if (dev_id >= 0x67C0 && dev_id <= 0x67FF) {
        if (dev_id == 0x67DF) return 32; // RX 580
        if (dev_id == 0x67C7) return 28; // RX 570
        if (dev_id == 0x67C4) return 20; // RX 560
        if (dev_id == 0x67C1) return 16; // RX 550
        return 24; // Other Polaris
    }

    // Fiji (GCN 1.2)
    if (dev_id >= 0x7300 && dev_id <= 0x73FF && dev_id < 0x7310) {
        return 64; // Fiji (R9 Fury series)
    }

    return 0; // Unknown
}

// Helper function to get GPU generation
const char* get_gpu_generation(uint32_t family, enum amd_asic_type asic_type) {
    if (asic_type == AMD_ASIC_NAVI10) {
        return "Navi (RDNA 1.0 - 2019)";
    }

    switch (family) {
        case 0x80: case 0x81: return "Polaris (GCN 1.1 - 2016)";
        case 0x90: case 0x91: return "Vega (GCN 1.2 - 2017)";
        case 0xA0: case 0xA1: return "Navi Pre-RDNA (GCN 1.3 - 2018)";
        case 0x60: case 0x61: return "Volcanic Islands (GCN 1.0 - 2015)";
        case 0x70: case 0x71: return "Sea Islands (GCN 1.0 - 2014)";
        case 0x50: case 0x51: return "Southern Islands (GCN - 2012)";
        case 0x40: case 0x41: return "Northern Islands (VLIW - 2011)";
        case 0x30: case 0x31: return "Evergreen (VLIW - 2010)";
        case 0x20: case 0x21: return "R600/R700 (VLIW - 2007-2009)";
        default: return "Unknown Generation";
    }
}

int main() {
  printf("🌀 HIT Client: Connecting to the GPU Subway (%s)...\n",
         HIT_SOCKET_PATH);

  ipc_connection_t conn;
  if (ipc_client_connect(HIT_SOCKET_PATH, &conn) < 0) {
    perror("❌ Connection failed! Is the amd_rmapi_server running?");
    return 1;
  }
  printf("✅ Connected to the Driver Brain!\n");

  // 1. Request GPU Info
  printf("📡 Sending Request: GET_GPU_INFO...\n");
  ipc_message_t msg = {IPC_REQ_GET_GPU_INFO, 1, 0, NULL};
  if (ipc_send_message(&conn, &msg) < 0) {
    printf("❌ Failed to send request.\n");
    ipc_close(&conn);
    return 1;
  }

  // 2. Wait for Reply
  ipc_message_t resp;
  if (ipc_recv_message(&conn, &resp) > 0) {
    if (resp.type == IPC_REP_GET_GPU_INFO) {
      struct amdgpu_gpu_info *info = (struct amdgpu_gpu_info *)resp.data;

      // Calculate additional info
      uint32_t compute_units = estimate_compute_units(info->device_id);
      const char* asic_name = get_asic_type_name(info->asic_type);
      const char* architecture = get_gpu_architecture(info->family, info->asic_type);
      const char* shader_model = get_shader_model(info->family);
      const char* generation = get_gpu_generation(info->family, info->asic_type);

      printf("╔══════════════════════════════════════════════════════════════╗\n");
      printf("║                    🎮 GPU TECHNICAL SPECIFICATIONS                   ║\n");
      printf("╠══════════════════════════════════════════════════════════════╣\n");
      printf("║ GPU Name:           %-42s ║\n", info->gpu_name);
      printf("║ Device ID:          0x%08X (%-30s) ║\n", info->device_id, asic_name);
      printf("║ ASIC Family:        0x%02X%-37s ║\n", info->family, "");
      printf("║ ASIC Type:          %-42s ║\n", asic_name);
      printf("║ Generation:         %-42s ║\n", generation);
      printf("║ Architecture:       %-42s ║\n", architecture);
      printf("║ Shader Model:       %-42s ║\n", shader_model);
      printf("║ VRAM Size:          %-4u MB%-34s ║\n", info->vram_size_mb, "");
      printf("║ GPU Clock:          %-4u MHz%-34s ║\n", info->gpu_clock_mhz, "");

      if (compute_units > 0) {
        printf("║ Compute Units:      %-4u %-34s ║\n", compute_units, "(estimated)");
      }

      printf("║ VRAM Base Address:  0x%016lX%-22s ║\n", info->vram_base, "");
      printf("╚══════════════════════════════════════════════════════════════╝\n");

      // Additional technical details
      printf("\n📊 TECHNICAL ANALYSIS:\n");

      // Architecture-specific features
      printf("\n🔧 ARCHITECTURE FEATURES:\n");
      if (strstr(architecture, "RDNA") != NULL) {
        printf("✅ Ray Tracing: Hardware accelerated (RT cores)\n");
        printf("✅ Mesh Shaders: Yes (Hardware support)\n");
        printf("✅ Variable Rate Shading: Yes (VRS)\n");
        printf("✅ Sampler Feedback: Yes\n");
        printf("✅ Async Compute: Enhanced\n");
        printf("✅ Video Core: VCN 3.0+ (Hardware decode/encode)\n");
        printf("✅ Display Engine: DCN 2.0+ (Advanced scaling)\n");
      } else if (strstr(architecture, "GCN") != NULL) {
        printf("✅ GCN Architecture: Unified shader design\n");
        printf("✅ Async Compute: Yes (Hardware queues)\n");
        printf("✅ HSA Support: Heterogeneous System Architecture\n");
        printf("✅ Mantle API: Precursor to Vulkan\n");
        printf("⚠️  Ray Tracing: Software emulation only\n");
        printf("✅ Video Core: VCE/UVD (Hardware decode)\n");
        printf("✅ Display Engine: DCE (Unified display)\n");
      } else if (strstr(architecture, "VLIW") != NULL) {
        printf("⚠️  Legacy VLIW Architecture: Vector processors\n");
        printf("⚠️  Limited async compute capabilities\n");
        printf("✅ DX11/OpenGL 4.x: Full feature support\n");
        printf("⚠️  No modern APIs: Vulkan/DX12 limited\n");
        printf("✅ Video Core: UVD (Hardware decode)\n");
        printf("✅ Display Engine: DCE (Basic display)\n");
      }

      // Compute capabilities
      printf("\n⚡ COMPUTE CAPABILITIES:\n");
      if (compute_units > 0) {
        printf("• Compute Units: %u\n", compute_units);
        printf("• Stream Processors: ~%u (%u per CU)\n", compute_units * 64, 64);
        printf("• Peak TFLOPS (FP32): ~%.1f TFLOPS\n", (compute_units * 64 * info->gpu_clock_mhz * 2) / 1000000.0);
      } else {
        printf("• Compute Units: Unknown (legacy GPU)\n");
      }

      // Memory bandwidth estimate
      float memory_bw = (info->vram_size_mb > 0) ? (info->gpu_clock_mhz * 256.0f / 1000.0f) : 0; // Rough estimate
      if (memory_bw > 0) {
        printf("• Memory Bandwidth: ~%.1f GB/s (estimated)\n", memory_bw);
      }

      // Memory analysis
      printf("\n💾 MEMORY ANALYSIS:\n");
      if (info->vram_size_mb >= 16384) {
        printf("✅ Professional GPU: 16GB+ VRAM (4K content creation, ML/AI)\n");
      } else if (info->vram_size_mb >= 8192) {
        printf("✅ High-End GPU: 8GB+ VRAM (4K gaming, professional workloads)\n");
      } else if (info->vram_size_mb >= 4096) {
        printf("✅ Gaming GPU: 4-8GB VRAM (1440p/4K gaming capable)\n");
      } else if (info->vram_size_mb >= 2048) {
        printf("⚠️  Entry-Level: 2-4GB VRAM (1080p gaming, light content creation)\n");
      } else {
        printf("⚠️  Basic GPU: <2GB VRAM (1080p gaming, basic tasks)\n");
      }

      // Memory bus width estimate (rough)
      const char* mem_bus = "Unknown";
      if (strstr(info->gpu_name, "RX 6900") || strstr(info->gpu_name, "RX 6800")) {
        mem_bus = "256-bit GDDR6";
      } else if (strstr(info->gpu_name, "RX 6700") || strstr(info->gpu_name, "RX 6600")) {
        mem_bus = "128-bit GDDR6";
      } else if (strstr(info->gpu_name, "RX 5700") || strstr(info->gpu_name, "RX 5600")) {
        mem_bus = "256-bit GDDR6";
      } else if (strstr(info->gpu_name, "Vega 64") || strstr(info->gpu_name, "RX 580")) {
        mem_bus = "2048-bit HBM2/GDDR5";
      }
      printf("• Memory Bus: %s\n", mem_bus);

      // API Support
      printf("\n🎯 API SUPPORT:\n");
      if (strstr(architecture, "RDNA") != NULL) {
        printf("✅ Vulkan: 1.0+ (Full hardware support)\n");
        printf("✅ DirectX: 12 Ultimate (Feature complete)\n");
        printf("✅ OpenGL: 4.6+ (Compatibility)\n");
        printf("✅ OpenCL: 2.0+ (Compute)\n");
      } else if (strstr(architecture, "GCN") != NULL) {
        printf("✅ Vulkan: 1.0+ (Full support)\n");
        printf("✅ DirectX: 12 (Feature complete)\n");
        printf("✅ OpenGL: 4.5+ (Compatibility)\n");
        printf("✅ OpenCL: 1.2+ (Compute)\n");
      } else {
        printf("⚠️  Vulkan: Limited support\n");
        printf("✅ DirectX: 11 (Full support)\n");
        printf("✅ OpenGL: 4.1+ (Compatibility)\n");
        printf("✅ OpenCL: 1.1+ (Basic compute)\n");
      }

      // Performance estimate
      printf("\n🚀 PERFORMANCE CLASS:\n");
      if (info->gpu_clock_mhz >= 2500) {
        printf("✅ Enthusiast/Professional: %u MHz+ (High-end gaming, content creation)\n", info->gpu_clock_mhz);
      } else if (info->gpu_clock_mhz >= 2000) {
        printf("✅ High Performance: %u MHz+ (4K gaming, professional workloads)\n", info->gpu_clock_mhz);
      } else if (info->gpu_clock_mhz >= 1500) {
        printf("✅ Good Performance: %u MHz (1440p gaming, content creation)\n", info->gpu_clock_mhz);
      } else if (info->gpu_clock_mhz >= 1000) {
        printf("⚠️  Moderate Performance: %u MHz (1080p gaming, light workloads)\n", info->gpu_clock_mhz);
      } else {
        printf("⚠️  Entry Level: %u MHz (Basic computing tasks)\n", info->gpu_clock_mhz);
      }

      // Usage recommendations
      printf("\n💡 RECOMMENDED USAGE:\n");
      if (strstr(architecture, "RDNA") != NULL && info->vram_size_mb >= 4096) {
        printf("🎮 AAA Gaming (4K/1440p), Real-time Ray Tracing\n");
        printf("🎨 Professional Content Creation (Blender, DaVinci Resolve)\n");
        printf("⚡ Machine Learning, AI workloads\n");
        printf("🎬 Video Editing, 3D Rendering\n");
      } else if (strstr(architecture, "GCN") != NULL && info->vram_size_mb >= 2048) {
        printf("🎮 AAA Gaming (1440p/1080p), eSports\n");
        printf("🎨 Content Creation (Lightroom, Premiere)\n");
        printf("⚡ Compute workloads, Cryptocurrency mining\n");
        printf("🎬 Video editing, streaming\n");
      } else if (strstr(architecture, "VLIW") != NULL) {
        printf("🎮 Older games, eSports (1080p)\n");
        printf("🎨 Basic photo editing, video playback\n");
        printf("💼 Office work, web browsing\n");
        printf("🎬 Basic video editing\n");
      } else {
        printf("💻 Basic computing, 2D graphics\n");
        printf("🌐 Web browsing, office applications\n");
        printf("📺 Video playback, media consumption\n");
      }

      free(resp.data);
    } else {
      printf("⚠️ Received unexpected message type: %u\n", resp.type);
    }
  }

  printf("👋 Closing connection. HIT Edition rules!\n");
  ipc_close(&conn);
  return 0;
}