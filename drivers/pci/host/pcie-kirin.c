/*
 * PCIe host controller driver for Kirin SoCs
 *
 * Copyright (C) 2015 Hilisicon Electronics Co., Ltd.
 *		http://www.huawei.com
 *
 * Author: Xiaowei Song <songxiaowei@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "pcie-kirin.h"
#include "pcie-kirin-common.h"
/*lint -e438 -e550 -e713 -e732 -e737 -e774 -e838  -esym(438,*) -esym(550,*) -esym(713,*) -esym(732,*) -esym(737,*) -esym(774,*) -esym(838,*) */
unsigned int g_rc_num;

struct kirin_pcie g_kirin_pcie[] = {
	{
		.irq = {
				{
					.name = "kirin-pcie0-inta",
				},
				{
					.name = "kirin-pcie0-msi",
				},
				{
					.name = "kirin-pcie0-intc",
				},
				{
					.name = "kirin-pcie0-intd",
				},
				{
					.name = "kirin-pcie0-linkdown",
				}
			},
		.rc_dev = NULL,
		.ep_dev = NULL,
		.is_ready = ATOMIC_INIT(0),
		.is_enumerated = ATOMIC_INIT(0),
		.is_power_on = ATOMIC_INIT(0),
		.usr_suspend = ATOMIC_INIT(0),
	},
#ifdef CONFIG_KIRIN_PCIE_KIRIN970
	{
		.irq = {
				{
					.name = "kirin-pcie1-inta",
				},
				{
					.name = "kirin-pcie1-msi",
				},
				{
					.name = "kirin-pcie1-intc",
				},
				{
					.name = "kirin-pcie1-intd",
				},
				{
					.name = "kirin-pcie1-linkdown",
				}
			},
		.rc_dev = NULL,
		.ep_dev = NULL,
		.is_ready = ATOMIC_INIT(0),
		.is_enumerated = ATOMIC_INIT(0),
		.is_power_on = ATOMIC_INIT(0),
		.usr_suspend = ATOMIC_INIT(0),
	},
#endif
};

static int kirin_pcie_link_up(struct pcie_port *pp);

void kirin_elb_writel(struct kirin_pcie *pcie, u32 val, u32 reg)
{
	writel(val, pcie->apb_base + reg);
}

u32 kirin_elb_readl(struct kirin_pcie *pcie, u32 reg)
{
	return readl(pcie->apb_base + reg);
}

void kirin_apb_phy_writel(struct kirin_pcie *pcie, u32 val, u32 reg)
{
	if (pcie->dtsinfo.board_type != BOARD_FPGA)
		writel(val, pcie->phy_base + pcie->apb_phy_offset + reg);
}

u32 kirin_apb_phy_readl(struct kirin_pcie *pcie, u32 reg)
{
	if (pcie->dtsinfo.board_type != BOARD_FPGA)
		return readl(pcie->phy_base + pcie->apb_phy_offset + reg);
	return 0;
}

void kirin_natural_phy_writel(struct kirin_pcie *pcie, u32 val, u32 reg)
{
	writel(val, pcie->phy_base + pcie->natural_phy_offset + reg * 4);
}

u32 kirin_natural_phy_readl(struct kirin_pcie *pcie, u32 reg)
{
	return readl(pcie->phy_base + pcie->natural_phy_offset + reg * 4);
}

#ifndef CONFIG_KIRIN_PCIE_HI3660
void kirin_sram_phy_writel(struct kirin_pcie *pcie, u32 val, u32 reg)
{
	writel(val, pcie->phy_base + pcie->sram_phy_offset + reg * 4);
}

u32 kirin_sram_phy_readl(struct kirin_pcie *pcie, u32 reg)
{
	return readl(pcie->phy_base + pcie->sram_phy_offset + reg * 4);
}
#endif
static int32_t kirin_pcie_get_clk(struct kirin_pcie *pcie, struct platform_device *pdev)
{
	pcie->phy_ref_clk = devm_clk_get(&pdev->dev, "pcie_phy_ref");
	if (IS_ERR(pcie->phy_ref_clk)) {
		PCIE_PR_ERR("Failed to get pcie_phy_ref clock");
		return PTR_ERR(pcie->phy_ref_clk);
	}

	pcie->pcie_aux_clk = devm_clk_get(&pdev->dev, "pcie_aux");
	if (IS_ERR(pcie->pcie_aux_clk)) {
		PCIE_PR_ERR("Failed to get pcie_aux clock");
		return PTR_ERR(pcie->pcie_aux_clk);
	}

	pcie->apb_phy_clk = devm_clk_get(&pdev->dev, "pcie_apb_phy");
	if (IS_ERR(pcie->apb_phy_clk)) {
		PCIE_PR_ERR("Failed to get pcie_apb_phy clock");
		return PTR_ERR(pcie->apb_phy_clk);
	}

	pcie->apb_sys_clk = devm_clk_get(&pdev->dev, "pcie_apb_sys");
	if (IS_ERR(pcie->apb_sys_clk)) {
		PCIE_PR_ERR("Failed to get pcie_apb_sys clock");
		return PTR_ERR(pcie->apb_sys_clk);
	}

	pcie->pcie_aclk = devm_clk_get(&pdev->dev, "pcie_aclk");
	if (IS_ERR(pcie->pcie_aclk)) {
		PCIE_PR_ERR("Failed to get pcie_aclk clock");
		return PTR_ERR(pcie->pcie_aclk);
	}

	PCIE_PR_INFO("Successed to get all clock");

	return 0;
}

static int32_t get_phy_layout(struct kirin_pcie *pcie, struct device_node *np)
{
	u32 val[3];
	size_t size = 3;

	if (of_property_read_u32_array(np, "phy_layout_info", val, size)) {
		PCIE_PR_ERR("Failed to get phy layout info");
		return -1;
	}

	pcie->natural_phy_offset = val[0];
	pcie->sram_phy_offset = val[1];
	pcie->apb_phy_offset = val[2];

	return 0;
}

static int32_t kirin_pcie_get_baseaddr(struct pcie_port *pp,
					struct platform_device *pdev)
{
	struct resource *apb;
	struct resource *phy;
	struct resource *dbi;
	struct kirin_pcie *pcie = to_kirin_pcie(pp);
	struct device_node *np;

	apb = platform_get_resource_byname(pdev, IORESOURCE_MEM, "apb");
	pcie->apb_base = devm_ioremap_resource(&pdev->dev, apb);
	if (IS_ERR(pcie->apb_base)) {
		PCIE_PR_ERR("Failed to get PCIeCTRL apb base");
		return PTR_ERR(pcie->apb_base);
	}

	phy = platform_get_resource_byname(pdev, IORESOURCE_MEM, "phy");
	pcie->phy_base = devm_ioremap_resource(&pdev->dev, phy);
	if (IS_ERR(pcie->phy_base)) {
		PCIE_PR_ERR("Failed to get PCIePHY base");
		return PTR_ERR(pcie->phy_base);
	}

	dbi = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dbi");
	pp->dbi_base = devm_ioremap_resource(&pdev->dev, dbi);
	if (IS_ERR(pp->dbi_base)) {
		PCIE_PR_ERR("Failed to get PCIe dbi base");
		return PTR_ERR(pp->dbi_base);
	}

	np = pdev->dev.of_node;
	if (of_property_read_u32(np, "iatu_base_offset", &pcie->dtsinfo.iatu_base_offset)) {
		PCIE_PR_ERR("Failed to get iatu_base_offset info");
		return -1;
	}

