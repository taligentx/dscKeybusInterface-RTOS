PROGRAM = dscHomeKit

# Include path to dscKeybusInterface-RTOS component.mk, esp-homekit, and other components
EXTRA_COMPONENTS = $(abspath ../../../) \
	extras/http-parser \
	$(abspath ../../../)/components/esp-wolfssl \
	$(abspath ../../../)/components/esp-cjson \
	$(abspath ../../../)/components/esp-homekit

# Include path to esp-open-rtos common.mk
include $(SDK_PATH)/common.mk

FLASH_SIZE ?= 32

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS
