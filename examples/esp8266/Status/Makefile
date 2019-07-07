PROGRAM = dscStatus

# Include path to dscKeybusInterface-RTOS component.mk
EXTRA_COMPONENTS = $(abspath ../../../)

# Serial port settings
ESPPORT ?= /dev/ttyUSB0
ESPBAUD ?= 230400

# esp8266 flash size in megabits
FLASH_SIZE ?= 32  # NodeMCU/Wemos boards with 4MB flash

# Include path to esp-open-rtos common.mk
include $(SDK_PATH)/common.mk