	if (get_phy_layout(pcie, np))
		return -1;
#if defined(CONFIG_KIRIN_PCIE_KIRIN970) || defined(CONFIG_KIRIN_PCIE_HI3660)
	np = of_find_compatible_node(NULL, NULL, "hisilicon,crgctrl");
#else
	np = of_find_compatible_node(NULL, NULL, "hisilicon,mmc1_sysctrl");
#endif
	if (!np) {
		PCIE_PR_ERR("Failed to get crgctrl node ");
		return -1;
	}
	pcie->crg_base = of_iomap(np, 0);
	if (!pcie->crg_base) {
		PCIE_PR_ERR("Failed to iomap crg_base iomap");
		return -1;
	}

	np = of_find_compatible_node(NULL, NULL, "hisilicon,sysctrl");
	if (!np) {
		PCIE_PR_ERR("Failed to get sysctrl Node ");
		return -1;
	}
	pcie->sctrl_base = of_iomap(np, 0);
	if (!pcie->sctrl_base) {
		PCIE_PR_ERR("Failed to iomap sctrl_base");
		return -1;
	}

	np = of_find_compatible_node(NULL, NULL, "hisilicon,pmctrl");
	if (!np) {
		PCIE_PR_ERR("Failed to get pmctrl Node ");
		return -1;
	}
	pcie->pmctrl_base = of_iomap(np, 0);
	if (!pcie->pmctrl_base) {
		PCIE_PR_ERR("Failed to iomap sctrl_base");
		return -1;
	}
	pcie->pme_base = ioremap(MSG_CPU_ADDR, MSG_CPU_ADDR_SIZE);
	if (!pcie->pme_base) {
		PCIE_PR_ERR("Failed to ioremap pme addr");
		return -1;
	}

	PCIE_PR_INFO("Successed to get all resource");
	return 0;
}

static int32_t kirin_pcie_get_pinctrl(struct kirin_pcie *pcie,
					struct platform_device *pdev)
{
        int gpio_id;

        gpio_id = of_get_named_gpio(pdev->dev.of_node, "reset-gpio", 0);
        if (gpio_id < 0) {
                PCIE_PR_ERR("Failed to get perst gpio number");
                return -1;
        }

        pcie->gpio_id_reset = gpio_id;

        return 0;
}
static void kirin_pcie_get_boardtype(struct kirin_pcie *pcie,
				struct platform_device *pdev)
{
	int len;
	struct device_node *np;
	struct kirin_pcie_dtsinfo *dtsinfo;

	np = pdev->dev.of_node;
	dtsinfo = &pcie->dtsinfo;
	if (of_property_read_u32(np, "board_type", &dtsinfo->board_type)) {
		PCIE_PR_ERR("Failed to get board_type");
		dtsinfo->board_type = 2;
	}
	PCIE_PR_INFO("The board_type value is [%d] ", dtsinfo->board_type);

	if (of_property_read_u32(np, "chip_type", &dtsinfo->chip_type)) {
		PCIE_PR_ERR("Failed to get chip_type");
		dtsinfo->chip_type = 0;
	}
	PCIE_PR_INFO("The chip_type value is [%d] ", dtsinfo->chip_type);

	if (of_find_property(np, "ep_flag", &len)) {
		dtsinfo->ep_flag = 1;
		PCIE_PR_INFO("EndPoint Device");
	} else {
		dtsinfo->ep_flag = 0;
		PCIE_PR_INFO("RootComplex");
	}
}

static int32_t kirin_pcie_get_isoinfo(struct kirin_pcie *pcie,
					struct platform_device *pdev)
{
	size_t array_num = 2;
	struct device_node *np;
	struct kirin_pcie_dtsinfo *dtsinfo;

	np = pdev->dev.of_node;
	dtsinfo = &pcie->dtsinfo;

	if (of_property_read_u32_array(np, "iso_info", dtsinfo->iso_info, array_num)) {
		PCIE_PR_ERR("Failed to get isoen info");
		return -1;
	}

	return 0;
}

void kirin_pcie_iso_ctrl(struct kirin_pcie *pcie, int en_flag)
{
	if (en_flag)
		writel(pcie->dtsinfo.iso_info[1],
			pcie->sctrl_base + pcie->dtsinfo.iso_info[0]);
	else
		writel(pcie->dtsinfo.iso_info[1],
			pcie->sctrl_base + pcie->dtsinfo.iso_info[0]
			+ ADDR_OFFSET_4BYTE);
}

static int32_t kirin_pcie_get_assertinfo(struct kirin_pcie *pcie,
					struct platform_device *pdev)
{
	size_t array_num = 2;
	struct device_node *np;
	struct kirin_pcie_dtsinfo *dtsinfo;

	np = pdev->dev.of_node;
	dtsinfo = &pcie->dtsinfo;

	if (of_property_read_u32_array(np, "assert_info", dtsinfo->assert_info, array_num)) {
		PCIE_PR_ERR("Failed to get assert info");
		return -1;
	}

	return 0;
}

void kirin_pcie_reset_ctrl(struct kirin_pcie *pcie, enum RST_TYPE rst)
{
	if (rst == RST_DISABLE)
		writel(pcie->dtsinfo.assert_info[1],
			pcie->crg_base + pcie->dtsinfo.assert_info[0]
			+ ADDR_OFFSET_4BYTE);
	else
		writel(pcie->dtsinfo.assert_info[1],
			pcie->crg_base + pcie->dtsinfo.assert_info[0]);
}

static void kirin_pcie_get_linkstate(struct kirin_pcie *pcie,
					struct platform_device *pdev)
{
	int ret;
	struct kirin_pcie_dtsinfo *dtsinfo;

	dtsinfo = &pcie->dtsinfo;

	ret = of_property_read_u32(pdev->dev.of_node,
				"ep_ltr_latency", &dtsinfo->ep_ltr_latency);
	if (ret) {
		PCIE_PR_DEBUG("Not to set ep ltr_latency ?");
		dtsinfo->ep_ltr_latency = 0x0;
	}

	ret = of_property_read_u32(pdev->dev.of_node,
				"ep_l1ss_ctrl2", &dtsinfo->ep_l1ss_ctrl2);
	if (ret) {
		PCIE_PR_DEBUG("Not to set ep L1ss_ctrl2 ?");
		dtsinfo->ep_l1ss_ctrl2 = 0x0;
	}

	ret = of_property_read_u32(pdev->dev.of_node,
				"l1ss_ctrl1", &dtsinfo->l1ss_ctrl1);
	if (ret) {
		PCIE_PR_DEBUG("Not to set L1ss_ctrl1 ?");
		dtsinfo->l1ss_ctrl1 = 0x0;
	}

	ret = of_property_read_u32(pdev->dev.of_node,
				"aspm_state", &dtsinfo->aspm_state);
	if (ret) {
		PCIE_PR_DEBUG("Not to set aspm_state ?");
		dtsinfo->aspm_state = ASPM_L1;
	}

	PCIE_PR_DEBUG(" ltr_latency = [0x%x], l1ss_ctrl2 = [0x%x] ",
			dtsinfo->ep_ltr_latency, dtsinfo->ep_l1ss_ctrl2);
	PCIE_PR_DEBUG(" l1ss_ctrl1 = [0x%x], aspm_state = [0x%x] ",
			dtsinfo->l1ss_ctrl1, dtsinfo->aspm_state);
}

static void kirin_pcie_get_eco(struct kirin_pcie *pcie,
				struct platform_device *pdev)
{
	int ret;
	struct kirin_pcie_dtsinfo *dtsinfo;

	dtsinfo = &pcie->dtsinfo;

