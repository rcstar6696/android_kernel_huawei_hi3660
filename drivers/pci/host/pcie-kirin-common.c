#include "pcie-kirin-common.h"

/*lint -e618  -e826 -e648 -e438 -e550 -e713 -e732 -e737 -e774 -e838 -esym(618,*) -esym(826,*) -esym(648,*) -esym(438,*) -esym(550,*) -esym(713,*) -esym(732,*) -esym(737,*) -esym(774,*) -esym(838,*) */

/**
 * config_enable_dbi - make it possible to access the rc configuration registers in the CDM,
 * or the ep configuration registers.
 * @flag: If flag equals 0, you can access the ep configuration registers in the CDM;
 *  If not, you can access the rc configuration registers in the CDM.
 */

int kirin_pcie_valid(u32 rc_id)
{
	if (rc_id >= g_rc_num) {
		PCIE_PR_ERR("There is no rc_id = %d", rc_id);
		return 0;
	}

	if (!atomic_read(&(g_kirin_pcie[rc_id].is_power_on))) {
		PCIE_PR_ERR("PCIe%d is power off ", rc_id);
		return 0;
	}

	return -1;
}

int config_enable_dbi(u32 rc_id, int flag)
{
	u32 ret1;
	u32 ret2;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return -1;

	pcie = &g_kirin_pcie[rc_id];

	ret1 = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL0_ADDR);
	ret2 = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL1_ADDR);
	if (flag) {
		ret1 = ret1 | PCIE_ELBI_SLV_DBI_ENABLE;
		ret2 = ret2 | PCIE_ELBI_SLV_DBI_ENABLE;
	} else {
		ret1 = ret1 & (~PCIE_ELBI_SLV_DBI_ENABLE);
		ret2 = ret2 & (~PCIE_ELBI_SLV_DBI_ENABLE);
	}
	kirin_elb_writel(pcie, ret1, SOC_PCIECTRL_CTRL0_ADDR);
	kirin_elb_writel(pcie, ret2, SOC_PCIECTRL_CTRL1_ADDR);

	udelay(2);//lint !e778  !e774  !e516 !e747
	ret1 = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL0_ADDR);
	ret2 = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL1_ADDR);

	PCIE_PR_INFO("PCIeCTRL apb register 0x0=[0x%x], 0x4=[0x%x]", ret1, ret2);

	return 0;
}
/**
 * set_bme - enable bus master or not.
 * @flag: If flag equals 0, bus master is disabled. If not, bus master is enabled.
 */
int set_bme(u32 rc_id, int flag)
{
	int ret;
	struct pcie_port *pp;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return -1;

	pcie = &g_kirin_pcie[rc_id];

	pp = &(pcie->pp);

	config_enable_dbi(rc_id, ENABLE);

	ret = readl(pp->dbi_base + PCI_COMMAND);
	if (flag) {
		PCIE_PR_INFO("Enable Bus master!!!");
		ret |= PCI_COMMAND_MASTER;
	} else {
		PCIE_PR_INFO("Disable Bus master!!!");
		ret &= ~PCI_COMMAND_MASTER;/* [false alarm]:fortify */
	}
	writel(ret, pp->dbi_base + PCI_COMMAND);

	udelay(5);//lint !e778  !e774  !e516 !e747
	ret = readl(pp->dbi_base + PCI_COMMAND);
	PCIE_PR_INFO("Register[0x4] value is [0x%x]", ret);

	config_enable_dbi(rc_id, DISABLE);

	return 0;
}

/**
 * set_mse - enable mem space or not.
 * @flag: If flag equals 0, mem space is disabled. If not, mem space is enabled.
 */
int set_mse(u32 rc_id, int flag)
{
	int ret;
	struct pcie_port *pp;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return -1;

	pcie = &g_kirin_pcie[rc_id];

	pp = &(pcie->pp);

	config_enable_dbi(rc_id, ENABLE);

	ret = readl(pp->dbi_base + PCI_COMMAND);
	if (flag) {
		PCIE_PR_INFO("Enable MEM space!!!");
		ret |= PCI_COMMAND_MEMORY;
	} else {
		PCIE_PR_INFO("Disable MEM space!!!");
		ret &= ~PCI_COMMAND_MEMORY;/* [false alarm]:fortify */
	}
	writel(ret, pp->dbi_base + PCI_COMMAND);

	udelay(5);//lint !e778  !e774  !e516 !e747
	ret = readl(pp->dbi_base + PCI_COMMAND);
	PCIE_PR_INFO("Register[0x4] value is [0x%x]", ret);

	config_enable_dbi(rc_id, DISABLE);

	return 0;
}

int ltssm_enable(u32 rc_id, int yes)
{
	u32 val;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return -1;

	pcie = &g_kirin_pcie[rc_id];

	if (yes) {
		val = kirin_elb_readl(pcie,  SOC_PCIECTRL_CTRL7_ADDR);
		val |= PCIE_LTSSM_ENABLE_BIT;
		kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL7_ADDR);
	} else {
		val = kirin_elb_readl(pcie,  SOC_PCIECTRL_CTRL7_ADDR);
		val &= ~PCIE_LTSSM_ENABLE_BIT;
		kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL7_ADDR);
	}

	return 0;
}

void kirin_pcie_config_l0sl1(u32 rc_id, enum link_aspm_state aspm_state)
{
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return;

	pcie = &g_kirin_pcie[rc_id];

	if (!pcie->rc_dev || !pcie->ep_dev) {
		PCIE_PR_ERR("Failed to get dev ");
		return;
	}

	if (aspm_state == ASPM_CLOSE) {
		pcie_capability_clear_and_set_word(pcie->ep_dev, PCI_EXP_LNKCTL,
						   PCI_EXP_LNKCTL_ASPMC, aspm_state);

		pcie_capability_clear_and_set_word(pcie->rc_dev, PCI_EXP_LNKCTL,
					PCI_EXP_LNKCTL_ASPMC, aspm_state);
	} else {
		pcie_capability_clear_and_set_word(pcie->rc_dev, PCI_EXP_LNKCTL,
						   PCI_EXP_LNKCTL_ASPMC, aspm_state);

		pcie_capability_clear_and_set_word(pcie->ep_dev, PCI_EXP_LNKCTL,
					PCI_EXP_LNKCTL_ASPMC, aspm_state);
	}
}

static void enable_req_clk(struct kirin_pcie *pcie, u32 enable_flag)
{
	u32 val;

	val = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL1_ADDR);

	if (enable_flag)
		val &= ~PCIE_APB_CLK_REQ;
	else
		val |= PCIE_APB_CLK_REQ;

	kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL1_ADDR);
}

void kirin_pcie_config_l1ss(u32 rc_id, enum l1ss_ctrl_state enable)
{
	u32 reg_val;
	int rc_l1ss_pm, ep_l1ss_pm, ep_ltr_pm;
	struct kirin_pcie *pcie;
	struct kirin_pcie_dtsinfo *dtsinfo;

	if (!kirin_pcie_valid(rc_id))
		return;

	pcie = &g_kirin_pcie[rc_id];

	dtsinfo = &pcie->dtsinfo;

	if ((pcie->rc_dev == NULL) || (pcie->ep_dev == NULL)) {
		PCIE_PR_ERR("Failed to get RC_dev or EP_dev ");
		return;
	}

	PCIE_PR_DEBUG("Get RC PCI_EXT_L1SS_CAP_ID");
	rc_l1ss_pm = pci_find_ext_capability(pcie->rc_dev, PCI_EXT_L1SS_CAP_ID);
	if (!rc_l1ss_pm) {
		PCIE_PR_ERR("Failed to get RC PCI_EXT_L1SS_CAP_ID");
		return;
	}

	PCIE_PR_DEBUG("Get EP PCI_EXT_L1SS_CAP_ID");
	ep_l1ss_pm = pci_find_ext_capability(pcie->ep_dev, PCI_EXT_L1SS_CAP_ID);
	if (!ep_l1ss_pm) {
		PCIE_PR_ERR("Failed to get EP PCI_EXT_L1SS_CAP_ID");
		return;
	}

	pcie_capability_read_dword(pcie->ep_dev, PCI_EXP_DEVCTL2, &reg_val);
	reg_val &= ~(0x1 << 10);
	pcie_capability_write_dword(pcie->ep_dev, PCI_EXP_DEVCTL2, reg_val);

	pcie_capability_read_dword(pcie->rc_dev, PCI_EXP_DEVCTL2, &reg_val);
	reg_val &= ~(0x1 << 10);
	pcie_capability_write_dword(pcie->rc_dev, PCI_EXP_DEVCTL2, reg_val);


	/*disble L1ss*/
	pci_read_config_dword(pcie->ep_dev, ep_l1ss_pm + PCI_EXT_L1SS_CTRL1, &reg_val);
	reg_val &= ~L1SS_PM_ALL;
	pci_write_config_dword(pcie->ep_dev, ep_l1ss_pm + PCI_EXT_L1SS_CTRL1, reg_val);

	pci_read_config_dword(pcie->rc_dev, rc_l1ss_pm + PCI_EXT_L1SS_CTRL1, &reg_val);
	reg_val &= ~L1SS_PM_ALL;
	pci_write_config_dword(pcie->rc_dev, rc_l1ss_pm + PCI_EXT_L1SS_CTRL1, reg_val);

	if (enable) {
		enable_req_clk(pcie, DISABLE);

		/*RC: Power On Value & Scale*/
		if (dtsinfo->ep_l1ss_ctrl2) {
			pci_read_config_dword(pcie->rc_dev, rc_l1ss_pm + PCI_EXT_L1SS_CTRL2, &reg_val);
			reg_val &= ~0xFF;
			reg_val |= dtsinfo->ep_l1ss_ctrl2;
			pci_write_config_dword(pcie->rc_dev, rc_l1ss_pm + PCI_EXT_L1SS_CTRL2, reg_val);
		}

		pcie_capability_read_dword(pcie->rc_dev, PCI_EXP_DEVCTL2, &reg_val);
		reg_val |= (0x1 << 10);
		pcie_capability_write_dword(pcie->rc_dev, PCI_EXP_DEVCTL2, reg_val);

		/*EP: Power On Value & Scale*/
		if (dtsinfo->ep_l1ss_ctrl2) {
			pci_read_config_dword(pcie->ep_dev, ep_l1ss_pm+ PCI_EXT_L1SS_CTRL2, &reg_val);
			reg_val &= ~0xFF;
			reg_val |= dtsinfo->ep_l1ss_ctrl2;
			pci_write_config_dword(pcie->ep_dev, ep_l1ss_pm + PCI_EXT_L1SS_CTRL2, reg_val);
		}

		/*EP: LTR Latency*/
		if (dtsinfo->ep_ltr_latency) {
			ep_ltr_pm= pci_find_ext_capability(pcie->ep_dev, PCI_EXT_LTR_CAP_ID);
			if (ep_ltr_pm)
				pci_write_config_dword(pcie->ep_dev, ep_ltr_pm + LTR_MAX_SNOOP_LATENCY, dtsinfo->ep_ltr_latency);
		}

		pcie_capability_read_dword(pcie->ep_dev, PCI_EXP_DEVCTL2, &reg_val);
		reg_val |= (0x1 << 10);
		pcie_capability_write_dword(pcie->ep_dev, PCI_EXP_DEVCTL2, reg_val);

		/*Enable*/
		pci_read_config_dword(pcie->rc_dev, rc_l1ss_pm + PCI_EXT_L1SS_CTRL1, &reg_val);
		reg_val = dtsinfo->l1ss_ctrl1;
		reg_val &= 0xFFFFFFF0;
		reg_val |= enable;
		pci_write_config_dword(pcie->rc_dev, rc_l1ss_pm + PCI_EXT_L1SS_CTRL1, reg_val);

		pci_read_config_dword(pcie->ep_dev, ep_l1ss_pm + PCI_EXT_L1SS_CTRL1, &reg_val);
		reg_val = dtsinfo->l1ss_ctrl1;
		reg_val &= 0xFFFFFFF0;
		reg_val |= enable;
		pci_write_config_dword(pcie->ep_dev, ep_l1ss_pm + PCI_EXT_L1SS_CTRL1, reg_val);

	} else {
		enable_req_clk(pcie, ENABLE);
	}

}

static void set_atu_addr(struct pcie_port *pp, int type, char *dbi_base,
			u64 src_addr, u64 dst_addr, u32 size)
{
	kirin_pcie_writel_rc(pp, lower_32_bits(src_addr), dbi_base + PCIE_ATU_LOWER_BASE);
	kirin_pcie_writel_rc(pp, upper_32_bits(src_addr), dbi_base + PCIE_ATU_UPPER_BASE);
	kirin_pcie_writel_rc(pp, lower_32_bits(src_addr + size - 1),
			  dbi_base + PCIE_ATU_LIMIT);
	kirin_pcie_writel_rc(pp, lower_32_bits(dst_addr), dbi_base + PCIE_ATU_LOWER_TARGET);
	kirin_pcie_writel_rc(pp, upper_32_bits(dst_addr), dbi_base + PCIE_ATU_UPPER_TARGET);
	kirin_pcie_writel_rc(pp, type, dbi_base + PCIE_ATU_CR1);
	kirin_pcie_writel_rc(pp, (u32)PCIE_ATU_ENABLE, dbi_base + PCIE_ATU_CR2);//lint !e648
}

static void kirin_pcie_atu_cfg(struct pcie_port *pp, int index, int direct,
		int type, u64 src_addr, u64 dst_addr, u32 size)
{
	struct kirin_pcie *pcie;
	char *dbi_base;
	unsigned int base_addr;

	pcie = to_kirin_pcie(pp);//lint !e826
	if (!atomic_read(&(pcie->is_power_on))) {
		PCIE_PR_ERR("PCIe is power off");
		return;
	}

	dbi_base = pp->dbi_base;
	base_addr = pcie->dtsinfo.iatu_base_offset;

	if (direct == (int)PCIE_ATU_REGION_OUTBOUND)//lint !e648)
		index &= 0x7FFFFFFF;
	else
		index |= direct;

	if (base_addr != PCIE_ATU_VIEWPORT) {
		base_addr += index * 0x200;
		if (direct == (int)PCIE_ATU_REGION_INBOUND)//lint !e648
			base_addr += INBOUNT_OFFSET;
	} else
		kirin_pcie_writel_rc(pp, index, dbi_base + PCIE_ATU_VIEWPORT);//lint !e648

	dbi_base += base_addr;

	set_atu_addr(pp, type, dbi_base, src_addr, dst_addr, size);
}

void dw_pcie_prog_outbound_atu(struct pcie_port *pp, int index,
		int type, u64 cpu_addr, u64 pci_addr, u32 size)
{
	kirin_pcie_atu_cfg(pp, index, PCIE_ATU_REGION_OUTBOUND,
					type, cpu_addr, pci_addr, size);//lint !e648
}

void kirin_pcie_outbound_atu(u32 rc_id, int index,
		int type, u64 cpu_addr, u64 pci_addr, u32 size)
{
	struct pcie_port *pp;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return;

	pcie = &g_kirin_pcie[rc_id];
	pp = &(pcie->pp);

	kirin_pcie_atu_cfg(pp, index, PCIE_ATU_REGION_OUTBOUND,
					type, cpu_addr, pci_addr, size); //lint !e648
}

void kirin_pcie_inbound_atu(u32 rc_id, int index,
		int type, u64 cpu_addr, u64 pci_addr, u32 size)
{
	struct pcie_port *pp;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return;

	pcie = &g_kirin_pcie[rc_id];
	pp = &pcie->pp;

	kirin_pcie_atu_cfg(pp, index, PCIE_ATU_REGION_INBOUND,
					type, pci_addr, cpu_addr, size);
}

