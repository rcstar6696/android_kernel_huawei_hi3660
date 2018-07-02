#
# Makefile for the vl53L0 drivers.
#

# Each configuration option enables a list of files.

ccflags-y	+= -Idrivers/media/huawei/camera/laser/vl53L0/inc -Idrivers/media/huawei/camera/laser/vl53L0 -Idrivers/media/huawei/camera -DSTM_TEST
obj-$(CONFIG_HISI_CAMERA_VL53L0_970)               += stmvl53l0.o
stmvl53l0-objs				+= gp2ap01vt.o hw_laser_i2c.o stmvl53l0_module.o src/vl53l0_api_calibration.o
stmvl53l0-objs				+= src/vl53l0_api_core.o src/vl53l0_api_histogram.o src/vl53l0_api_ranging.o
stmvl53l0-objs				+= src/vl53l0_api_strings.o src/vl53l0_api.o src/vl53l0_platform.o
stmvl53l0-objs				+= src/vl53l0_i2c_platform.o src/vl53l0_port_i2c.o