	ret = of_property_read_u32(pdev->dev.of_node, "eco", &dtsinfo->eco);
	if (ret) {
		PCIE_PR_DEBUG("Not choose SRAM ECO");
		dtsinfo->eco = 0x0;
	}

	PCIE_PR_DEBUG(" set eco to [0x%x]", dtsinfo->eco);
}

static void pcie_get_time_params(struct kirin_pcie *pcie,
					struct device_node *np)
{
	size_t array_num = 2;

	if (of_property_read_u32_array(np, "t_ref2perst",
		pcie->dtsinfo.t_ref2perst, array_num))
		memset(&pcie->dtsinfo.t_ref2perst, 0, array_num);

	if (of_property_read_u32_array(np, "t_perst2access",
		pcie->dtsinfo.t_perst2access, array_num))
		memset(&pcie->dtsinfo.t_perst2access, 0, array_num);

	if (of_property_read_u32_array(np, "t_perst2rst",
		pcie->dtsinfo.t_perst2rst, array_num))
		memset(&pcie->dtsinfo.t_perst2rst, 0, array_num);
}

int32_t kirin_pcie_get_dtsinfo(u32 *rc_id,
					struct platform_device *pdev)
{
	struct pcie_port *pp;
	struct kirin_pcie *pcie;
	struct device_node *np;

	if (!pdev->dev.of_node) {
		PCIE_PR_ERR("Of_node is null");
		return -EINVAL;
	}

	if (of_property_read_u32(pdev->dev.of_node, "rc-id", rc_id)) {
		dev_err(&pdev->dev, "Failed to get rc_id info\n");
		return -EINVAL;
	}

	np = of_find_node_by_name(NULL, "kirin_pcie");
	if (!np) {
		PCIE_PR_ERR("Failed to get kirin_pcie info");
		return -1;
	}

	if (of_property_read_u32(np, "rc_num", &g_rc_num)) {
		PCIE_PR_ERR("Failed to get rc_num info");
		return -1;
	}

	if (*rc_id >= g_rc_num) {
		PCIE_PR_ERR("There is no rc_id = %d", *rc_id);
		return -EINVAL;
	}

	pcie = &g_kirin_pcie[*rc_id];
	pcie->rc_id = *rc_id;
	pp = &pcie->pp;

	kirin_pcie_get_boardtype(pcie, pdev);

	kirin_pcie_get_eyeparam(pcie, pdev);

	kirin_pcie_get_linkstate(pcie, pdev);

	kirin_pcie_get_eco(pcie, pdev);

	pcie_get_time_params(pcie, pdev->dev.of_node);

	if (kirin_pcie_get_isoinfo(pcie, pdev))
		return -ENODEV;

	if (kirin_pcie_get_assertinfo(pcie, pdev))
		return -ENODEV;

	if (kirin_pcie_get_clk(pcie, pdev))
		return -ENODEV;

	if (kirin_pcie_get_pinctrl(pcie, pdev))
		return -ENODEV;

	if (kirin_pcie_get_baseaddr(pp, pdev))
		return -ENODEV;

	return 0;
}

static int perst_from_pciectrl(struct kirin_pcie *pcie, int pull_up)
{
	u32 val;
	val = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL12_ADDR);
	val |= PERST_FUN_SEC;
	if (pull_up)
		val |= PERST_ASSERT_EN;
	else
		val &= ~PERST_ASSERT_EN;
	kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL12_ADDR);

	return 0;
}

static int perst_from_gpio(struct kirin_pcie *pcie, int pull_up)
{
	return gpio_direction_output((unsigned int)pcie->gpio_id_reset, pull_up);
}

int kirin_pcie_perst_cfg(struct kirin_pcie *pcie, int pull_up)
{
	int ret;

	if (pull_up)
		usleep_range(pcie->dtsinfo.t_ref2perst[0], pcie->dtsinfo.t_ref2perst[1]);

	if (pcie->dtsinfo.board_type == BOARD_FPGA)
		ret = perst_from_pciectrl(pcie, pull_up);
	else
		ret = perst_from_gpio(pcie, pull_up);

	if (ret)
		PCIE_PR_ERR("Failed to pulse perst signal");

	if (pull_up)
		usleep_range(pcie->dtsinfo.t_perst2access[0], pcie->dtsinfo.t_perst2access[1]);
	else
		usleep_range(pcie->dtsinfo.t_perst2rst[0], pcie->dtsinfo.t_perst2rst[1]);

	return ret;
}


static void kirin_pcie_sideband_dbi_w_mode(struct pcie_port *pp, bool on)
{
	u32 val;
	struct kirin_pcie *pcie = to_kirin_pcie(pp);

	val = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL0_ADDR);
	if (on)
		val = val | PCIE_ELBI_SLV_DBI_ENABLE;
	else
		val = val & ~PCIE_ELBI_SLV_DBI_ENABLE;

	kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL0_ADDR);
}

static void kirin_pcie_sideband_dbi_r_mode(struct pcie_port *pp, bool on)
{
	u32 val;
	struct kirin_pcie *pcie = to_kirin_pcie(pp);

	val = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL1_ADDR);
	if (on)
		val = val | PCIE_ELBI_SLV_DBI_ENABLE;
	else
		val = val & ~PCIE_ELBI_SLV_DBI_ENABLE;

	kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL1_ADDR);
}

static int kirin_pcie_establish_link(struct pcie_port *pp)
{
	int count = 0;
	struct kirin_pcie *pcie = to_kirin_pcie(pp);

	PCIE_PR_INFO("++");

	if (kirin_pcie_link_up(pp)) {
		PCIE_PR_ERR("Link already up");
		return 0;
	}

	/* setup root complex */
	dw_pcie_setup_rc(pp);
	PCIE_PR_DEBUG("Setup rc done ");

	/* assert LTSSM enable */
	kirin_elb_writel(pcie, PCIE_LTSSM_ENABLE_BIT,
			  PCIE_APP_LTSSM_ENABLE);

	/* check if the link is up or not */
	while (!kirin_pcie_link_up(pp)) {
		mdelay(1);
		count++;
		if (count == 200) {
			PCIE_PR_ERR("Link Fail, status is [0x%x] ",
				 kirin_elb_readl(pcie, SOC_PCIECTRL_STATE4_ADDR));
			dsm_pcie_dump_info(pcie, DSM_ERR_ESTABLISH_LINK);
			return -EINVAL;
		}
	}

	PCIE_PR_INFO("PCIe Link success ");
	return 0;
}

/*EP rigist hook fun for link event notification*/
int kirin_pcie_register_event(struct kirin_pcie_register_event *reg)
{
	int ret = 0;
	struct pci_dev *dev;
	struct pcie_port *pp;
	struct kirin_pcie *pcie;

	if (!reg || !reg->user) {
		PCIE_PR_INFO("Event registration or user of event is null");
		return -ENODEV;
	}

	dev = (struct pci_dev *)reg->user;
	pp = (struct pcie_port *)(dev->bus->sysdata);
	/*lint -e826 -esym(826,*)*/
	pcie = container_of(pp, struct kirin_pcie, pp);
	/*lint -e826 +esym(826,*)*/

	if (pp) {
		pcie->event_reg = reg;
		PCIE_PR_INFO("Event 0x%x is registered for RC", reg->events);
	} else {
		PCIE_PR_INFO("did not find RC for pci endpoint device");
		ret = -ENODEV;
	}
	return ret;
}
EXPORT_SYMBOL_GPL(kirin_pcie_register_event);

int kirin_pcie_deregister_event(struct kirin_pcie_register_event *reg)
{
	int ret = 0;
	struct pci_dev *dev;
	struct pcie_port *pp;
	struct kirin_pcie *pcie;

	if (!reg || !reg->user) {
		PCIE_PR_INFO("Event registration or user of event is NULL");
		return -ENODEV;
	}

	dev = (struct pci_dev *)reg->user;
	pp = (struct pcie_port *)(dev->bus->sysdata);
	/*lint -e826 -esym(826,*)*/
	pcie = container_of(pp, struct kirin_pcie, pp);
	/*lint -e826 +esym(826,*)*/

	if (pp) {
		pcie->event_reg = NULL;
		PCIE_PR_INFO("deregistered ");
	} else {
		PCIE_PR_INFO("No RC for this EP device ");
		ret = -ENODEV;
	}
	return ret;
}
EXPORT_SYMBOL_GPL(kirin_pcie_deregister_event);

static void dump_apb_register(struct kirin_pcie *pcie)
{
	u32 j;

	PCIE_PR_INFO("####DUMP APB CORE Register : ");
	for (j = 0; j < 0x4; j++) {
		printk(KERN_INFO "0x%-8x: %8x %8x %8x %8x \n",
			0x10 * j,
			kirin_elb_readl(pcie, 0x10 * j + 0x0),
			kirin_elb_readl(pcie, 0x10 * j + 0x4),
			kirin_elb_readl(pcie, 0x10 * j + 0x8),
			kirin_elb_readl(pcie, 0x10 * j + 0xC));
	}

	for (j = 0; j < 0x2; j++) {
		printk(KERN_INFO "0x%-8x: %8x %8x %8x %8x \n",
			0x10 * j + 0x400,
			kirin_elb_readl(pcie, 0x10 * j + 0x0 + 0x400),
			kirin_elb_readl(pcie, 0x10 * j + 0x4 + 0x400),
			kirin_elb_readl(pcie, 0x10 * j + 0x8 + 0x400),
			kirin_elb_readl(pcie, 0x10 * j + 0xC + 0x400));
	}

	PCIE_PR_INFO("####DUMP APB PHY Register : ");
	printk(KERN_INFO "0x%-8x: %8x %8x %8x %8x %8x ",
		0x0,
		kirin_apb_phy_readl(pcie, 0x0),
		kirin_apb_phy_readl(pcie, 0x4),
		kirin_apb_phy_readl(pcie, 0x8),
		kirin_apb_phy_readl(pcie, 0xc),
		kirin_apb_phy_readl(pcie, 0x400));
	printk("\n");
}

/*print apb and cfg-register of RC*/
static void dump_link_register(struct kirin_pcie *pcie)
{
	struct pcie_port *pp = &pcie->pp;
	int i;
	u32 val0, val1, val2, val3;

	if (!atomic_read(&(pcie->is_power_on)))
		return;

	dump_apb_register(pcie);

	PCIE_PR_INFO("####DUMP RC CFG Register ");
	for (i = 0; i < 0x18; i++) {
		kirin_pcie_rd_own_conf(pp, 0x10 * i + 0x0, 4, &val0);
		kirin_pcie_rd_own_conf(pp, 0x10 * i + 0x4, 4, &val1);
		kirin_pcie_rd_own_conf(pp, 0x10 * i + 0x8, 4, &val2);
		kirin_pcie_rd_own_conf(pp, 0x10 * i + 0xC, 4, &val3);
		printk(KERN_INFO "0x%-8x: %8x %8x %8x %8x \n",
			0x10 * i, val0, val1, val2, val3);
	}
	for (i = 0; i < 6; i++) {
		kirin_pcie_rd_own_conf(pp, 0x10 * i + 0x0 + 0x700, 4, &val0);
		kirin_pcie_rd_own_conf(pp, 0x10 * i + 0x4 + 0x700, 4, &val1);
		kirin_pcie_rd_own_conf(pp, 0x10 * i + 0x8 + 0x700, 4, &val2);
		kirin_pcie_rd_own_conf(pp, 0x10 * i + 0xC + 0x700, 4, &val3);
		printk(KERN_INFO "0x%-8x: %8x %8x %8x %8x \n",
			0x10 * i + 0x700, val0, val1, val2, val3);
	}
	for (i = 0; i < 0x9; i++) {
		kirin_pcie_rd_own_conf(pp, 0x10 * i + 0x0 + 0x8A0, 4, &val0);
		kirin_pcie_rd_own_conf(pp, 0x10 * i + 0x4 + 0x8A0, 4, &val1);
		kirin_pcie_rd_own_conf(pp, 0x10 * i + 0x8 + 0x8A0, 4, &val2);
		kirin_pcie_rd_own_conf(pp, 0x10 * i + 0xC + 0x8A0, 4, &val3);
		printk(KERN_INFO "0x%-8x: %8x %8x %8x %8x \n",
			0x10 * i + 0x8A0, val0, val1, val2, val3);
	}

	return;
}

typedef void (* WIFI_DUMP_FUNC) (void);
#ifdef CONFIG_KIRIN_PCIE_NOC_DBG
bool g_pcie_dump_flag = false;
typedef void (* WIFI_DUMP_FUNC) (void);
WIFI_DUMP_FUNC g_wifi_dump = NULL;
void set_pcie_dump_flag(void)
{
	g_pcie_dump_flag = true;
}

void clear_pcie_dump_flag(void)
{
	g_pcie_dump_flag = false;
}

bool get_pcie_dump_flag(void)
{
	return g_pcie_dump_flag;
}

void dump_pcie_apb_info(u32 rc_id)
{
	struct kirin_pcie *pcie;

	if (rc_id >= g_rc_num) {
		PCIE_PR_ERR("There is no rc_id = %d", rc_id);
		return;
	}

	pcie = &g_kirin_pcie[rc_id];

	if (!atomic_read(&pcie->is_power_on)) {
		PCIE_PR_ERR("PCIe is Poweroff");
		return;
	}

	dump_apb_register(pcie);

	if (g_wifi_dump) {
		PCIE_PR_ERR("Dump wifi info");
		g_wifi_dump();
	}

	clear_pcie_dump_flag();
}

void register_wifi_dump_func(WIFI_DUMP_FUNC func)
{
	g_wifi_dump = func;
}
#else
void set_pcie_dump_flag(void)
{
	return;
}

void clear_pcie_dump_flag(void)
{
	return;
}

bool get_pcie_dump_flag(void)
{
	return false;
}

void dump_pcie_apb_info(u32 rc_id)
{
	return;
}

void register_wifi_dump_func(WIFI_DUMP_FUNC func)
{
	return;
}
#endif
EXPORT_SYMBOL_GPL(set_pcie_dump_flag);
EXPORT_SYMBOL_GPL(get_pcie_dump_flag);
EXPORT_SYMBOL_GPL(dump_pcie_apb_info);
EXPORT_SYMBOL_GPL(register_wifi_dump_func);

/*notify EP about link-down event*/
static void kirin_pcie_notify_callback(struct kirin_pcie *pcie,
				enum kirin_pcie_event event)
{
	if ((pcie->event_reg != NULL) && (pcie->event_reg->callback != NULL) &&
			(pcie->event_reg->events & event)) {
		struct kirin_pcie_notify *notify = &pcie->event_reg->notify;
		notify->event = event;
		notify->user = pcie->event_reg->user;
		PCIE_PR_INFO("Callback for the event : %d", event);
		pcie->event_reg->callback(notify);
	} else {
		PCIE_PR_INFO("EP does not register this event : %d", event);
	}
}

