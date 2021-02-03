cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

#obj-y := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)*.c $(cur_dir)*.cpp $(cur_dir)*.S))
obj-y :=

ifeq ($(ROM_BUILD),1)
obj-y += ../../tests/rom/startup_ARMCM.S
else
obj-y += startup_main.S
endif

ifeq ($(FACTORY_SUITE),1)
obj-y += ../../tests/factory_suite/
else
ifeq ($(RELIABILITY_TEST),1)
obj-y += ../../tests/reliability_test/
else
ifeq ($(NOSTD),1)
asflags-y += -DNOSTD
obj-y += nostd_main.c
else
ifeq ($(NOAPP),1)
obj-y += noapp_main.cpp
else
obj-y += main.cpp
endif
endif
endif
endif

ifneq ($(filter 1,$(USB_SERIAL_TEST) $(USB_SERIAL_DIRECT_XFER_TEST) $(USB_AUDIO_TEST) $(I2C_TEST) $(AF_TEST) $(VD_TEST) $(CP_TEST) $(I2S_TDM_TEST) $(TDM_TEST) $(SE_TEST)),)
ccflags-y += -Itests/hwtest -DHWTEST

ifneq ($(filter 1,$(USB_SERIAL_TEST) $(USB_SERIAL_DIRECT_XFER_TEST) $(USB_AUDIO_TEST)),)
obj-y += ../../tests/hwtest/usb_dev_test.c
ifeq ($(USB_SERIAL_TEST),1)
ccflags-y += -DUSB_SERIAL_TEST
else ifeq ($(USB_SERIAL_DIRECT_XFER_TEST),1)
ccflags-y += -DUSB_SERIAL_DIRECT_XFER_TEST
else
ccflags-y += -DUSB_AUDIO_TEST
endif
endif

ifneq ($(USB_SERIAL_DIRECT_XFER_SIZE),)
ccflags-y += -DUSB_SERIAL_DIRECT_XFER_SIZE=$(USB_SERIAL_DIRECT_XFER_SIZE)
endif

ifeq ($(I2C_TEST),1)
obj-y += ../../tests/hwtest/i2c_test.c
ccflags-y += -DI2C_TEST

ifeq ($(I2C_TEST_DMA_MODE),1)
CFLAGS_i2c_test.o += -DI2C_TEST_DMA_MODE
endif
ifeq ($(I2C_TEST_INT_MODE),1)
CFLAGS_i2c_test.o += -DI2C_TEST_INT_MODE
endif
ifneq ($(I2C_SPEED),)
CFLAGS_i2c_test.o += -DI2C_SPEED=$(I2C_SPEED)
endif
endif # I2C_TEST

ifeq ($(AF_TEST),1)
obj-y += ../../services/audioflinger/  ../../tests/hwtest/af_test.c
ccflags-y += -Iservices/audioflinger -DAF_TEST

ifeq ($(AF_INT_CODEC_TEST),1)
obj-y += ../../platform/drivers/codec/
CFLAGS_af_test.o += -DAF_INT_CODEC_TEST
export AF_DEVICE_INT_CODEC ?= 1
export AF_DEVICE_I2S ?= 0
else
export AF_DEVICE_INT_CODEC ?= 0
export AF_DEVICE_I2S ?= 1
endif

ifneq ($(CHAN_NUM_CAPTURE),)
CFLAGS_af_test.o += -DCHAN_NUM_CAPTURE=$(CHAN_NUM_CAPTURE)
endif
ifneq ($(CHAN_SEP_BUF_CAPTURE),)
CFLAGS_af_test.o += -DCHAN_SEP_BUF_CAPTURE=$(CHAN_SEP_BUF_CAPTURE)
endif
ifeq ($(FIXED_BUF_CAPTURE),1)
CFLAGS_af_test.o += -DFIXED_BUF_CAPTURE
endif
endif # AF_TEST

ifeq ($(VD_TEST),1)
ccflags-y += -DVD_TEST -Iapps/voice_detector/
endif

ifeq ($(CP_TEST),1)
obj-y += ../../tests/hwtest/cp_test.c
ccflags-y += -DCP_TEST
endif

ifeq ($(I2S_TDM_TEST),1)
ccflags-y += -DI2S_TDM_TEST
obj-y += ../../tests/hwtest/i2s_tdm_test.c
endif #I2S_TDM_TEST

ifeq ($(TDM_TEST),1)
obj-y += ../../tests/hwtest/tdm_test.c
ccflags-y += -DTDM_TEST
ccflags-y += -DCHIP_HAS_I2S=$(CHIP_HAS_I2S)
endif #TDM_TEST

ifeq ($(SE_TEST),1)
obj-y += ../../tests/hwtest/se_aes_test.c
obj-y += ../../tests/hwtest/se_test.c
ccflags-y += -DSE_TEST
endif #SE_TEST

endif # hwtest cases


obj-y := $(obj-y:.c=.o)
obj-y := $(obj-y:.cpp=.o)
obj-y := $(obj-y:.S=.o)

obj-y += ../../utils/hwtimer_list/

ccflags-y +=  \
	-Iutils/hwtimer_list \
	-Iplatform/drivers/ana \
	-Iplatform/drivers/security_engine \
	-Iplatform/drivers/usb/usb_dev/inc \
	-Iapps/main \
	-Iapps/factory \
	-Iutils/list  \
	-Iapps/audioplayers  \
	-Iservices/bt_app  \
	-Iapps/common  \
	-Iservices/norflash_api  \
	-Iservices/nv_section/log_section

ifneq ($(FLASH_FILL),)
ccflags-y += -DFLASH_FILL=$(FLASH_FILL)
endif

ifneq ($(DEBUG_PORT),)
ccflags-y += -DDEBUG_PORT=$(DEBUG_PORT)
endif

ifneq ($(FLASH_SIZE),)
ccflags-y += -DFLASH_SIZE=$(FLASH_SIZE)
endif

ifeq ($(ROM_UTILS_ON),1)
ccflags-y += -DROM_UTILS_ON
endif

ifeq ($(NO_PMU),1)
ccflags-y += -DNO_PMU
endif

ifeq ($(NO_TIMER),1)
ccflags-y += -DNO_TIMER
endif

ifeq ($(SLEEP_TEST),1)
ccflags-y += -DSLEEP_TEST
endif

ifeq ($(RTOS),1)
ifeq ($(KERNEL),RTX)
ccflags-y += -Irtos/rtx/TARGET_CORTEX_M
endif
endif

ifeq ($(DEBUG_MODE_USB_DOWNLOAD),1)
ccflags-y += -DDEBUG_MODE_USB_DOWNLOAD
endif

