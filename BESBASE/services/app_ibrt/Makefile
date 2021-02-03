
cur_dir := $(dir $(lastword $(MAKEFILE_LIST)))

obj_s := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.s))
obj_c := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.c))

obj_cpp := $(patsubst $(cur_dir)%,%,$(wildcard $(cur_dir)src/*.cpp))

src_obj := $(obj_c:.c=.o) $(obj_s:.S=.o) $(obj_cpp:.cpp=.o)

IBRT_LIB_NAME := libtws_ibrt_ui

$(IBRT_LIB_NAME)-y := $(src_obj)

obj-y :=  $(IBRT_LIB_NAME).a

subdir-ccflags-y += \
	-Iservices/fs/fat \
	-Iservices/fs/sd \
	-Iservices/fs/fat/ChaN \
	-Iservices/overlay \
	-Iservices/nvrecord \
	-Iservices/resources \
	-Iservices/multimedia/audio/process/resample/include \
	$(BT_IF_INCLUDES) \
	-Iservices/nv_section/factory_section \
	-Iplatform/drivers/uarthci \
	-Iplatform/drivers/ana \
	-Iplatform/drivers/bt \
	-Iutils/cqueue \
	-Iutils/heap \
	-Iservices/audioflinger \
	-Iutils/lockcqueue \
	-Iutils/intersyshci \
	-Iapps/key \
	-Iapps/main \
	-Iapps/common \
	-Iapps/audioplayers \
	-Iapps/audioplayers/a2dp_decoder \
	-Iapps/anc/inc \
	-Iapps/factory \
	-Iservices/ble_app \
	-Iservices/ble_app \
	-Iservices/interconnection/green \
	-Iutils/hwtimer_list \
	-Ithirdparty/audio_codec_lib/liblhdc-dec/inc \
	$(BT_PROFILES_INCLUDES) \
	-Iservices/ble_stack/common/api \
	-Iservices/ble_app/app_tws \
	-Iservices/nv_section/log_section \
	-Iservices/ai_voice/manager \
	-Iapps/battery/ \
	-Iutils/crc16 \
	-Iutils/crc32 \
	-Iutils/rom_utils \
	-Iservices/multimedia/audio/codec/sbc/inc \
	-Iservices/multimedia/audio/codec/sbc/src/inc \
	-Iservices/bt_app/a2dp_codecs/include \
	-Iservices/voicepath \
	-Iservices/voicepath/gsound/gsound_target \
	-Iservices/ble_app/app_main \
	-Iservices/app_tws/inc

ifeq ($(IBRT),1)
subdir-ccflags-y += \
	-Iservices/tws_ibrt/inc \
	-Iservices/app_ibrt/inc \
	-Iservices/bt_app
endif

ifeq ($(IBRT_OTA),1)
subdir-ccflags-y += \
    -Iservices/ibrt_ota/inc
endif

ifeq ($(ANC_APP),1)
CFLAGS_app_ibrt_ui_test.o += -DANC_APP
endif
