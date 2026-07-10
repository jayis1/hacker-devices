/*
 * sdram.c — External SDRAM Controller Init for ECHO-Phantom
 *
 * Author: jayis1
 * License: GPL-2.0
 *
 * Initializes the FMC (Flexible Memory Controller) to interface
 * with the IS42S16160G 128 MB SDRAM used for the audio capture
 * ring buffer.
 *
 * SDRAM configuration:
 *   - 16-bit data bus
 *   - 4 banks × 8192 rows × 512 columns × 2 bytes = 128 MB
 *   - CAS latency: 3 cycles
 *   - Auto-refresh enabled
 */

#include "board.h"
#include "registers.h"

/* ========================================================================
 *  SDRAM mode register bits
 * ======================================================================== */

#define SDRAM_MODE_BURST_LENGTH_1  (0x0)
#define SDRAM_MODE_BURST_LENGTH_2  (0x1)
#define SDRAM_MODE_BURST_LENGTH_4  (0x2)
#define SDRAM_MODE_BURST_LENGTH_8  (0x3)
#define SDRAM_MODE_CAS_LATENCY_3   (0x3 << 4)
#define SDRAM_MODE_BURST_SEQUENTIAL (0x0 << 3)
#define SDRAM_MODE_BURST_INTERLEAVE (0x1 << 3)

/* ========================================================================
 *  Initialize SDRAM
 * ========================================================================
 */

int sdram_init(void)
{
    /* Enable FMC clock (already done in gpio_init) */

    /* Configure FMC SDRAM controller */

    /* SDRAM Control Register 1 (SDCR1):
     *   NR[1:0] = 0x0 (12 rows, but our chip has 13)
     *   NC[3:2] = 0x0 (8 columns → actually 9)
     *   MWID[5:4] = 0x1 (16-bit bus)
     *   NB[6] = 0x1 (4 internal banks)
     *   CAS[10:9] = 0x3 (CAS latency 3)
     *   WPWR[12] = 0x1 (write protection)
     */

    /* For IS42S16160G: 13 rows, 9 columns, 4 banks, 16-bit data */
    FMC_SDCR1 = (0x1U << 0)   /* NR: 13 rows (value 1 = 11-13 rows) */
              | (0x0U << 2)   /* NC: 9 columns (value 0 = 8-9 columns) */
              | (0x1U << 4)   /* MWID: 16-bit */
              | (0x1U << 6)   /* NB: 4 banks */
              | (0x3U << 9)   /* CAS: 3 cycles */
              | (0x1U << 12); /* WPWR: write protection */

    FMC_SDCR2 = 0;  /* Bank 2 not used */

    /* SDRAM Timing Register 1 (SDTR1):
     *   TMRD = 2 cycles
     *   TXSR = 7 cycles
     *   TRAS = 5 cycles
     *   TRC  = 7 cycles
     *   TWR  = 2 cycles
     *   TRP  = 3 cycles
     *   TRCD = 3 cycles
     */
    FMC_SDTR1 = (SDRAM_TMRD << 0)
              | (SDRAM_TXSR << 4)
              | (SDRAM_TRAS << 8)
              | (SDRAM_TRC  << 12)
              | (SDRAM_TWR  << 16)
              | (SDRAM_TRP  << 20)
              | (SDRAM_TRCD << 24);

    FMC_SDTR2 = 0;

    /* --- SDRAM initialization sequence --- */

    /* Step 1: Send clock configuration enable command */
    FMC_SDCMR = FMC_SDCMR_MODE_CLK | (0x1U << 9);  /* MODE=1, CTB1=1 */
    delay_ms(1);  /* Wait for clock to stabilize */

    /* Step 2: Precharge all banks */
    FMC_SDCMR = FMC_SDCMR_MODE_PALL | (0x1U << 9);
    delay_ms(1);

    /* Step 3: Auto-refresh cycle 1 */
    FMC_SDCMR = FMC_SDCMR_MODE_AUTO | (0x1U << 9) | (5U << 5);  /* 5 cycles */
    delay_ms(1);

    /* Step 4: Auto-refresh cycle 2 */
    FMC_SDCMR = FMC_SDCMR_MODE_AUTO | (0x1U << 9) | (5U << 5);
    delay_ms(1);

    /* Step 5: Load mode register */
    /* Mode register value: CAS=3, burst length=1, sequential burst */
    uint32_t mode_val = SDRAM_MODE_CAS_LATENCY_3 | SDRAM_MODE_BURST_LENGTH_1 | SDRAM_MODE_BURST_SEQUENTIAL;
    /* For 16-bit bus, the mode register is written to BA0[1:0] on DQ[7:0] */
    /* The FMC translates the MODE bits to the appropriate address */
    FMC_SDCMR = FMC_SDCMR_MODE_LOAD | (0x1U << 9) | ((mode_val & 0x1FFFU) << 9);
    delay_ms(1);

    /* Step 6: Enable refresh timer */
    /* Set refresh rate: 7.8125 µs per row × 8192 rows = ~64 ms total refresh */
    /* At 120 MHz FMC clock: 7.8125 µs × 120 MHz ≈ 938 cycles per row */
    /* The refresh counter in the FMC handles this automatically */
    /* We configure it via SDCMR with MODE = 0 and the refresh count */
    FMC_SDCMR = (938U << 1);  /* Set refresh rate (COUNT field) */

    /* Wait for SDRAM to be ready */
    delay_ms(10);

    /* Test: write and read back a value */
    volatile uint32_t *sdram = (volatile uint32_t *)SDRAM_BASE_ADDR;
    sdram[0] = 0x12345678U;
    sdram[1] = 0xDEADBEEFU;
    sdram[2] = 0xCAFEBABEU;

    /* Verify */
    if (sdram[0] != 0x12345678U || sdram[1] != 0xDEADBEEFU) {
        return -1;  /* SDRAM test failed */
    }

    return 0;
}

/* ========================================================================
 *  Allocate memory from SDRAM (simple bump allocator)
 * ======================================================================== */

static uint32_t g_sdram_offset = 0;

void *sdram_alloc(uint32_t size)
{
    if (g_sdram_offset + size > SDRAM_SIZE_BYTES)
        return NULL;

    void *ptr = (void *)(SDRAM_BASE_ADDR + g_sdram_offset);
    g_sdram_offset += size;

    /* Align to 32-byte boundary */
    g_sdram_offset = (g_sdram_offset + 31) & ~31U;

    return ptr;
}