static void kirin_handle_work(struct work_struct *work)
{
	/*lint -e826 -esym(826,*)*/
	struct kirin_pcie *pcie = container_of(work, struct kirin_pcie, handle_work);
	/*lint -e826 +esym(826,*)*/

	dsm_pcie_dump_info(pcie, DSM_ERR_LINK_DOWN);

	dump_link_register(pcie);

	kirin_pcie_notify_callback(pcie, KIRIN_PCIE_EVENT_LINKDOWN);
}

static irqreturn_t kirin_pcie_linkdown_irq_handler(int irq, void *arg)
{
	struct pcie_port *pp = arg;
	struct kirin_pcie *pcie = to_kirin_pcie(pp);

	PCIE_PR_ERR("Triggle linkdown irq[%d]", irq);

	schedule_work(&pcie->handle_work);

	return IRQ_HANDLED;
}
static irqreturn_t kirin_pcie_msi_irq_handler(int irq, void *arg)
{
	struct pcie_port *pp = arg;

	PCIE_PR_ERR("Triggle msi irq[%d]", irq);
	return dw_handle_msi_irq(pp);
}

#ifdef CONFIG_KIRIN_PCIE_TEST
static irqreturn_t kirin_pcie_intx_irq_handler(int irq, void *arg)
{
	PCIE_PR_ERR("Triggle intx irq[%d]", irq);
	return IRQ_HANDLED;
}
#endif

static void kirin_pcie_msi_init(struct pcie_port *pp)
{
	dw_pcie_msi_init(pp);

}

static void kirin_pcie_enable_interrupts(struct pcie_port *pp)
{
	if (IS_ENABLED(CONFIG_PCI_MSI))
		kirin_pcie_msi_init(pp);
}

void kirin_pcie_readl_rc(struct pcie_port *pp,
					void __iomem *dbi_base, u32 *val)
{
	struct kirin_pcie *pcie = to_kirin_pcie(pp);
	if (!atomic_read(&(pcie->is_power_on)))
		return;

	kirin_pcie_sideband_dbi_r_mode(pp, true);
	*val = readl(dbi_base);
	kirin_pcie_sideband_dbi_r_mode(pp, false);
}

void kirin_pcie_writel_rc(struct pcie_port *pp,
					u32 val, void __iomem *dbi_base)
{
	struct kirin_pcie *pcie = to_kirin_pcie(pp);
	if (!atomic_read(&(pcie->is_power_on)))
		return;

	kirin_pcie_sideband_dbi_w_mode(pp, true);
	writel(val, dbi_base);
	kirin_pcie_sideband_dbi_w_mode(pp, false);
}

int kirin_pcie_rd_own_conf(struct pcie_port *pp, int where, int size,
				u32 *val)
{
	int ret;
	struct kirin_pcie *pcie = to_kirin_pcie(pp);
	if (!atomic_read(&(pcie->is_power_on)))
		return -EINVAL;

	kirin_pcie_sideband_dbi_r_mode(pp, true);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
	ret = dw_pcie_cfg_read(pp->dbi_base + (where & ~0x3), where, size, val);
#else
	ret = dw_pcie_cfg_read(pp->dbi_base + where, size, val);
#endif
	kirin_pcie_sideband_dbi_r_mode(pp, false);
	return ret;
}

int kirin_pcie_wr_own_conf(struct pcie_port *pp, int where, int size,
				u32 val)
{
	int ret;
	struct kirin_pcie *pcie = to_kirin_pcie(pp);
	if (!atomic_read(&(pcie->is_power_on)))
		return -EINVAL;

	kirin_pcie_sideband_dbi_w_mode(pp, true);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
	ret = dw_pcie_cfg_write(pp->dbi_base + (where & ~0x3),
			where, size, val);
#else
	ret = dw_pcie_cfg_write(pp->dbi_base + where, size, val);
#endif
	kirin_pcie_sideband_dbi_w_mode(pp, false);
	return ret;
}

static int kirin_pcie_link_up(struct pcie_port *pp)
{
	struct kirin_pcie *pcie = to_kirin_pcie(pp);
	u32 val;

	if (!atomic_read(&(pcie->is_power_on)) || atomic_read(&(pcie->usr_suspend)))
		return 0;

	val = kirin_elb_readl(pcie, PCIE_ELBI_RDLH_LINKUP);

	if (((val & PCIE_LINKUP_ENABLE) == PCIE_LINKUP_ENABLE)
		&& pcie->ep_link_status == DEVICE_LINK_UP)
		return 1;

	return 0;
}

static int kirin_pcie_host_init(struct pcie_port *pp)
{
	if (kirin_pcie_establish_link(pp))
		return -1;

	kirin_pcie_enable_interrupts(pp);

	return 0;
}

static struct pcie_host_ops kirin_pcie_host_ops = {
	.readl_rc = kirin_pcie_readl_rc,
	.writel_rc = kirin_pcie_writel_rc,
	.rd_own_conf = kirin_pcie_rd_own_conf,
	.wr_own_conf = kirin_pcie_wr_own_conf,
	.link_up = kirin_pcie_link_up,
	.host_init = kirin_pcie_host_init,
};

static int __init kirin_add_pcie_port(struct pcie_port *pp,
					   struct platform_device *pdev)
{
	int ret;
	int index;
	struct kirin_pcie *pcie = to_kirin_pcie(pp);

	PCIE_PR_INFO("++");
	for (index = 0; index < MAX_IRQ_NUM; index++) {
		pcie->irq[index].num = platform_get_irq(pdev, index);
		if (!pcie->irq[index].num) {
			PCIE_PR_ERR("Failed to get [%s] irq ,num = [%d]", pcie->irq[index].name,
				pcie->irq[index].num);
			return -ENODEV;
		}
	}
#ifdef CONFIG_KIRIN_PCIE_TEST
	ret = devm_request_irq(&pdev->dev, (unsigned int)pcie->irq[IRQ_INTC].num,
				kirin_pcie_intx_irq_handler,
				(unsigned long)IRQF_TRIGGER_RISING, pcie->irq[IRQ_INTC].name, pp);
	ret |= devm_request_irq(&pdev->dev, (unsigned int)pcie->irq[IRQ_INTD].num,
				kirin_pcie_intx_irq_handler,
				(unsigned long)IRQF_TRIGGER_RISING, pcie->irq[IRQ_INTD].name, pp);
	if (ret) {
		PCIE_PR_ERR("Failed to request intx irq");
		return ret;
	}
#endif

	ret = devm_request_irq(&pdev->dev, pcie->irq[IRQ_LINKDOWN].num,
				kirin_pcie_linkdown_irq_handler,
				IRQF_TRIGGER_RISING, pcie->irq[IRQ_LINKDOWN].name, pp);
	if (ret) {
		PCIE_PR_ERR("Failed to request linkdown irq");
		return ret;
	}

	PCIE_PR_INFO("pcie->irq[1].name = [%s], pcie->irq[4].name = [%s]",
				 pcie->irq[IRQ_MSI].name, pcie->irq[IRQ_LINKDOWN].name);

	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		pp->msi_irq = pcie->irq[IRQ_MSI].num;
		ret = devm_request_irq(&pdev->dev, pp->msi_irq,
					kirin_pcie_msi_irq_handler,
					IRQF_SHARED | IRQF_TRIGGER_RISING,
					pcie->irq[IRQ_MSI].name, pp);
		if (ret) {
			PCIE_PR_ERR("Failed to request msi irq");
			return ret;
		}
	}

	PCIE_PR_INFO("Add pcie port sucessed ");
	PCIE_PR_INFO("--");
	return 0;
}

