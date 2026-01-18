# 🌀 Haiku-AMD Driver: Bringing the Power to Haiku!

Yo! This is where the magic happens for **Haiku OS**. This directory contains the specific code needed to make AMD GPUs work natively on Haiku, and boy, is it cool!

We've designed this to be fully compatible with the way Haiku handles graphics, so your desktop stays smooth and your games look great.

---

## 🚀 What's inside?

- **Tracker Integration**: We've got the hooks needed for the Haiku Tracker (the file manager and desktop) to talk to the GPU like a boss.
- **Zink Support**: This is a super-fast way to run OpenGL apps on top of Vulkan. It's like giving your apps a nitro boost!
- **NVIDIA-style Logic**: We've used the same clever tricks found in the `nvidia-haiku` driver (like how it manages the PCI bus and interrupts) so it's reliable and well-tested.

---

## 🧩 Structure of the Haiku Side

- **`accelerant/`**: This is the "Gas Pedal" of the driver. It lives in userland and handles the high-speed math for apps like Tracker and Zink.
  - Files: `AmdAccelerant.cpp`, `ZinkAmd.cpp`, `TrackerAmd.cpp`.
- **`addon/`**: This is the "Link" to the kernel. It's a small piece of code that gives us access to the actual hardware registers when we need them.
  - File: `AmdAddon.cpp`.

---

## 🛠 Building the Haiku Version

On Haiku, we use the `jam` build system instead of `make`. 
1. Open your Terminal.
2. Run `jam` in this directory.
3. The build system will brew the code into a shiny new driver package!

---

## 🤝 How it fits together

We don't reinvent the wheel here. This Haiku code uses the **HAL** and **RMAPI** from the main `src/amd/` folder.
- **The Brain**: Lives in `src/amd/` (Abstract and smart).
- **The Haiku Body**: Lives here in `haiku-amd/` (Fast and OS-specific).

Together, they make the **Haiku Imposible Team** Edition the best way to use AMD on Haiku!

---
*Developed with pride for the Haiku Community.*
**- The Haiku Imposible Team**