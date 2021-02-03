cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

obj-y += $(CHIP)/

subdir-ccflags-y += -Iservices/nvrecord -Iutils/cqueue -Iutils/heap

ifeq ($(BT_XTAL_SYNC),1)
subdir-ccflags-y += -DBT_XTAL_SYNC
endif

export BT_DRV_UART_BRIDGE_FLAGS :=
ifneq ($(DEBUG_PORT),)
BT_DRV_UART_BRIDGE_FLAGS += -DDEBUG_PORT=$(DEBUG_PORT)
endif

export BT_DRV_FLAGS :=
ifeq ($(NO_SLEEP),1)
BT_DRV_FLAGS += -DNO_SLEEP
endif

export BT_DRV_CONFIG_FLAGS :=
ifeq ($(INTERSYS_NO_THREAD),1)
BT_DRV_CONFIG_FLAGS += -D_INTERSYS_NO_THREAD_
endif

VCO_TEST_TOOL ?= 1
ifeq ($(VCO_TEST_TOOL),1)
subdir-ccflags-y += -DVCO_TEST_TOOL
endif

LAURENT_ALGORITHM ?= 0
ifeq ($(LAURENT_ALGORITHM),1)
subdir-ccflags-y += -DLAURENT_ALGORITHM
endif

FORCE_SCO_MAX_RETX ?= 0
ifeq ($(FORCE_SCO_MAX_RETX),1)
subdir-ccflags-y += -D__FORCE_SCO_MAX_RETX__
endif

CONTROLLER_DUMP_ENABLE ?= 0
ifeq ($(CONTROLLER_DUMP_ENABLE),1)
subdir-ccflags-y += -DCONTROLLER_DUMP_ENABLE=1
endif

FA_RX_GAIN_CTRL ?= 0
ifeq ($(FA_RX_GAIN_CTRL),1)
subdir-ccflags-y += -D__FA_RX_GAIN_CTRL__
endif

ifeq ($(BT_FA_ECC),1)
subdir-ccflags-y += -D__FASTACK_ECC_ENABLE__
endif

ifeq ($(BT_EXT_LNA_PA),1)
KBUILD_CPPFLAGS += -DBT_EXT_LNA_PA
ifeq ($(BT_EXT_LNA),1)
KBUILD_CPPFLAGS += -DBT_EXT_LNA
endif

ifeq ($(BT_EXT_PA),1)
KBUILD_CPPFLAGS += -DBT_EXT_PA
endif
endif

ifneq ($(BT_DEBUG_TPORTS),0)
subdir-ccflags-y += -DTPORT_TYPE=$(BT_DEBUG_TPORTS)
endif

export CONTROLLER_DUMP_ENABLE ?= 0
ifeq ($(CONTROLLER_DUMP_ENABLE),1)
	KBUILD_CPPFLAGS += -DCONTROLLER_DUMP_ENABLE=1
endif

export CONTROLLER_MEM_LOG_ENABLE ?= 0
ifeq ($(CONTROLLER_MEM_LOG_ENABLE),1)
	KBUILD_CPPFLAGS += -DCONTROLLER_MEM_LOG_ENABLE=1
endif