static int kirin_pcie_probe(struct platform_device *pdev)
{
	struct kirin_pcie *pcie;
	struct pcie_port *pp;
	int ret;
	u32 rc_id;
	struct kirin_pcie_dtsinfo *dtsinfo;

	PCIE_PR_INFO("++");

	if (kirin_pcie_get_dtsinfo(&rc_id, pdev)) {
		PCIE_PR_ERR("Failed to get dts info");
		return -EINVAL;
	}

	PCIE_PR_INFO("PCIe No.%d probe", rc_id);

	pcie = &g_kirin_pcie[rc_id];
	pp = &pcie->pp;
	pp->dev = &(pdev->dev);
	dtsinfo = &pcie->dtsinfo;

	if (gpio_request((unsigned int)pcie->gpio_id_reset, "pcie_reset")) {
		PCIE_PR_ERR("Failed to request gpio-%d", pcie->gpio_id_reset);
		return -1;
	}
	pp->ops = &kirin_pcie_host_ops;
	INIT_WORK(&pcie->handle_work, kirin_handle_work);

	platform_set_drvdata(pdev, pcie);

	ret = kirin_add_pcie_port(pp, pdev);
	if (ret < 0) {
		PCIE_PR_ERR("Failed to assign resource, ret=[%d]", ret);
		return ret;
	}

#if defined(CONFIG_KIRIN_PCIE_EYEPARAM) && defined(CONFIG_DEBUG_FS)
	pcie_debug_init(pcie);
#endif

#if defined(CONFIG_PCIE_KIRIN_SLT) && defined(__SLT_FEATURE__)
	pcie_slt_wlan_on(pcie, ENABLE);
#endif

	atomic_set(&(pcie->is_ready), 1);
	spin_lock_init(&pcie->ep_ltssm_lock);

	PCIE_PR_INFO("--");
	return 0;
}

static int kirin_pcie_save_rc_cfg(struct kirin_pcie *pcie)
{
	int ret;
	u32 val = 0;
	int aer_pos;
	struct pcie_port *pp;

	pp = &(pcie->pp);

	kirin_pcie_rd_own_conf(pp, PORT_MSI_CTRL_ADDR, 4, &val);
	pcie->msi_controller_config[0] = val;
	kirin_pcie_rd_own_conf(pp, PORT_MSI_CTRL_UPPER_ADDR, 4, &val);
	pcie->msi_controller_config[1] = val;
	kirin_pcie_rd_own_conf(pp, PORT_MSI_CTRL_INT0_ENABLE, 4, &val);
	pcie->msi_controller_config[2] = val;

	aer_pos = pci_find_ext_capability(pcie->rc_dev, PCI_EXT_CAP_ID_ERR);
	if (!aer_pos ) {
		PCIE_PR_ERR("Failed to get RC PCI_EXT_CAP_ID_ERR");
		return -1;
	}

	pci_read_config_dword(pcie->rc_dev, aer_pos + PCI_ERR_ROOT_COMMAND,
		&pcie->aer_config);

	ret = pci_save_state(pcie->rc_dev);
	if (ret) {
		PCIE_PR_ERR("Failed to save state of RC.");
		return -1;
	}
	pcie->rc_saved_state = pci_store_saved_state(pcie->rc_dev);

	return 0;
}

static int kirin_pcie_restore_rc_cfg(struct kirin_pcie *pcie)
{
	struct pcie_port *pp;
	int aer_pos;

	pp = &(pcie->pp);
	if (!pcie->rc_dev) {
		PCIE_PR_ERR("Failed to get RC dev");
		return -1;
	}

	kirin_pcie_wr_own_conf(pp, PORT_MSI_CTRL_ADDR,
			4, pcie->msi_controller_config[0]);
	kirin_pcie_wr_own_conf(pp, PORT_MSI_CTRL_UPPER_ADDR,
			4, pcie->msi_controller_config[1]);
	kirin_pcie_wr_own_conf(pp, PORT_MSI_CTRL_INT0_ENABLE,
			4, pcie->msi_controller_config[2]);

	aer_pos = pci_find_ext_capability(pcie->rc_dev, PCI_EXT_CAP_ID_ERR);
	if (!aer_pos ) {
		PCIE_PR_ERR("Failed to get RC PCI_EXT_CAP_ID_ERR");
		return -1;
	}

	pci_write_config_dword(pcie->rc_dev, aer_pos + PCI_ERR_ROOT_COMMAND,
		pcie->aer_config);

	pci_load_saved_state(pcie->rc_dev, pcie->rc_saved_state);
	pci_restore_state(pcie->rc_dev);
	pci_load_saved_state(pcie->rc_dev, pcie->rc_saved_state);

	return 0;
}

static void send_pme_turn_off_msg(struct kirin_pcie *pcie)
{
	u32 val;

#if defined(CONFIG_KIRIN_PCIE_HI3660)
	val = kirin_elb_readl(pcie, SOC_PCIECTRL_CTRL7_ADDR);
	val |= PME_TURN_OFF_BIT;
	kirin_elb_writel(pcie, val, SOC_PCIECTRL_CTRL7_ADDR);
#else

	struct pcie_port *pp;
	unsigned int iatu_offset;

	pp = &(pcie->pp);
	iatu_offset = pcie->dtsinfo.iatu_base_offset;

	kirin_pcie_outbound_atu(pcie->rc_id, PCIE_ATU_REGION_INDEX0,
		MSG_TYPE_ROUTE_BROADCAST,	MSG_CPU_ADDR, 0x0, MSG_CPU_ADDR_SIZE);

	kirin_pcie_readl_rc(pp, pp->dbi_base+ iatu_offset + PCIE_ATU_CR2, &val);
	val |= (MSG_CODE_PME_TURN_OFF | 0x400000);
	kirin_pcie_writel_rc(pp, val, pp->dbi_base+ iatu_offset + PCIE_ATU_CR2);

	writel(0x0, pcie->pme_base);

#endif
}

static int kirin_pcie_shutdown_prepare(struct pci_dev *dev)
{
	u32 val;
	u32 pm;
	int index = 0;
	struct pcie_port *pp;
	struct kirin_pcie *pcie;

	PCIE_PR_INFO("++");

	if (!dev) {
		PCIE_PR_ERR("pci_dev is null");
		return -1;
	}
	pp = dev->sysdata;
	pcie = to_kirin_pcie(pp);

	/*Enable PME*/
	pm = pci_find_capability(dev, PCI_CAP_ID_PM);
	if (!pm) {
		PCIE_PR_ERR("Failed to get PCI_CAP_ID_PM");
		return -1;
	}
	kirin_pcie_rd_own_conf(pp, pm + PCI_PM_CTRL, 4, &val);
	val |= 0x100;
	kirin_pcie_wr_own_conf(pp, pm + PCI_PM_CTRL, 4, val);

	send_pme_turn_off_msg(pcie);

	do {
		val = kirin_elb_readl(pcie, SOC_PCIECTRL_STATE1_ADDR);
		val = val & PME_ACK_BIT;
		if (index >= 1000) {
			PCIE_PR_ERR("Failed to get PME_TO_ACK");
			return -1;
		}
		index++;
		udelay((unsigned long)10);
	} while (val != PME_ACK_BIT);

	PCIE_PR_INFO("Get PME ACK ");

	PCIE_PR_INFO("--");
	return 0;
}

static void kirin_pcie_shutdown(struct platform_device *pdev)
{
	struct kirin_pcie *pcie;

	PCIE_PR_INFO("++");

	pcie = dev_get_drvdata(&(pdev->dev));
	if (pcie == NULL) {
		PCIE_PR_ERR("Failed to get drvdata");
		return;
	}

	if (atomic_read(&(pcie->is_power_on))) {
		if (kirin_pcie_power_on((&pcie->pp), RC_POWER_OFF)) {
			PCIE_PR_ERR("Failed to power off");
			return;
		}
	}

	PCIE_PR_INFO("--");
}

