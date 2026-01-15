# 🤖 Prompt Analysis & Critique: Building a GPU Driver with AI

This document reviews our collaboration process, looking at how the instructions were given and how the AI responded. This is to help you (the Human) get even better results from AI in the future.

---

## 🔝 The "Highs" (What worked great)

1. **Specific Error Logs**: When Haiku failed to compile because of `epoll`, you pasted the *exact* error log. This is the **Gold Standard** for AI prompts. It allowed me to fix the code in one shot.
2. **Clear Personas**: Asking for documentation for a "16-year-old average mental capacity" was a brilliant move. It forced the AI to strip away the "corporate talk" and focus on clear, simplified logic.
3. **Evolutionary Thinking**: You moved from "fix this bug" to "how do we maintain this for the next 10 years?". That progression helped build a solid architecture (The Abstraction Mesh).

---

## 📉 The "Lows" (What could be better)

1. **The "Single OS" Trap**: Early prompts focused on Linux. As a result, some Linux-specific code (`epoll`) snuck in. 
   - **Tip**: If you want a universal driver, tell the AI: *"Use only standard POSIX libraries from the start (no epoll, no kqueue, etc.)"*
2. **Batching Requests**: Some prompts were "Wait, do the same but for every folder". 
   - **Tip**: Asking for a "Global Documentation Strategy" at the beginning saves time. But honestly, doing it step-by-step also worked fine for this project.
3. **Implicit vs Explicit**: You expected the driver to be universal, but the AI (me) originally treated it as "Linux-first, port-later".
   - **Tip**: Always start with: *"This must be 100% OS-agnostic. The kernel-specific code must ONLY live in the adapter folders."*

---

## 🛠 Advice for the Future

- **Architecture First**: If you want to automate updates, ask the AI to write a "Vendor Import Policy" early on.
- **The "POSIX Check"**: Every few steps, ask: *"Does this code still use only standard C and POSIX headers?"*
- **The "ELI5" (Explain Like I'm 5)**: Keep using the "16-year-old" or "ELI5" technique. It makes the code better because simple code is usually better code.

---
**- The Haiku Imposible Team**
