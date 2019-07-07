PROGRAM = dscHomeKit

# Include path to dscKeybusInterface-RTOS component.mk, esp-homekit, and other components
EXTRA_COMPONENTS = $(abspath ../../../) \
	extras/http-parser \
	$(abspath ../../../)/components/esp-wolfssl \
	$(abspath ../../../)/components/esp-cjson \
	$(abspath ../../../)/components/esp-homekit

# Serial port settings
ESPPORT ?= /dev/ttyUSB0
ESPBAUD ?= 230400

# esp8266 flash size in megabits
FLASH_SIZE ?= 32  # NodeMCU/Wemos boards with 4MB flash

# Include path to esp-open-rtos common.mk
include $(SDK_PATH)/common.mk

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS
