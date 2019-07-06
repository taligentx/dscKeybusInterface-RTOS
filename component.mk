# Component makefile for dscKeybusInterface-RTOS

INC_DIRS += $(dscKeybusInterface-RTOS_ROOT)src

# args for passing into compile rule generation
dscKeybusInterface-RTOS_SRC_DIR = $(dscKeybusInterface-RTOS_ROOT)src

$(eval $(call component_compile_rules,dscKeybusInterface-RTOS))
