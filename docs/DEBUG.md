# 🩺 HIT Debug Guide: Diagnosing Hardware Heartbeats

Yo! You've successfully exited simulation mode and are now talking to your real **Radeon HD 7290**. This is where things get exciting—and a little messy! If the system crashes, this guide will help you understand "what the hardware was thinking."

---

## 1. The `HW_TRACE` System

We've added a high-visibility tracing layer to the driver. Every time the "Brain" (HAL) touches a register on your GPU, it prints a message like this:

`[HW_TRACE] WRITE | Offset: 0x00000100 | Val: 0x00000001`

### How to read it:
- **READ/WRITE**: Are we asking the GPU for info, or giving it an order?
- **Offset**: This is the "Door Number" on the GPU chip. Example: `0x100` might be the "Wake Up" doorbell.
- **Val**: The secret code we are sending or receiving.

---

## 2. Where to find the logs?

On Haiku, we've optimized the driver to print these directly to your **Terminal**. 
- Simply run: `./rmapi_server` manually.
- Every "poke" the driver makes to your APU will be listed in real-time.

### 🔴 If Haiku hangs (The "White Screen of Death"):
This means the driver sent a command that the hardware didn't like! 
1. Look at the **LAST** `HW_TRACE` line before the hang.
2. That offset is our suspect! Compare it with the official AMD `drm-kmod` source to see what that register does.

---

## 3. The "Hardware Handshake" Milestone

When you see this in your terminal:
`HAL: Poking hardware... Hello?`
`[HW_TRACE] WRITE | Offset: 0x00000100 | Val: 0x00000001`
`[HW_TRACE] READ  | Offset: 0x00000100 | Val: 0x00000001`

**Congratulations!** You just had a real conversation with your Radeon silicon. The driver sent a `1` and the chip said `1` back. You are officially accelerating!

---

## 🛠 Pro Tips for Debugging
- **Keep it slow**: We use `os_prim_delay_us` to give the chip time to think. If you get weird values, we might need to increase the delay.
- **Root access**: On some systems, you need to run as `root` or `sudo` to touch physical memory!
- **Dprintf**: If you're building the kernel version, check `/var/log/syslog` or use `dmesg` to see the kernel-level heartbeat.

*Go forth and be the Hardware Whisperer!*
**- The Haiku Imposible Team**
