# ⚔️ AMDGPU_Abstracted vs. nvidia-haiku: The Showdown!

Yo! If you're wondering how our **HIT Edition** driver stacks up against the pro **nvidia-haiku** driver, you're in the right place. We've used exactly the same high-level architecture, but tailored it for AMD cards.

---

## 🥊 The Comparison Table

| Feature | nvidia-haiku (The Pro) | AMDGPU_Abstracted (The New Kid) |
| :--- | :--- | :--- |
| **Core Architecture** | NVIDIA Resource Manager (RM) | HIT Modular HAL + RMAPI |
| **Haiku Integration** | Full Accelerant & Kernel Addon | Stubs for Accelerant & Addon |
| **RMAPI Model** | Proprietary NVIDIA NvRm API | Open HIT RMAPI (Inpired by NVIDIA) |
| **Vulkan/OpenGL** | Custom NVIDIA-specific hooks | Zink (OpenGL-on-Vulkan) focus |
| **Resource Tracking** | Internal RM Handles | HIT **RESSERV** (Parent-Child Tree) |

---

## 🏗 What's the status for Haiku?

Right now, our driver is like a high-performance engine sitting on a workbench. It runs perfectly in userland (Linux/Haiku), but it hasn't been "bolted onto the car" (the Haiku kernel) yet.

### 🧩 What we HAVE:
1. **The Brain (HAL/RMAPI)**: Our core logic is light-years ahead. It already understands how to talk to virtual AMD GPUs and manage memory.
2. **The IPC Subway**: We have a working socket system that could easily be converted into Haiku's `ioctl` or `port` system.
3. **The Folder Structure**: We already have `haiku-amd/` with the right spots for the Accelerant and the Addon.

### ❌ What we MISS (The "To-Do" List):
1. **The ioctl Bridge**: In Haiku, the Accelerant (userland) talks to the Addon (kernel) using `ioctl`. We need to map our `rmapi_` calls into kernel `ioctl` codes.
2. **Real PCI Mapping**: Currently, we use `os_prim_alloc` (simulated). We need to use `create_area` and `map_physical_memory` in Haiku to talk to real AMD chips.
3. **Interrupt Hub (IH)**: Haiku uses specific interrupt handlers. We need to bridge our internal `IH` block to Haiku's `install_io_interrupt_handler`.

---

## 🛠 How to use it on Haiku (Right Now!)

You can't use it as a display driver *yet*, but you can run it as a high-performance compute system or a simulator!

1. **Build it on Haiku**:
   ```bash
   make OS=haiku
   ```
2. **Start the "Virtual Driver"**:
   ```bash
   ./rmapi_server &
   ```
3. **Talk to it**: Your app can now use `rmapi_` calls to "allocate" memory in Haiku. The driver will use Haiku's system calls to give you space.

---

## 🎢 The Roadster to Full Integration

To make it as easy to use as `nvidia-haiku`, here is the plan:
1. **Step 1**: Complete `AmdAddon.cpp` to scan for real AMD PCI IDs.
2. **Step 2**: Create the `/dev/graphics/amdgpu` device node in the kernel.
3. **Step 3**: Point the Haiku `AmdAccelerant.cpp` to that device node.
4. **Step 4**: Launch **App Server Neo** and enjoy the HIT-powered graphics!

*We're building the future of Haiku graphics, one specialist at a time!*
**- The Haiku Imposible Team**
