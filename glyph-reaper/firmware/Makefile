# GLYPH-REAPER Firmware Makefile
# Target: nRF52840-QIAA (Cortex-M4F, 1MB Flash, 256KB RAM)
# SoftDevice: S140 v7.3.0
# SDK: nRF5 SDK 17.1.0
# Author: jayis1

# Toolchain
CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE    = arm-none-eabi-size
GDB     = arm-none-eabi-gdb

# SDK root (adjust to your installation)
SDK_ROOT ?= /opt/nrf5_sdk_17.1.0

# SoftDevice hex file
SD_HEX  = $(SDK_ROOT)/components/softdevice/s140/hex/s140_nrf52_7.3.0_softdevice.hex

# Target
TARGET  = glyph_reaper
CPU     = cortex-m4
FPU     = fpv4-sp-d16
FLOAT_ABI = hard

# Directories
DRIVERS_DIR = drivers
BUILD_DIR   = build

# Source files — application
C_SRC  = main.c
C_SRC += $(DRIVERS_DIR)/fpga_driver.c
C_SRC += $(DRIVERS_DIR)/dsp_driver.c
C_SRC += $(DRIVERS_DIR)/psram_driver.c
C_SRC += $(DRIVERS_DIR)/flash_driver.c
C_SRC += $(DRIVERS_DIR)/ble_c2_driver.c
C_SRC += $(DRIVERS_DIR)/usb_cdc_driver.c
C_SRC += $(DRIVERS_DIR)/crypto_driver.c
C_SRC += $(DRIVERS_DIR)/accel_driver.c
C_SRC += $(DRIVERS_DIR)/protocol_handler.c
C_SRC += $(DRIVERS_DIR)/compression_engine.c
C_SRC += $(DRIVERS_DIR)/frame_buffer.c

# SDK source files
SDK_C_SRC  = $(SDK_ROOT)/modules/nrfx/mdk/system_nrf52840.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_spim.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_qspi.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_twim.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_gpiote.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_timer.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_rtc.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrx/drivers/src/nrfx_saadc.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_wdt.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_rng.c
SDK_C_SRC += $(SDK_ROOT)/components/libraries/timer/app_timer.c
SDK_C_SRC += $(SDK_ROOT)/components/libraries/usbd/app_usbd.c
SDK_C_SRC += $(SDK_ROOT)/components/libraries/usbd/class/cdc/acm/app_usbd_cdc_acm.c
SDK_C_SRC += $(SDK_ROOT)/components/libraries/crypto/nrf_crypto.c
SDK_C_SRC += $(SDK_ROOT)/components/libraries/crypto/nrf_crypto_aes.c
SDK_C_SRC += $(SDK_ROOT)/components/ble/common/ble_advdata.c
SDK_C_SRC += $(SDK_ROOT)/components/ble/ble_services/ble_nus/ble_nus.c

# Assembly sources
ASM_SRC  = $(SDK_ROOT)/modules/nrfx/mdk/gcc_startup_nrf52840.S

# Object files
C_OBJ  = $(C_SRC:%.c=$(BUILD_DIR)/%.o)
SDK_OBJ = $(SDK_C_SRC:$(SDK_ROOT)/%.c=$(BUILD_DIR)/sdk/%.o)
ASM_OBJ = $(ASM_SRC:%.S=$(BUILD_DIR)/%.o)

# Include paths
INC_DIRS = .
INC_DIRS += $(DRIVERS_DIR)
INC_DIRS += $(SDK_ROOT)/modules/nrfx
INC_DIRS += $(SDK_ROOT)/modules/nrfx/mdk
INC_DIRS += $(SDK_ROOT)/modules/nrfx/drivers/include
INC_DIRS += $(SDK_ROOT)/modules/nrfx/drivers/src
INC_DIRS += $(SDK_ROOT)/components/libraries/timer
INC_DIRS += $(SDK_ROOT)/components/libraries/crypto
INC_DIRS += $(SDK_ROOT)/components/libraries/usbd
INC_DIRS += $(SDK_ROOT)/components/libraries/usbd/class/cdc/acm
INC_DIRS += $(SDK_ROOT)/components/ble/common
INC_DIRS += $(SDK_ROOT)/components/ble/ble_services/ble_nus
INC_DIRS += $(SDK_ROOT)/components/softdevice/s140/headers
INC_DIRS += $(SDK_ROOT)/integration/nrfx
INC_DIRS += $(SDK_ROOT)/integration/nrfx/legacy

# Compiler flags
CFLAGS  = -mcpu=$(CPU) -mthumb -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
CFLAGS += -std=gnu11 -Os -g3 -ffunction-sections -fdata-sections
CFLAGS += -DNRF52840_XXAA -DNRF52 -DNRF52840 -DBOARD_CUSTOM
CFLAGS += -DS140 -DSOFTDEVICE_PRESENT -DBLE_STACK_SUPPORT_REQD
CFLAGS += -DFLOAT_ABI_HARD -DCONFIG_GPIO_AS_PINRESET
CFLAGS += -DGLYPH_REAPER_REV_A
CFLAGS += $(addprefix -I,$(INC_DIRS))

# Linker flags
LDFLAGS = -mcpu=$(CPU) -mthumb -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
LDFLAGS += -Wl,--gc-sections -Wl,--map=$(BUILD_DIR)/$(TARGET).map
LDFLAGS += -Wl,-T,$(SDK_ROOT)/modules/nrfx/mdk/gcc_nrf52840.ld
LDFLAGS += --specs=nano.specs --specs=nosys.specs

# Libraries
LIBS = -lc -lm -lnosys

#=============================================================================
# Build targets
#=============================================================================

.PHONY: all clean flash erase debug

all: $(BUILD_DIR)/$(TARGET).hex

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/$(DRIVERS_DIR)
	mkdir -p $(BUILD_DIR)/sdk

# Compile application C files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile SDK C files
$(BUILD_DIR)/sdk/%.o: $(SDK_ROOT)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile assembly files
$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(BUILD_DIR)/$(TARGET).elf: $(C_OBJ) $(SDK_OBJ) $(ASM_OBJ)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@
	$(SIZE) $@

# Generate hex file
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# Generate binary file
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Flash with J-Link
flash: $(BUILD_DIR)/$(TARGET).hex
	JLinkExe -device nRF52840_xxAA -if SWD -speed 4000 -autoconnect 1 \
		-command "loadfile $(SD_HEX); loadfile $<; r; g; q"

# Erase chip
erase:
	JLinkExe -device nRF52840_xxAA -if SWD -speed 4000 -autoconnect 1 \
		-command "erase; q"

# Debug with GDB
debug: $(BUILD_DIR)/$(TARGET).elf
	JLinkGDBServer -device nRF52840_xxAA -if SWD -speed 4000 &
	$(GDB) -ex "target remote localhost:2331" \
		-ex "load" \
		-ex "monitor reset" \
		$(BUILD_DIR)/$(TARGET).elf

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Show sizes
size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) $<