# 🧩 How the Magic Happens (Step-by-Step)

Want to know exactly what happens when you tell the computer to draw something? Here is the secret path data takes inside the **HIT Abstracted Driver**:

## 1. You ask for something!
Imagine you're coding a game. You want to save some space on the GPU for a cool texture. You call `rmapi_alloc_memory`.
- **Think of this as:** Sending a text message to your friend who owns the GPU.

## 2. The Subway Ride (IPC)
Your request travels through a "subway tunnel" (a Unix Socket) to the **RMAPI Server**. 
- **The Catch:** Your app waits patiently until the server texts back.

## 3. The Brain (OBJGPU)
The Server is the "Brain" of the operation. It looks at your request and says, "Okay, let's find a spot on the GPU for this!". It uses its `OBJGPU` (a special container for all GPU info) to keep track of what's happening.

## 4. The Specialist (HAL)
The Brain doesn't do the physical work. It calls the **HAL Layer** (Hardware Abstraction). The HAL is like a manager who knows which "IP Block" (specialist worker) to call.
- For memory, it calls the **GMC Specialist**.

## 5. The Worker (IP Block)
The **GMC Block** does the math. Since we're in "Userland Mode", it asks the computer for some regular RAM but pretends it's GPU RAM. 
- In the future, this is where it would talk to the actual graphics card registers!

## 6. The Cleanup Crew (RESSERV)
Every bit of memory we give you gets a "tag" (a handle). We put this tag in a family tree. 
- If you forget to delete the memory, or if your app crashes, the tree sees that you're gone and cleans it up for us. No mess left behind!

## 7. Success!
The Server sends the GPU memory address back to your app. Now you can start drawing!

---
### Cheat Sheet
`App` -> `Subway Tunnel` -> `Brain` -> `Specialist Manager` -> `Worker` -> `Done!`

*Easy, right?*
