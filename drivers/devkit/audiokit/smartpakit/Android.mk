# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := smartpakit
LOCAL_SOURCE_FOLDER := $(shell pwd)/$(LOCAL_PATH)
LOCAL_TARGET_FOLDER := $(KERNEL_OUT)/external_modules/extra
LOCAL_INSTALL_MODULE := $(LOCAL_MODULE).ko $(LOCAL_MODULE)_i2c.ko

include $(BUILD_KERNEL_MODULES)
