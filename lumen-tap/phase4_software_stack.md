# Phase 4: Software Stack — LumenTap

**Document Version:** 1.0
**Date:** 2026-07-06
**Author:** jayis1
**Status:** Final

---

## 1. Memory Map & Boot

The STM32H743 boots from flash at 0x0800_0000. The linker script
(`firmware/linker.ld`) places:

| Region | Address | Size | Contents |
|--------|---------|------|----------|
| FLASH | 0x0800_0000 | 2 MB | code + .rodata |
| DTCM | 0x2000_0000 | 128 KB | (unused — future fast data) |
| ITCM | 0x0000_0000 | 64 KB | (unused — future critical code) |
| RAM (AXI SRAM) | 0x2400_0000 | 1 MB | .data, .bss, stacks, buffers |

Stack top: 0x2400_0000 + 1 MB = 0x2404_0000.

No second-stage bootloader is used; the firmware is flashed directly
via OpenOCD over SWD. A future revision may add a USB DFU bootloader in
the first 16 KB of flash with a jump-to-app trampoline.

---

## 2. Clock Tree

```
HSE 25 MHz ── PLL1 (M=5, N=192, P=2) ── SYSCLK 480 MHz
                                          ├── CPU 480 MHz
                                          ├── AHB 240 MHz
                                          ├── APB1 120 MHz (I2C, SDMMC1)
                                          ├── APB2 120 MHz (TIM1, ADC)
                                          └── USB OTG-HS 48 MHz (PLL3 or internal)
```

`clock_init()` in `main.c` programs RCC_CR / RCC_PLLCFGR / RCC_CFGR /
FLASH_ACR. VOS scale 1 (highest performance) is set via PWR_D3CR
before raising the CPU clock.

---

## 3. GPIO & Peripheral Init Order

`board_init()` calls, in order:

1. `clock_init()` — HSE + PLL1 + flash latency + SYSCLK switch.
2. `systick_init()` — 1 ms tick, drives `g_tick` and laser interlock
   refresh.
3. `gpio_global_init()` — enable all GPIO bank clocks.
4. `laser_init()` — laser EN/PMW/arm-key/LEDs + I2C1 + TSL257 + TIM1
   PWM. Laser stays OFF.
5. `audio_init()` — ADC1 calibration + DMA1_Stream0 double-buffer +
   USB OTG-HS init + DSP state init.
6. `sdlog_init()` — SDMMC1 init + card identification sequence
   (CMD0/8/55/41/2/3) + clock to 25 MHz.

After init, `main()` calls `audio_start_capture()` to arm ADC+DMA and
enters the super-loop.

---

## 4. Critical Device Driver — ADC + DMA + DSP

### 4.1 ADC1 configuration

- 16-bit resolution (`ADC_CFGR_RES_16BIT`).
- Continuous mode, DMA circular, overwrite on overrun.
- Differential pair INP0/INN0 (PA0/PA1).
- Sample time 38.5 ADC clocks (~ 8 µs at 5 MHz ADC clock → fine for 192
  kS/s with 1 channel).
- Boost mode enabled for higher sample rates.

### 4.2 DMA1_Stream0

- Peripheral-to-memory, circular, double-buffer (M0AR + M1AR).
- 16-bit transfers, 128 samples per half-buffer.
- HTIE + TCIE interrupts → `DMA1_Stream0_IRQHandler` → `audio_dma_isr()`.

### 4.3 DSP block (per half-transfer, 666 µs budget)

| Stage | Taps/Stages | Cycles (approx) | Time @ 480 MHz |
|-------|-------------|-----------------|----------------|
| int16→float | 128 | 1k | 2 µs |
| DC blocker FIR | 31 taps | 4k | 8 µs |
| Bandpass biquad | 4 stages | 3k | 6 µs |
| FM demod (Hilbert + atan2) | 64 taps | 12k | 25 µs |
| Noise suppress (64-pt DFT×2) | — | 30k | 62 µs |
| AGC + gain apply | 128 | 2k | 4 µs |
| Decimate 4× + 24-bit pack | 32 | 2k | 4 µs |
| **Total** | | | **~ 111 µs / 666 µs** |

