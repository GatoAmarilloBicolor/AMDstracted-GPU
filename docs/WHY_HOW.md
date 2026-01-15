# 💡 Why and How (The Big Plan)

## The "Why"
Most graphics drivers are like giant, scary machines that only one person in the world knows how to fix. They are stuck in the kernel (the "forbidden zone" of your computer), and if they sneeze, your PC crashes.

**The Haiku Imposible Team** wanted to change that. We want a driver that is:
1. **Simple**: Easy for any 16-year-old to read and play with.
2. **Safe**: If it breaks, your screen might flicker, but your computer stays alive.
3. **Everywhere**: We want AMD cards to work on Haiku OS just as well as they do on Linux.

---

## The "How"

### 🛡 The Bubble Principle
We moved the scary parts of the driver into their own "bubble" (a process). This process is the **RMAPI Server**. It has all the "brain power" but none of the danger.

### 🧩 The LEGO Method
We split the GPU hardware into **IP Blocks**. 
- Want to add support for a new GPU? Just add a new block!
- Want to fix a bug in the screen display? You only need to touch the **DCE Block**!

### 🌍 The Universal Language
We use **OS Primitives**. Instead of using Linux-specific code everywhere, we use a middleman. 
- When we want to allocate memory, we say `os_prim_alloc`. 
- Our "Middleman" then decides whether to use Linux code or Haiku code. 
- This makes our driver a world traveler!

---
*Stay cool, stay portable.*
**- The Haiku Imposible Team**