int wlan_on(u32 rc_id, int on)
{
	int ret;
	u32 wl_power;
	struct device_node *np;
	struct kirin_pcie *pcie;

	if (rc_id >= g_rc_num) {
		PCIE_PR_ERR("There is no rc_id = %d", rc_id);
		return -EINVAL;
	}

	pcie = &g_kirin_pcie[rc_id];

	if (!pcie->pp.dev) {
		PCIE_PR_ERR("NO PCIe device");
		return -1;
	}

	np = pcie->pp.dev->of_node;
	if (np) {
		if (!(of_property_read_u32(np, "wl_power", &wl_power))) {
			PCIE_PR_INFO("WL Power On Number is [%d] ", wl_power);
		} else {
			PCIE_PR_INFO("Failed to get wl_power info");
			return -1;
		}
	} else {
		PCIE_PR_INFO("Failed to get kirin_pcie info");
		return -1;
	}

	if (!wl_power) {
		PCIE_PR_ERR("wl_power is invalid");
		return -1;
	}

	ret = gpio_request(wl_power, "wl_on");
	if (ret != 0) {
		PCIE_PR_ERR("Failed to request wl_on gpio");
		return -1;
	}
	if (on) {
		PCIE_PR_INFO("Power on Wlan");
		gpio_direction_output(wl_power, 1);
		mdelay(200);//lint !e778  !e774  !e516  !e747  !e845
	} else {
		PCIE_PR_INFO("Power down Wlan");
		gpio_direction_output(wl_power, 0);
		mdelay(100);//lint !e778  !e774  !e516  !e747  !e845
	}
	gpio_free(wl_power);

	return 0;
}

int kirin_pcie_power_on(struct pcie_port *pp, enum rc_power_status on_flag)
{
	int ret;
	struct kirin_pcie *pcie = to_kirin_pcie(pp);

	PCIE_PR_DEBUG("++");

	/*power on*/
	if (on_flag == RC_POWER_ON || on_flag == RC_POWER_RESUME) {
		spin_lock(&pcie->ep_ltssm_lock);
		pcie->ep_link_status = DEVICE_LINK_UP;
		spin_unlock(&pcie->ep_ltssm_lock);

		ret = kirin_pcie_turn_on(pp, on_flag);

	} else if (on_flag == RC_POWER_OFF || on_flag == RC_POWER_SUSPEND) {

		ret = kirin_pcie_turn_off(pp, on_flag);

	} else {
		PCIE_PR_ERR("Invalid Param");
		ret = -1;
	}

	PCIE_PR_DEBUG("--");
	return ret;
}

#ifdef CONFIG_KIRIN_PCIE_TEST

int retrain_link(u32 rc_id)
{
	u32 val;
	unsigned long start_jiffies;
	u32 cap_pos;
	struct pcie_port *pp;

	if (!kirin_pcie_valid(rc_id))
		return -1;

	pp = &(g_kirin_pcie[rc_id].pp);

	cap_pos = kirin_pcie_find_capability(pp, PCI_CAP_ID_EXP);
	if (!cap_pos)
		return -1;

	kirin_pcie_rd_own_conf(pp, (int)cap_pos + PCI_EXP_LNKCTL2, 4, &val);
	/* Retrain link */
	val |= PCI_EXP_LNKCTL_RL;
	kirin_pcie_wr_own_conf(pp, (int)cap_pos + PCI_EXP_LNKCTL2, 4, val);

	/* Wait for link training end. Break out after waiting for timeout */
	start_jiffies = jiffies;
	for (;;) {
		kirin_pcie_rd_own_conf(pp, (int)cap_pos + PCI_EXP_LNKSTA, 4, &val);
		val &= 0xffff;
		if (!(val & PCI_EXP_LNKSTA_LT))
			break;
		if (time_after(jiffies, start_jiffies + HZ))
			break;
		msleep(1);
	}
	if (!(val & PCI_EXP_LNKSTA_LT))
		return 0;
	return -1;
}

