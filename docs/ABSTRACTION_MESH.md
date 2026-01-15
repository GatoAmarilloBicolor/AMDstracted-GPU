# 🧶 The HIT Abstraction Mesh: Future-Proofing the Driver

So you want to know how we keep the **AMDGPU_Abstracted** driver updated when AMD releases new code? We've built a "Mesh"—a special way to catch new changes and translate them into our portable format without breaking everything.

---

## 1. The Strategy: "Shim & Trace"

Instead of changing the original AMD code directly, we use **Shims**. 
- A **Shim** is a tiny piece of code that sits between the "New AMD Logic" and our "OS Primitives".
- If AMD updates a function in `drm-kmod`, we don't rewrite it. We just make sure our Shim still understands the instructions.

### 🗺 The Mapping Table
To make this easy for anyone, we follow this simple map:

| Original AMD Type (`drm-kmod`) | HIT Mesh Translation | Why? |
| :--- | :--- | :--- |
| `kmalloc()` | `os_prim_alloc()` | Portable memory allocation. |
| `kfree()` | `os_prim_free()` | Safe cleanup. |
| `WREG32()` | `os_prim_write32()` | Writing to registers in a safe bubble. |
| `RREG32()` | `os_prim_read32()` | Reading from registers. |
| `struct drm_device` | `struct OBJGPU` | Our cleaner, lighter GPU container. |

---

## 2. Automating the Updates (The "Pull" Plan)

To update the driver, we follow a 3-step automated process:

### Step A: The Pull
We download the latest `drm-kmod` source into a separate "Vendor" folder.
```bash
./tools/pull_updates.sh --version v6.x
```

### Step B: The Diff
Our script compares the new code with our "Abstraction Mesh". 
- If a new engine (like a new Video Encoder) is added, the script identifies it and creates a new **IP Block** template for us.

### Step C: The Integration
We wrap the new logic inside our IP Block structure:
1. Create a `naviXX_new_engine_hw_init` stub.
2. Copy the register logic from the original source.
3. Replace all kernel calls with HIT Shims.

---

## 3. How to add new OS support easily

Want to support an emerging OS (like **SerenityOS** or **Redox**)? 
1. Create a new folder in `kernel-amd/os-primitives/your_os/`.
2. Implement the `os_prim_` functions using Your OS calls.
3. The **Abstraction Mesh** will do the rest! It will connect your new OS "Body" to the GPU "Brain" automatically.

---

## 🚀 The Vision
By keeping the hardware logic separate from the OS glue, we've created a driver that is *immortal*. As long as we keep our Shims updated, the **Haiku Imposible Team** will always have the latest AMD features!

*Built to last, built to be HIT.*
**- The Haiku Imposible Team**
