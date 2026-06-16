# CAN Creeper Firmware Makefile
# Target: nRF52840-QIAA (Cortex-M4F, 1MB Flash, 256KB RAM)
# SoftDevice: S140 v7.3.0
# SDK: nRF5 SDK 17.1.0

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
TARGET  = can_creeper
CPU     = cortex-m4
FPU     = fpv4-sp-d16
FLOAT_ABI = hard

# Directories
DRIVERS_DIR = drivers
BUILD_DIR   = build

# Source files
C_SRC  = main.c
C_SRC += $(DRIVERS_DIR)/can_fd_driver.c
C_SRC += $(DRIVERS_DIR)/psram_driver.c
C_SRC += $(DRIVERS_DIR)/ble_nus_driver.c
C_SRC += $(DRIVERS_DIR)/usb_cdc_driver.c
C_SRC += $(DRIVERS_DIR)/oled_driver.c
C_SRC += $(DRIVERS_DIR)/protocol_handler.c
C_SRC += $(DRIVERS_DIR)/injection_engine.c

# SDK source files
SDK_C_SRC  = $(SDK_ROOT)/modules/nrfx/mdk/system_nrf52840.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_spim.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_qspi.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_twim.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_gpiote.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_timer.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_rtc.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_saadc.c
SDK_C_SRC += $(SDK_ROOT)/modules/nrfx/drivers/src/nrfx_wdt.c
SDK_C_SRC += $(SDK_ROOT)/components/libraries/timer/app_timer.c
SDK_C_SRC += $(SDK_ROOT)/components/libraries/usbd/app_usbd.c
SDK_C_SRC += $(SDK_ROOT)/components/libraries/usbd/class/cdc/acm/app_usbd_cdc_acm.c
SDK_C_SRC += $(SDK_ROOT)/components/ble/common/ble_advdata.c
SDK_C_SRC += $(SDK_ROOT)/components/ble/ble_services/ble_nus/ble_nus.c

# Assembly sources
ASM_SRC  = $(SDK_ROOT)/modules/nrfx/mdk/gcc_startup_nrf52840.S

# Object files
C_OBJ  = $(C_SRC:%.c=$(BUILD_DIR)/%.o)
SDK_OBJ = $(SDK_C_SRC:$(SDK_ROOT)/%.c=$(BUILD_DIR)/sdk/%.o)
ASM_OBJ = $(ASM_SRC:$(SDK_ROOT)/%.S=$(BUILD_DIR)/sdk/%.o)
OBJ     = $(C_OBJ) $(SDK_OBJ) $(ASM_OBJ)

# Defines
DEFINES  = -DNRF52840_XXAA
DEFINES += -DBOARD_CAN_CREEPER
DEFINES += -DSOFTDEVICE_PRESENT
DEFINES += -DS140
DEFINES += -DAPP_TIMER_V2
DEFINES += -DAPP_TIMER_V2_RTC1_ENABLED
DEFINES += -DNRF_SDH_BLE_ENABLED
DEFINES += -DNRF_SDH_BLE_GATT_MAX_MTU_SIZE=247
DEFINES += -DNRF_SDH_BLE_VS_UUID_COUNT=0
DEFINES += -DNRF_SDH_SOC_ENABLED
DEFINES += -DNRF_SDH_DISPATCH_MODEL=2
DEFINES += -DNRF_SDH_STACK_OBSERVER=1
DEFINES += -DNRF_SDH_REQ_OBSERVER=1
DEFINES += -DNRF_SDH_STATE_OBSERVER=1
DEFINES += -DNRF_SDH_BLE_OBSERVER=1
DEFINES += -DNRF_SDH_SOC_OBSERVER=1
DEFINES += -DNRF_CRYPTO_MAX_INSTANCE_COUNT=1
DEFINES += -DNRFX_SPIM_ENABLED
DEFINES += -DNRFX_SPIM0_ENABLED
DEFINES += -DNRFX_SPIM1_ENABLED
DEFINES += -DNRFX_QSPI_ENABLED
DEFINES += -DNRFX_TWIM_ENABLED
DEFINES += -DNRFX_TWIM0_ENABLED
DEFINES += -DNRFX_GPIOTE_ENABLED
DEFINES += -DNRFX_TIMER_ENABLED
DEFINES += -DNRFX_TIMER0_ENABLED
DEFINES += -DNRFX_TIMER1_ENABLED
DEFINES += -DNRFX_TIMER2_ENABLED
DEFINES += -DNRFX_RTC_ENABLED
DEFINES += -DNRFX_RTC0_ENABLED
DEFINES += -DNRFX_RTC1_ENABLED
DEFINES += -DNRFX_RTC2_ENABLED
DEFINES += -DNRFX_SAADC_ENABLED
DEFINES += -DNRFX_WDT_ENABLED
DEFINES += -DUSBD_ENABLED
DEFINES += -DUSBD_CDC_ACM_ENABLED
DEFINES += -DBLE_NUS_ENABLED
DEFINES += -DDEBUG
DEFINES += -DDEBUG_NRF

