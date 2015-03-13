LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPP_FEATURES += exceptions

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
	$(LOCAL_PATH)/../SDL2_image \
	$(LOCAL_PATH)/../SDL2_mixer \
	$(LOCAL_PATH)/../SDL2_net \
	$(LOCAL_PATH)/../SDL2_ttf \
	$(LOCAL_PATH)/../../../src/include

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	../../../src/client.cpp \
	../../../src/parser_libcards.cpp \
	../../../src/parser_libnet.cpp \
	../../../src/parser.cpp \
	../../../src/data_filedb.cpp \
	../../../src/parser_lib.cpp \
	../../../src/tools.cpp \
	../../../src/carddata.cpp \
	../../../src/xml_parser.cpp \
	../../../src/security.cpp \
	../../../src/data.cpp \
	../../../src/localization.cpp \
	../../../src/driver.cpp \
	../../../src/game.cpp \
	../../../src/interpreter.cpp \
	../../../src/SDL_rotozoom.cpp \
	../../../src/sdl_driver.cpp \
	../../../src/game-sdl-version.cpp

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_mixer SDL2_net SDL2_ttf

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
