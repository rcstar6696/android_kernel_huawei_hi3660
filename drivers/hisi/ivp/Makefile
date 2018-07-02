# hisilicon ivp

ccflags-$(CONFIG_IVP_KIRIN970) += -Idrivers/hisi/ivp/kirin970
ccflags-$(CONFIG_IVP_HI3660) += -Idrivers/hisi/ivp/hi3660
ccflags-$(CONFIG_IVP_HI3650) += -Idrivers/hisi/ivp/hi3650

obj-$(CONFIG_IVP_SMMU)      += ivp_smmu.o
obj-$(CONFIG_HISI_IVP) += ivp_ipc.o
obj-$(CONFIG_HISI_IVP) += ivp_core.o
obj-$(CONFIG_IVP_KIRIN970) += kirin970/
obj-$(CONFIG_IVP_HI3660) += hi3660/
obj-$(CONFIG_IVP_HI3650) += hi3650/