# Compiler flags
CFLAGS  = -mcpu=$(CPU) -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
CFLAGS += -mthumb -mabi=aapcs
CFLAGS += -Wall -Wextra -Werror -Wno-unused-parameter
CFLAGS += -O2 -g3 -ggdb3
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -fno-strict-aliasing -fno-builtin -fshort-enums
CFLAGS += -fstack-usage -fno-common
CFLAGS += $(DEFINES)

# Assembler flags
ASMFLAGS  = -mcpu=$(CPU) -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += -x assembler-with-cpp
ASMFLAGS += $(DEFINES)

# Linker flags
LDFLAGS  = -mcpu=$(CPU) -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
LDFLAGS += -mthumb -mabi=aapcs
LDFLAGS += -L$(SDK_ROOT)/modules/nrfx/mdk
LDFLAGS += -Tlinker.ld
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--print-memory-usage
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map
LDFLAGS += -nostartfiles
LDFLAGS += -lm -lc -lnosys

# Include paths
INC  = -I.
INC += -I$(DRIVERS_DIR)
INC += -I$(SDK_ROOT)/modules/nrfx
INC += -I$(SDK_ROOT)/modules/nrfx/mdk
INC += -I$(SDK_ROOT)/modules/nrfx/hal
INC += -I$(SDK_ROOT)/modules/nrfx/drivers/include
INC += -I$(SDK_ROOT)/components
INC += -I$(SDK_ROOT)/components/libraries
INC += -I$(SDK_ROOT)/components/libraries/timer
INC += -I$(SDK_ROOT)/components/libraries/usbd
INC += -I$(SDK_ROOT)/components/libraries/usbd/class/cdc/acm
INC += -I$(SDK_ROOT)/components/softdevice/s140/headers
INC += -I$(SDK_ROOT)/components/softdevice/common
INC += -I$(SDK_ROOT)/components/ble/common
INC += -I$(SDK_ROOT)/components/ble/ble_services/ble_nus
INC += -I$(SDK_ROOT)/integration/nrfx
INC += -I$(SDK_ROOT)/integration/nrfx/legacy

# Build rules
.PHONY: all clean flash flash_swd erase size debug gdb

all: $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# Create build directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/$(DRIVERS_DIR)
	mkdir -p $(BUILD_DIR)/sdk/modules/nrfx/mdk
	mkdir -p $(BUILD_DIR)/sdk/modules/nrfx/drivers/src
	mkdir -p $(BUILD_DIR)/sdk/components/libraries/timer
	mkdir -p $(BUILD_DIR)/sdk/components/libraries/usbd
	mkdir -p $(BUILD_DIR)/sdk/components/libraries/usbd/class/cdc/acm
	mkdir -p $(BUILD_DIR)/sdk/components/ble/common
	mkdir -p $(BUILD_DIR)/sdk/components/ble/ble_services/ble_nus

# Compile C sources
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(BUILD_DIR)/sdk/%.o: $(SDK_ROOT)/%.c | $(BUILD_DIR)
	@echo "  CC    $<"
	@$(CC) $(CFLAGS) $(INC) -c $< -o $@

# Assemble
$(BUILD_DIR)/sdk/%.o: $(SDK_ROOT)/%.S | $(BUILD_DIR)
	@echo "  ASM   $<"
	@$(CC) $(ASMFLAGS) $(INC) -c $< -o $@

# Link
$(BUILD_DIR)/$(TARGET).elf: $(OBJ) linker.ld
	@echo "  LD    $@"
	@$(CC) $(LDFLAGS) $(OBJ) -o $@
	@$(SIZE) $@

# Create hex
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	@echo "  HEX   $@"
	@$(OBJCOPY) -O ihex $< $@

# Create binary
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	@echo "  BIN   $@"
	@$(OBJCOPY) -O binary $< $@

# Create disassembly
$(BUILD_DIR)/$(TARGET).dis: $(BUILD_DIR)/$(TARGET).elf
	@echo "  DIS   $@"
	@$(OBJDUMP) -d $< > $@