int set_link_speed(u32 rc_id, enum link_speed gen)
{
	u32 val = 0x1;
	u32 reg_val;
	int ret = 0;
	u32 cap_pos;
	struct pcie_port *pp;

	if (!kirin_pcie_valid(rc_id))
		return -1;

	pp = &(g_kirin_pcie[rc_id].pp);

	switch (gen) {
	case GEN1:
		val = 0x1;
		break;
	case GEN2:
		val = 0x2;
		break;
	case GEN3:
		val = 0x3;
		break;
	default:
		ret = -1;
	}

	cap_pos = kirin_pcie_find_capability(pp, PCI_CAP_ID_EXP);
	if (!cap_pos)
		return -1;

	kirin_pcie_rd_own_conf(pp, (int)cap_pos + PCI_EXP_LNKCTL2, 4, &reg_val);
	reg_val &= ~(0x3 << 0);
	reg_val |= val;
	kirin_pcie_wr_own_conf(pp, (int)cap_pos + PCI_EXP_LNKCTL2, 4, reg_val);

	if (!ret)
		return retrain_link(rc_id);
	return ret;
}

int show_link_speed(u32 rc_id)
{
	unsigned int val;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return -1;

	pcie = &g_kirin_pcie[rc_id];

	val = kirin_elb_readl(pcie, SOC_PCIECTRL_STATE4_ADDR);
	val = val & 0xc0;
	switch (val) {
	case 0x0:
		PCIE_PR_INFO("Link speed: gen1");
		break;
	case 0x40:
		PCIE_PR_INFO("Link speed: gen2");
		break;
	case 0x80:
		PCIE_PR_INFO("Link speed: gen3");
		break;
	default:
		PCIE_PR_INFO("Link speed info unknow");
	}

	return val;
}

/**
 * show_link_state - show the rc link state.
 */
u32 show_link_state(u32 rc_id)
{
	unsigned int val;
	struct kirin_pcie *pcie;

	if (!kirin_pcie_valid(rc_id))
		return 0xFFFFFFFF;

	pcie = &g_kirin_pcie[rc_id];

	val = kirin_elb_readl(pcie, SOC_PCIECTRL_STATE4_ADDR);

	PCIE_PR_INFO("Register 0x410 value [0x%x]", val);

	val = val & 0x3f;

	switch (val) {
	case 0x3:
		PCIE_PR_INFO("L-state: Compliance");
		break;
	case 0x11:
		PCIE_PR_INFO("L-state: L0");
		break;
	case 0x12:
		PCIE_PR_INFO("L-state: L0S");
		break;
	case 0x14: {
		val = kirin_elb_readl(pcie, SOC_PCIECTRL_STATE5_ADDR);
		PCIE_PR_INFO("Register 0x414 value [0x%x]", val);
		val = val & 0xC000;
		if (val == 0x4000)
			PCIE_PR_ERR("L-state: L1.1");
		else if (val == 0xC000)
			PCIE_PR_ERR("L-state: L1.2");
		else {
			PCIE_PR_ERR("L-state: L1.0");
			val = 0x14;
		}
		break;
	}
	case 0x15:
		PCIE_PR_INFO("L-state: L2");
		break;
	case 0x1B:
		PCIE_PR_INFO("L-state: LoopBack");
		break;
	default:
		val = 0x0;
		PCIE_PR_ERR("Other state");
	}
	return val;
}

u32 kirin_pcie_find_capability(struct pcie_port *pp, int cap)
{
	u8 id;
	int ttl = 0x30;
	u32 pos = 0;
	u32 ent = 0;

	kirin_pcie_rd_own_conf(pp, PCI_CAPABILITY_LIST, 1, &pos);

	while (ttl--) {
		if (pos < 0x40)
			break;
		pos &= ~3;
		kirin_pcie_rd_own_conf(pp, pos, 2, &ent);
		id = ent & 0xff;
		if (id == 0xff)
			break;
		if (id == cap)
			return pos;
		pos = (ent >> 8);
	}
	return 0;
}


#endif
/*lint -e618  -e826 -e648 -e438 -e550 -e713 -e732 -e737 -e774 -e838 -esym(618,*) -esym(826,*) -esym(648,*) -esym(438,*) -esym(550,*) -esym(713,*) -esym(732,*) -esym(737,*) -esym(774,*) -esym(838,*) */


