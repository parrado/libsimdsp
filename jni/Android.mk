ifneq ($(TARGET_SIMULATOR),true)



LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -ggdb -Wall -fPIC -mfloat-abi=softfp -mfpu=neon 



LOCAL_LDLIBS := -L$(LOCAL_PATH)/lib -llog -lOpenSLES -g -pie 

LOCAL_C_INCLUDES := bionic
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_SRC_FILES:=  sharedmem.cpp sdcore.cpp main.cpp

LOCAL_MODULE := simdsp


include $(BUILD_SHARED_LIBRARY)

endif  # TARGET_SIMULATOR != true