# Flash with nrfutil (DFU over USB)
flash: $(BUILD_DIR)/$(TARGET).hex
	@echo "Creating DFU package..."
	nrfutil pkg generate --hw-version 52 --sd-req 0x0100 \
		--application-version 1 --application $(BUILD_DIR)/$(TARGET).hex \
		--key-file private.pem $(BUILD_DIR)/$(TARGET).zip
	@echo "Flashing via USB DFU..."
	nrfutil dfu usb-serial -pkg $(BUILD_DIR)/$(TARGET).zip -p /dev/ttyACM0

# Flash with nrfjprog (SWD)
flash_swd: $(BUILD_DIR)/$(TARGET).hex
	@echo "Flashing via SWD..."
	nrfjprog --program $(BUILD_DIR)/$(TARGET).hex --sectorerase --reset

# Flash SoftDevice + application combined
flash_all: $(BUILD_DIR)/$(TARGET).hex
	@echo "Merging SoftDevice + Application..."
	mergehex -m $(SD_HEX) $(BUILD_DIR)/$(TARGET).hex -o $(BUILD_DIR)/combined.hex
	@echo "Flashing combined image..."
	nrfjprog --program $(BUILD_DIR)/combined.hex --sectorerase --reset

# Erase chip
erase:
	nrfjprog --eraseall

# Recover (if APPROTECT is set)
recover:
	nrfjprog --recover

# Size analysis
size: $(BUILD_DIR)/$(TARGET).elf
	$(SIZE) $<

# Debug with GDB
debug: $(BUILD_DIR)/$(TARGET).elf
	$(GDB) -ex "target extended-remote :3333" -ex "monitor reset halt" $<

# GDB server
gdbserver:
	JLinkGDBServer -device nRF52840_xxAA -if SWD -speed 4000

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Dependencies
main.o: main.c board.h registers.h drivers/can_fd_driver.h drivers/psram_driver.h \
        drivers/ble_nus_driver.h drivers/usb_cdc_driver.h drivers/oled_driver.h \
        drivers/protocol_handler.h drivers/injection_engine.h

$(DRIVERS_DIR)/can_fd_driver.o: $(DRIVERS_DIR)/can_fd_driver.c $(DRIVERS_DIR)/can_fd_driver.h board.h registers.h
$(DRIVERS_DIR)/psram_driver.o: $(DRIVERS_DIR)/psram_driver.c $(DRIVERS_DIR)/psram_driver.h board.h registers.h
$(DRIVERS_DIR)/ble_nus_driver.o: $(DRIVERS_DIR)/ble_nus_driver.c $(DRIVERS_DIR)/ble_nus_driver.h board.h registers.h
$(DRIVERS_DIR)/usb_cdc_driver.o: $(DRIVERS_DIR)/usb_cdc_driver.c $(DRIVERS_DIR)/usb_cdc_driver.h board.h registers.h
$(DRIVERS_DIR)/oled_driver.o: $(DRIVERS_DIR)/oled_driver.c $(DRIVERS_DIR)/oled_driver.h board.h registers.h
$(DRIVERS_DIR)/protocol_handler.o: $(DRIVERS_DIR)/protocol_handler.c $(DRIVERS_DIR)/protocol_handler.h board.h
$(DRIVERS_DIR)/injection_engine.o: $(DRIVERS_DIR)/injection_engine.c $(DRIVERS_DIR)/injection_engine.h board.h registers.h

# Help
help:
	@echo "CAN Creeper Firmware Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all         Build firmware (hex + bin)"
	@echo "  flash       Flash via USB DFU (nrfutil)"
	@echo "  flash_swd   Flash via SWD (nrfjprog)"
	@echo "  flash_all   Flash SoftDevice + application"
	@echo "  erase       Erase entire chip"
	@echo "  recover     Recover chip (disable APPROTECT)"
	@echo "  size        Print size analysis"
	@echo "  debug       Start GDB debug session"
	@echo "  gdbserver   Start JLink GDB server"
	@echo "  clean       Remove build artifacts"
	@echo ""
	@echo "Variables:"
	@echo "  SDK_ROOT    Path to nRF5 SDK (default: /opt/nrf5_sdk_17.1.0)"
	@echo ""
	@echo "Prerequisites:"
	@echo "  arm-none-eabi-gcc (GNU ARM Embedded Toolchain 10.3+)"
	@echo "  nRF5 SDK 17.1.0"
	@echo "  nrfutil 6.1+ (for DFU flashing)"
	@echo "  nrfjprog (for SWD flashing)"
	@echo "  mergehex (for combined flashing)"
