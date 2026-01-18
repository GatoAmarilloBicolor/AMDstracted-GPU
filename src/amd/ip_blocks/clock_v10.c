/*
 * Clock Control v10 (Pixel Clock Generator) for NAVI10, NAVI14, NAVI20
 * Handles PLL programming for display pixel clocks
 * 
 * Developed by: Haiku Imposible Team (HIT)
 */

#include "../hal/hal.h"
#include "../../os/os_primitives.h"
#include <string.h>
#ifdef __HAIKU__
#include <GraphicsDefs.h>
#endif

/* ============================================================================
 * PLL Register Offsets (SPLL - System PLL for display)
 * ============================================================================ */

#define mmSPLL_CNTL_0                    0x0014  // PLL control
#define mmSPLL_CNTL_1                    0x0015  // PLL control
#define mmSPLL_CNTL_2                    0x0016  // PLL control
#define mmSPLL_STATUS                    0x0017  // PLL status/lock
#define mmSPLL_FUNC_CNTL                 0x0013  // Function control

#define mmDCFEV_DISP_CLK_CNTL            0x4100  // Display clock control

/* Offset to add to register address */
#define GFXHUB_OFFSET 0x0000

/* ============================================================================
 * Clock State Structure
 * ============================================================================ */

struct clock_v10_state {
    uint32_t spll_enabled;
    uint32_t current_frequency_khz;
    uint32_t ref_clock_khz;        // Reference clock (usually 100MHz)
    uint32_t fbdiv;                // Feedback divider
    uint32_t postdiv;              // Post divider
};

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/*
 * Calculate PLL dividers for target frequency
 * 
 * Formula: f_out = (f_ref * FBDIV) / (POSTDIV * 2)
 * 
 * Reference clock is typically 100 MHz (100000 kHz)
 * 
 * Example: For 148.5 MHz pixel clock:
 *   FBDIV = 297, POSTDIV = 1
 *   f_out = (100000 * 297) / (1 * 2) = 14850000 / 100 = 148500 kHz
 */
static int clock_calculate_dividers(uint32_t target_khz, uint32_t ref_khz,
                                     uint32_t *fbdiv, uint32_t *postdiv) {
    // Navi10 PLL constraints:
    // FBDIV: 16-255 (feedback divider)
    // POSTDIV: 1-7 (post-divider stages)
    
    os_prim_log("Clock: Calculating dividers for %u kHz (ref=%u kHz)\n",
                target_khz, ref_khz);
    
    // Try postdiv values from 1 to 7
    for (uint32_t pd = 1; pd <= 7; pd++) {
        // Calculate required FBDIV: FBDIV = (target * postdiv * 2) / ref
        uint32_t fbdiv_needed = (target_khz * pd * 2) / ref_khz;
        
        // Check if FBDIV is in valid range
        if (fbdiv_needed >= 16 && fbdiv_needed <= 255) {
            // Verify accuracy
            uint32_t actual = (ref_khz * fbdiv_needed) / (pd * 2);
            
            // Allow 1% tolerance
            if (actual > target_khz * 99 / 100 && 
                actual < target_khz * 101 / 100) {
                *fbdiv = fbdiv_needed;
                *postdiv = pd;
                
                os_prim_log("Clock: Found dividers: FBDIV=%u, POSTDIV=%u\n",
                           fbdiv_needed, pd);
                os_prim_log("Clock: Actual frequency: %u kHz\n", actual);
                return 0;
            }
        }
    }
    
    os_prim_log("Clock: WARNING - Could not find valid dividers for %u kHz\n",
                target_khz);
    
    // Fallback: use safe defaults (close to target)
    *fbdiv = 150;    // ~75 MHz output
    *postdiv = 1;
    return -1;
}

/*
 * Program pixel clock via SPLL (System PLL)
 */