Headroom is ~ 83 %, leaving ample time for SD writes and CDC JSON.

---

## 5. USB Stack

A minimal USB OTG-HS device stack is implemented inline in
`audio.c` + `usb_descriptors.h`:

- Standard control endpoint (EP0) handles GET_DESCRIPTOR (device,
  config, string), SET_ADDRESS, SET_CONFIGURATION, SET_INTERFACE.
- UAC2 audio streaming endpoint (EP_AUDIO_IN, ISO async) is fed from
  the `audio_ring` produced by the DSP ISR.
- CDC-ACM control interface (EP_CDC_NOTIFY + EP_CDC_OUT + EP_CDC_IN)
  carries the JSON wire protocol.
- UAC2 class requests (CUR/RANGE for sampling frequency, mute, volume)
  return fixed 48 kHz / 24-bit values.

No RTOS, no USB library dependency — the OTG register programming is
hand-written for the H7's DWC2 core.

---

## 6. CDC Wire Protocol (device ↔ app)

Line-oriented JSON, `\n`-terminated.

| Direction | Example |
|-----------|---------|
| app → device | `{"cmd":"set_power","duty":28}\n` |
| app → device | `{"cmd":"set_bandpass","low":20,"high":16000}\n` |
| app → device | `{"cmd":"record_start","operator":"alice","target_id":"office-A","note":"single-pane"}\n` |
| device → app | `{"type":"status","tick":12345,"laser":1,"pwm":28,...}\n` (4 Hz) |

Parser: `parse_and_exec()` in `main.c` does minimal `strstr`/`strchr`
field extraction to avoid pulling a JSON library into the firmware.

---

## 7. Device Tree (conceptual)

```
/ {
    model = "LumenTap Laser-Doppler Acoustic Tool";
    compatible = "jayis1,lumentap-1.0";
    #address-cells = <1>;
    #size-cells = <1>;

    cpus { cpu@0 { clock-frequency = <480000000>; }; };

    soc {
        rcc: rcc@58024400 { compatible = "st,stm32h7-rcc"; };
        pwr: pwr@58024800 { compatible = "st,stm32h7-pwr"; };

        adc1: adc@40022000 {
            compatible = "st,stm32h7-adc";
            reg = <0x40022000 0x100>;
            assigned-clock-rates = <5000000>;
            st,adc-channels = <0>;
            st,differential;
        };

        dma1: dma@48026000 {
            compatible = "st,stm32h7-dma";
            reg = <0x48026000 0x400>;
            streams = <8>;
        };

        i2c1: i2c@40005400 {
            compatible = "st,stm32h7-i2c";
            clock-frequency = <100000>;
            tsl257@29 { compatible = "ams,tsl257"; reg = <0x29>; };
            cs4271@4a { compatible = "cirrus,cs4271"; reg = <0x4a>; status="disabled"; };
        };

        sdmmc1: sdmmc@58007000 {
            compatible = "st,stm32h7-sdmmc";
            bus-width = <4>;
            max-frequency = <50000000>;
        };

        usb_otg_hs: usb@40040000 {
            compatible = "st,stm32h7-otg";
            dr_mode = "peripheral";
            maximum-speed = "full-speed";
        };

        tim1: pwm@40010000 {
            compatible = "st,stm32h7-tim";
            pwm-channel = <1>;
            pwm-frequency = <100000>;
        };
    };

    laser { compatible = "jayis1,lumentap-laser"; pwm = <&tim1 1>; class1-cap-uw = <390>; };
    safety {
        arm-key-gpio = <&gpiob 3>;
        ambient-sensor = <&tsl257>;
        ambient-block-threshold = <6000>;
    };
};
```

---

## 8. Bootloader

Not present in v1.0. v1.1 plan: USB DFU bootloader in flash sectors 0–1
(16 KB), application in sectors 2+. DFU triggered by holding the user
button (PB8) at reset. Trampoline checks a magic word in backup-domain
SRAM to decide whether to stay in DFU or jump to application.

— *jayis1*