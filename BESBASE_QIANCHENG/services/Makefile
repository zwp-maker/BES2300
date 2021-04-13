obj-y := audio_dump/ \
         audioflinger/ \
         audio_process/ \
         hw_dsp/ \
         bt_app/ \
         overlay/ \
         resources/ \
         ../utils/crc32/ \
         ../utils/heap/ \
         osif/ \
         norflash_api/

ifeq ($(BONE_SENSOR_TDM),1)
obj-y += bone_sensor/
endif

ifeq ($(INTERCONNECTION),1)
obj-y += interconnection/
else ifeq ($(AI_VOICE),1)
obj-y += interconnection/
endif

ifeq ($(ENHANCED_STACK),1)
obj-y += bt_profiles_enhanced/
obj-y += bt_if_enhanced/
else
obj-y += bt_profiles/
obj-y += bt_if/
endif

ifeq ($(MBED),1)
obj-y += fs/
endif

ifneq ($(MBED)_$(AI_VOICE),0_0)
obj-y +=../utils/kfifo/
endif

ifeq ($(VOICE_DATAPATH_ENABLED),1)
obj-y += voicepath/
obj-y += voicepath/$(VOICE_DATAPATH_TYPE)/
endif

ifeq ($(BLE),1)
obj-y += ble_app/ \
         bridge/ \
         ble_stack/ \
         ../utils/retention_ram/ \
         ble_profiles/
endif

ifeq ($(AUTO_TEST),1)
obj-y += auto_test/
endif

obj-y += nv_section/

ifneq ($(FPGA),1)
obj-y += nvrecord/
endif

ifeq ($(VOICE_RECOGNITION),1)
obj-y += voice_recognition/
endif

ifeq ($(APP_ANC_TEST),1)
obj-y += anc_spp_tool/
endif

ifeq ($(TEST_OVER_THE_AIR),1)
obj-y += tota/
endif

ifeq ($(BES_OTA_BASIC),1)
ifeq ($(IBRT_OTA),1)
obj-y += ibrt_ota/
else
obj-y += ota/
endif
endif

ifeq ($(TILE_DATAPATH_ENABLED),1)
obj-y += ../thirdparty/tile/
endif

ifeq ($(AI_VOICE),1)
obj-y += ai_voice/ \
         ../utils/crc16/
endif

ifeq ($(CHIP_HAS_CP),1)
obj-y += cp_accel/
endif

ifeq ($(IBRT),1)
obj-y += tws_ibrt/
obj-y += app_ibrt/
endif

ifeq ($(TWS_SYSTEM_ENABLED),1)
obj-y += app_tws/
endif

obj-y += communication/