#ifdef CONFIG_PM
static int kirin_pcie_resume_noirq(struct device *dev)
{
	struct pci_dev *rc_dev;
	struct pcie_port *pp;
	struct kirin_pcie *pcie;

	PCIE_PR_INFO("++");

	pcie = dev_get_drvdata(dev);
	if (!pcie) {
		PCIE_PR_ERR("Failed to get drvdata");
		return -EINVAL;
	}
	pp = &pcie->pp;
	rc_dev = pcie->rc_dev;

	if (atomic_read(&(pcie->is_enumerated)) && (!atomic_read(&(pcie->usr_suspend)))) {
		if (kirin_pcie_power_on(pp, RC_POWER_RESUME)) {
			PCIE_PR_ERR("Failed to power on ");
			goto FAIL;
		}

		/* assert LTSSM enable */
		kirin_elb_writel(pcie, PCIE_LTSSM_ENABLE_BIT,
			PCIE_APP_LTSSM_ENABLE);

		PCIE_PR_DEBUG("Begin to recover RC cfg ");
		if (rc_dev)
			kirin_pcie_restore_rc_cfg(pcie);
	}

	PCIE_PR_INFO("--");
	return 0;

FAIL:
	schedule_work(&pcie->handle_work);

	return -EINVAL;
}


static int kirin_pcie_suspend_noirq(struct device *dev)
{
	struct kirin_pcie *pcie;
	struct pci_dev *rc_dev;
	struct pcie_port *pp;

	PCIE_PR_INFO("++");

	pcie = dev_get_drvdata(dev);
	if (pcie == NULL) {
		PCIE_PR_ERR("Failed to get drvdata");
		return -EINVAL;
	}
	rc_dev = pcie->rc_dev;
	pp = &pcie->pp;

	if (atomic_read(&(pcie->is_power_on))) {
		if (!atomic_read(&(pcie->usr_suspend))) {
			kirin_pcie_lp_ctrl(pcie->rc_id, DISABLE);
			kirin_pcie_shutdown_prepare(rc_dev);
		}
		if (kirin_pcie_power_on(pp, RC_POWER_SUSPEND)) {
			PCIE_PR_ERR("Failed to power off ");
			return -EINVAL;
		}
	}

	PCIE_PR_INFO("--");

	return 0;
}

#else

#define kirin_pcie_suspend_noirq NULL
#define kirin_pcie_resume_noirq NULL

#endif

static const struct dev_pm_ops kirin_pcie_dev_pm_ops = {
	.suspend_noirq	= kirin_pcie_suspend_noirq,
	.resume_noirq	= kirin_pcie_resume_noirq,
};

static const struct of_device_id kirin_pcie_match_table[] = {
	{
		.compatible = "hisilicon,kirin-pcie",
		.data = NULL,
	},
	/*lint -e785 -esym(785,*)*/
	{},
	/*lint -e785 +esym(785,*)*/
};

struct platform_driver kirin_pcie_driver = {
	.probe			= kirin_pcie_probe,
	.shutdown		= kirin_pcie_shutdown,
	.driver			= {
		.name			= "Kirin-pcie",
		.pm				= &kirin_pcie_dev_pm_ops,
		.of_match_table = kirin_pcie_match_table,
		.suppress_bind_attrs = true
	},
};

static int kirin_pcie_usr_suspend(u32 rc_idx, int power_off_ops)
{
	int ret;
	int val;
	struct pcie_port *pp;
	struct pci_dev *rc_dev;
	struct kirin_pcie *pcie = &g_kirin_pcie[rc_idx];

	PCIE_PR_INFO("++");

	if (atomic_read(&(pcie->usr_suspend)) || !atomic_read(&(pcie->is_power_on))) {
		PCIE_PR_ERR("Already suspend by EP ");
		return -EINVAL;
	}

	pp = &pcie->pp;
	rc_dev = pcie->rc_dev;

	if (!rc_dev) {
		PCIE_PR_ERR("Failed to get RC dev");
		return -1;
	}

	if (power_off_ops != POWEROFF_BUSDOWN) {
		kirin_pcie_lp_ctrl(rc_idx, DISABLE);
		ret = kirin_pcie_shutdown_prepare(rc_dev);
		if (ret)
			return -EINVAL;
	}
	/*phy rst from sys to pipe */
	val = kirin_apb_phy_readl(pcie, SOC_PCIEPHY_CTRL1_ADDR);
	val |= 0x1 << 17;
	kirin_apb_phy_writel(pcie, val, SOC_PCIEPHY_CTRL1_ADDR);

	ret = kirin_pcie_power_on(pp, RC_POWER_OFF);
	if (ret) {
		PCIE_PR_ERR("Failed to power off ");
		return -EINVAL;
	}

	atomic_set(&(pcie->usr_suspend), 1);

	PCIE_PR_INFO("--");
	return 0;
}

static int kirin_pcie_usr_resume(u32 rc_idx)
{
	int ret;
	struct pcie_port *pp;
	struct kirin_pcie_dtsinfo *dtsinfo;
	struct kirin_pcie *pcie = &g_kirin_pcie[rc_idx];

	PCIE_PR_INFO("++");

	pp = &pcie->pp;
	dtsinfo = &pcie->dtsinfo;

	atomic_set(&(pcie->usr_suspend), 0);

	ret = kirin_pcie_power_on(pp, RC_POWER_ON);
	if (ret) {
		PCIE_PR_ERR("Failed to power on");

		atomic_set(&(pcie->usr_suspend), 1);
		return -EINVAL;
	}

	ret = kirin_pcie_establish_link(&pcie->pp);
	if (ret) {
		if (kirin_pcie_power_on(pp, RC_POWER_OFF))
			PCIE_PR_ERR("Failed to power off");

		atomic_set(&(pcie->usr_suspend), 1);
		return -EINVAL;
	}

	kirin_pcie_restore_rc_cfg(pcie);

	kirin_pcie_lp_ctrl(rc_idx, ENABLE);

	PCIE_PR_INFO("--");

	return 0;
}

/*
* EP Power ON/OFF callback Function:
* param: rc_idx---which rc the EP link with
*        power_ops---2: PowerOFF without PME, 1:PowerOn, 0: PowerOFF normally
*/
int kirin_pcie_pm_control(int power_ops, u32 rc_idx)
{
	PCIE_PR_DEBUG("RC = [%u], power_ops[%d]", rc_idx, power_ops);

	if (rc_idx >= g_rc_num) {
		PCIE_PR_ERR("There is no rc_id = %d", rc_idx);
		return -EINVAL;
	}

	if (!atomic_read(&(g_kirin_pcie[rc_idx].is_ready))) {
		PCIE_PR_ERR("PCIe driver is not ready");
		return -1;
	}

	if (power_ops == POWERON ) {
		dsm_pcie_clear_info();
		return kirin_pcie_usr_resume(rc_idx);
	} else if (power_ops == POWEROFF_BUSON|| power_ops == POWEROFF_BUSDOWN){
		return kirin_pcie_usr_suspend(rc_idx, power_ops);
	} else {
		PCIE_PR_ERR("Invalid power_ops[%d]", power_ops);
		return -EINVAL;
	}
}
EXPORT_SYMBOL_GPL(kirin_pcie_pm_control);

int kirin_pcie_ep_off(u32 rc_idx)
{
	struct kirin_pcie *pcie = &g_kirin_pcie[rc_idx];

	return  ( (atomic_read(&(pcie->usr_suspend)) == 1) ||
		(atomic_read(&(pcie->is_power_on)) == 0));
}
EXPORT_SYMBOL_GPL(kirin_pcie_ep_off);

