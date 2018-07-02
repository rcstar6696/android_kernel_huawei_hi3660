#ifndef _PCIE_KIRIN_COMMON_H
#define _PCIE_KIRIN_COMMON_H

#include "pcie-kirin.h"

#define INBOUNT_OFFSET 0x100

int config_enable_dbi(u32 rc_id, int flag);
int ltssm_enable(u32 rc_id, int yes);
int set_bme(u32 rc_id, int flag);
int set_mse(u32 rc_id, int flag);


void kirin_pcie_config_l0sl1(u32 rc_id, enum link_aspm_state aspm_state);
void kirin_pcie_config_l1ss(u32 rc_id, enum l1ss_ctrl_state enable);
void kirin_pcie_outbound_atu(u32 rc_id, int index,
		int type, u64 cpu_addr, u64 pci_addr, u32 size);
void kirin_pcie_inbound_atu(u32 rc_id, int index,
		int type, u64 cpu_addr, u64 pci_addr, u32 size);

int wlan_on(u32 rc_id, int on);

#ifdef CONFIG_KIRIN_PCIE_TEST
int retrain_link(u32 rc_id);
int set_link_speed(u32 rc_id, enum link_speed gen);
int show_link_speed(u32 rc_id);
u32 show_link_state(u32 rc_id);
u32 kirin_pcie_find_capability(struct pcie_port *pp, int cap);
#endif

#endif