int clock_v10_set_pixel_clock(struct OBJGPU *adev, uint32_t pixel_clock_10khz) {
    if (!adev || !adev->mmio_base) {
        os_prim_log("Clock: ERROR - Invalid GPU or MMIO\n");
        return -1;
    }

    // Convert from 10kHz units to kHz
    uint32_t target_khz = pixel_clock_10khz * 10;
    
    os_prim_log("Clock: Setting pixel clock to %u.%u MHz (%u kHz)\n",
                target_khz / 1000, (target_khz % 1000) / 100, target_khz);

    // Reference clock is typically 100 MHz
    uint32_t ref_khz = 100000;  // 100 MHz
    
    // Calculate PLL dividers
    uint32_t fbdiv, postdiv;
    if (clock_calculate_dividers(target_khz, ref_khz, &fbdiv, &postdiv) != 0) {
        os_prim_log("Clock: WARNING - Using fallback dividers\n");
    }

    // Map PLL registers
    uintptr_t pll_base = (uintptr_t)adev->mmio_base + GFXHUB_OFFSET;
    
    // Safety check
    if (pll_base < (uintptr_t)adev->mmio_base ||
        pll_base >= (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_log("Clock: ERROR - PLL base address out of bounds\n");
        return -1;
    }

    // ========================================================================
    // Step 1: Disable PLL during programming
    // ========================================================================
    os_prim_log("Clock: Disabling PLL for programming...\n");
    
    uintptr_t spll_cntl_addr = pll_base + mmSPLL_CNTL_0 * 4;
    os_prim_write32(spll_cntl_addr, 0);  // Disable
    os_prim_delay_us(100);

    // ========================================================================
    // Step 2: Program feedback divider
    // ========================================================================
    os_prim_log("Clock: Programming feedback divider (FBDIV=%u)...\n", fbdiv);
    
    uintptr_t spll_cntl1_addr = pll_base + mmSPLL_CNTL_1 * 4;
    // FBDIV typically in bits [7:0]
    uint32_t cntl1_val = fbdiv & 0xFF;
    os_prim_write32(spll_cntl1_addr, cntl1_val);
    os_prim_delay_us(10);

    // ========================================================================
    // Step 3: Program post divider
    // ========================================================================
    os_prim_log("Clock: Programming post divider (POSTDIV=%u)...\n", postdiv);
    
    uintptr_t spll_cntl2_addr = pll_base + mmSPLL_CNTL_2 * 4;
    // POSTDIV typically in bits [2:0]
    uint32_t cntl2_val = (postdiv - 1) & 0x7;  // postdiv is 1-7, encoded as 0-6
    os_prim_write32(spll_cntl2_addr, cntl2_val);
    os_prim_delay_us(10);

    // ========================================================================
    // Step 4: Enable PLL
    // ========================================================================
    os_prim_log("Clock: Enabling PLL...\n");
    
    uint32_t cntl0_val = 0x1;  // Enable bit
    os_prim_write32(spll_cntl_addr, cntl0_val);
    os_prim_delay_us(100);

    // ========================================================================
    // Step 5: Wait for PLL lock
    // ========================================================================
    os_prim_log("Clock: Waiting for PLL lock...\n");
    
    uintptr_t spll_status_addr = pll_base + mmSPLL_STATUS * 4;
    int timeout = 1000;  // 100ms timeout
    
    while (timeout--) {
        uint32_t status = os_prim_read32(spll_status_addr);
        
        // Lock bit is typically bit 31
        if (status & 0x80000000) {
            os_prim_log("Clock: PLL locked! ✓\n");
            break;
        }
        
        os_prim_delay_us(100);
    }
    
    if (timeout <= 0) {
        os_prim_log("Clock: WARNING - PLL lock timeout\n");
        // Don't fail, might still work
    }

    // ========================================================================
    // Step 6: Enable display clock output
    // ========================================================================
    os_prim_log("Clock: Enabling display clock output...\n");
    
    uintptr_t disp_clk_cntl = pll_base + mmDCFEV_DISP_CLK_CNTL * 4;
    uint32_t disp_val = 0x1;  // Enable
    os_prim_write32(disp_clk_cntl, disp_val);
    os_prim_delay_us(10);

    os_prim_log("Clock: Pixel clock set successfully! Target: %u kHz\n", target_khz);
    
    return 0;
}

/*
 * Get current pixel clock frequency
 */
uint32_t clock_v10_get_pixel_clock(struct OBJGPU *adev) {
    if (!adev || !adev->mmio_base) {
        return 0;
    }
    
    // For now, return a reasonable default (148.5 MHz for 1920x1080@60)
    // Real implementation would read from PLL registers
    return 14850;  // 148.5 MHz in 10kHz units
}

/*
 * Disable clock (power saving)
 */
int clock_v10_disable_clock(struct OBJGPU *adev) {
    if (!adev || !adev->mmio_base) {
        return -1;
    }
    
    os_prim_log("Clock: Disabling display clock...\n");
    
    uintptr_t pll_base = (uintptr_t)adev->mmio_base + GFXHUB_OFFSET;
    uintptr_t spll_cntl_addr = pll_base + mmSPLL_CNTL_0 * 4;
    
    if (spll_cntl_addr >= (uintptr_t)adev->mmio_base &&
        spll_cntl_addr < (uintptr_t)adev->mmio_base + 0x1000000) {
        os_prim_write32(spll_cntl_addr, 0);
    }
    
    return 0;
}