/*
* API FOR EP to control L1&L1-substate
* param: rc_idx---which rc the EP link with
*        enable---KIRIN_PCIE_LP_ON:enable L1 and L1-substate,
*				  KIRIN_PCIE_LP_Off: disable, others: illegal
*/
int kirin_pcie_lp_ctrl(u32 rc_idx, u32 enable)
{
	struct kirin_pcie * pcie = &g_kirin_pcie[rc_idx];
	struct kirin_pcie_dtsinfo *dtsinfo;

	PCIE_PR_DEBUG("++");

	if (rc_idx >= g_rc_num) {
		PCIE_PR_ERR("There is no rc_id = %d", rc_idx);
		return -EINVAL;
	}
	if (!atomic_read(&(pcie->is_ready))) {
		PCIE_PR_ERR("PCIe driver is not ready");
		return -EINVAL;
	}

	if (!atomic_read(&(pcie->is_power_on))) {
		PCIE_PR_ERR("PCIe%d is power off ", rc_idx);
		return -EINVAL;
	}

	dtsinfo = &(pcie->dtsinfo);

	if (enable) {
		PCIE_PR_DEBUG("Enable");
		if (pcie->dtsinfo.board_type == BOARD_ASIC) {
			kirin_pcie_config_l0sl1(pcie->rc_id,
				(enum link_aspm_state)dtsinfo->aspm_state);
			kirin_pcie_config_l1ss(pcie->rc_id, L1SS_PM_ASPM_ALL);
		}
	} else {
		PCIE_PR_DEBUG("Disable");
		kirin_pcie_config_l1ss(pcie->rc_id, L1SS_CLOSE);
		kirin_pcie_config_l0sl1(pcie->rc_id, ASPM_CLOSE);
	}

	PCIE_PR_DEBUG("-");

	return 0;
}
EXPORT_SYMBOL_GPL(kirin_pcie_lp_ctrl);

/*
* Enumerate Function:
* param: rc_idx---which rc the EP link with
*/
int kirin_pcie_enumerate(u32 rc_idx)
{
	int ret;
	u32 val;
	u32 dev_id;
	u32 vendor_id;
	struct pcie_port *pp;
	struct pci_bus *bus1;
	struct pci_dev *dev;
	struct kirin_pcie *pcie;
	struct kirin_pcie_dtsinfo *dtsinfo;

	PCIE_PR_INFO("++");
	PCIE_PR_DEBUG("RC[%u] begin to Enumerate ", rc_idx);

	if (rc_idx >= g_rc_num) {
		PCIE_PR_ERR("There is no rc_id = %d", rc_idx);
		return -EINVAL;
	}
	pcie = &g_kirin_pcie[rc_idx];
	pp = &pcie->pp;
	dtsinfo = &pcie->dtsinfo;

	if (!atomic_read(&(pcie->is_ready))) {
		PCIE_PR_ERR("PCIe driver is not ready");
		return -1;
	}

	if (atomic_read(&(pcie->is_enumerated))) {
		PCIE_PR_ERR("Enumeration was done successed before");
		return 0;
	}

	/*clk on*/
	ret = kirin_pcie_power_on(pp, RC_POWER_ON);
	if (ret) {
		PCIE_PR_ERR("Failed to power RC");
		dsm_pcie_dump_info(pcie, DSM_ERR_POWER_ON);
		return ret;
	}

	kirin_pcie_readl_rc(pp, pp->dbi_base, &val);
	val += rc_idx;
	kirin_pcie_writel_rc(pp, val, pp->dbi_base);

	if (dtsinfo->board_type == BOARD_FPGA) {
		kirin_pcie_writel_rc(pp, 0x10002, pp->dbi_base + 0xa0);
	}

	ret = dw_pcie_host_init(pp);
	if (ret) {
		PCIE_PR_ERR("Failed to initialize host");
		dsm_pcie_dump_info(pcie, DSM_ERR_ENUMERATE);
		goto FAIL_TO_POWEROFF;
	}

	kirin_pcie_rd_own_conf(pp, PCI_VENDOR_ID, 2, &vendor_id);
	kirin_pcie_rd_own_conf(pp, PCI_DEVICE_ID, 2, &dev_id);
	pcie->rc_dev = pci_get_device(vendor_id, dev_id, pcie->rc_dev);
	if (!pcie->rc_dev) {
		PCIE_PR_ERR("Failed to get RC device ");
		goto FAIL_TO_POWEROFF;
	}

	ret = kirin_pcie_save_rc_cfg(pcie);
	if (ret)
		goto FAIL_TO_POWEROFF;

	bus1 = pcie->rc_dev->subordinate;
	if (bus1) {
		list_for_each_entry(dev, &bus1->devices, bus_list) {
			if (pci_is_pcie(dev)) {
				pcie->ep_dev = dev;
				atomic_set(&(pcie->is_enumerated), 1);
				pcie->ep_devid = dev->device;
				pcie->ep_venid = dev->vendor;
				PCIE_PR_INFO("ep vendorid = 0x%x, deviceid = 0x%x",
					pcie->ep_venid, pcie->ep_devid);
			}
		}
	} else {
		PCIE_PR_ERR("Bus1 is null");
		pcie->ep_dev = NULL;
		pci_stop_and_remove_bus_device(pcie->rc_dev);
		goto FAIL_TO_POWEROFF;
	}

	if (!pcie->ep_dev) {
		PCIE_PR_ERR("There is no ep dev");
		pci_stop_and_remove_bus_device(pcie->rc_dev);
		goto FAIL_TO_POWEROFF;
	}

	kirin_pcie_lp_ctrl(pcie->rc_id, ENABLE);

	atomic_set(&(pcie->usr_suspend), 0);

	PCIE_PR_INFO("--");
	return 0;

FAIL_TO_POWEROFF:
	if (kirin_pcie_power_on(pp, RC_POWER_OFF)) {
			PCIE_PR_ERR("Failed to power off.");
	}
	return -1;
}
EXPORT_SYMBOL(kirin_pcie_enumerate);

int pcie_ep_link_ltssm_notify(u32 rc_id, u32 link_status)
{
	struct kirin_pcie *pcie;

	if (rc_id >= g_rc_num) {
		PCIE_PR_ERR("There is no rc_id = %d", rc_id);
		return -EINVAL;
	}

	if (link_status >= DEVICE_LINK_MAX || link_status <= DEVICE_LINK_MIN) {
		PCIE_PR_ERR("Invalid Device link status[%d]", link_status);
		return -EINVAL;
	}

	pcie = &g_kirin_pcie[rc_id];

	if (!atomic_read(&pcie->is_power_on)) {
		PCIE_PR_ERR("PCIe is Poweroff");
		return -EINVAL;
	}

	spin_lock(&pcie->ep_ltssm_lock);
	pcie->ep_link_status = link_status;
	spin_unlock(&pcie->ep_ltssm_lock);

	return 0;
}
EXPORT_SYMBOL(pcie_ep_link_ltssm_notify);

/*lint -e438 -e550 -e713 -e732 -e737 -e774 -e838 +esym(438,*) +esym(713,*) +esym(732,*) +esym(737,*) +esym(774,*) +esym(550,*) +esym(838,*) */

builtin_platform_driver(kirin_pcie_driver);

MODULE_AUTHOR("Xiaowei Song<songxiaowei@huawei.com>");
MODULE_DESCRIPTION("Hisilicon Kirin pcie driver");
MODULE_LICENSE("GPL");
