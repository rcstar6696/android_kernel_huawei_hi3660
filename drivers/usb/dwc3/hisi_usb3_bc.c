#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/bitops.h>

#include "dwc3-hisi.h"
extern struct hisi_dwc3_device *hisi_dwc3_dev;
static void phy_cr_wait_ack(void __iomem *otg_bc_base)
{
	int i = 1000;

	while (1) {
		if ((readl(otg_bc_base + USB3PHY_CR_STS) & USB3OTG_PHY_CR_ACK) == 1)
			break;
		udelay(50);
		if (i-- < 0) {
			usb_err("wait phy_cr_ack timeout!\n");
			break;
		}
	}
}

static void phy_cr_set_addr(void __iomem *otg_bc_base, u32 addr)
{
	u32 reg;

	/* set addr */
	reg = USB3OTG_PHY_CR_DATA_IN(addr);
	writel(reg, otg_bc_base + USB3PHY_CR_CTRL);

	udelay(100);

	/* cap addr */
	reg = readl(otg_bc_base + USB3PHY_CR_CTRL);
	reg |= USB3OTG_PHY_CR_CAP_ADDR;
	writel(reg, otg_bc_base + USB3PHY_CR_CTRL);

	phy_cr_wait_ack(otg_bc_base);

	/* clear ctrl reg */
	writel(0, otg_bc_base + USB3PHY_CR_CTRL);
}

static u16 phy_cr_read(void __iomem *otg_bc_base, u32 addr)
{
	u32 reg;
	int i = 1000;

	phy_cr_set_addr(otg_bc_base, addr);

	/* read cap */
	writel(USB3OTG_PHY_CR_READ, otg_bc_base + USB3PHY_CR_CTRL);

	udelay(100);

	while (1) {
		reg = readl(otg_bc_base + USB3PHY_CR_STS);
		if ((reg & USB3OTG_PHY_CR_ACK) == 1) {
			break;
		}
		udelay(50);
		if (i-- < 0) {
			usb_err("wait phy_cr_ack timeout!\n");
			break;
		}
	}

	/* clear ctrl reg */
	writel(0, otg_bc_base + USB3PHY_CR_CTRL);

	return (u16)USB3OTG_PHY_CR_DATA_OUT(reg);
}

static void phy_cr_write(void __iomem *otg_bc_base, u32 addr, u32 value)
{
	u32 reg;

	phy_cr_set_addr(otg_bc_base, addr);

	reg = USB3OTG_PHY_CR_DATA_IN(value);
	writel(reg, otg_bc_base + USB3PHY_CR_CTRL);

	/* cap data */
	reg = readl(otg_bc_base + USB3PHY_CR_CTRL);
	reg |= USB3OTG_PHY_CR_CAP_DATA;
	writel(reg, otg_bc_base + USB3PHY_CR_CTRL);

	/* wait ack */
	phy_cr_wait_ack(otg_bc_base);

	/* clear ctrl reg */
	writel(0, otg_bc_base + USB3PHY_CR_CTRL);

	reg = USB3OTG_PHY_CR_WRITE;
	writel(reg, otg_bc_base + USB3PHY_CR_CTRL);

	/* wait ack */
	phy_cr_wait_ack(otg_bc_base);
}

void set_usb3_phy_cr_param(u32 addr, u32 value)
{
	if (!hisi_dwc3_dev) {
		pr_err("hisi dwc3 device not ready!\n");
		return;
	}

	phy_cr_write(hisi_dwc3_dev->otg_bc_reg_base, addr, value);
}
EXPORT_SYMBOL_GPL(set_usb3_phy_cr_param);

void read_usb3_phy_cr_param(u32 addr)
{
	if (!hisi_dwc3_dev) {
		pr_err("hisi dwc3 device not ready!\n");
		return;
	}

	usb_dbg("read usb3 phy cr param 0x%x\n",
		phy_cr_read(hisi_dwc3_dev->otg_bc_reg_base, addr));
}
EXPORT_SYMBOL_GPL(read_usb3_phy_cr_param);


/* BC1.2 Spec:
 * If a PD detects that D+ is greater than VDAT_REF, it knows that it is
 * attached to a DCP. It is then required to enable VDP_SRC or pull D+
 * to VDP_UP through RDP_UP */
void disable_vdp_src(struct hisi_dwc3_device *hisi_dwc3)
{
	void __iomem *base = hisi_dwc3->otg_bc_reg_base;
	uint32_t reg;

	usb_dbg("+\n");

	if (hisi_dwc3->vdp_src_enable == 0)
		return;
	hisi_dwc3->vdp_src_enable = 0;

	usb_dbg("diaable VDP_SRC\n");

	reg = readl(base + BC_CTRL2);
	reg &= ~(BC_CTRL2_BC_PHY_VDATARCENB | BC_CTRL2_BC_PHY_VDATDETENB);
	writel(reg, base + BC_CTRL2);

	reg = readl(base + BC_CTRL0);
	reg |= BC_CTRL0_BC_SUSPEND_N;
	writel(reg, base + BC_CTRL0);

	writel((readl(base + BC_CTRL1) & ~BC_CTRL1_BC_MODE), base + BC_CTRL1);
	usb_dbg("-\n");
}

void enable_vdp_src(struct hisi_dwc3_device *hisi_dwc3)
{
	void __iomem *base = hisi_dwc3->otg_bc_reg_base;
	uint32_t reg;

	usb_dbg("+\n");

	if (hisi_dwc3->vdp_src_enable != 0)
		return;
	hisi_dwc3->vdp_src_enable = 1;

	usb_dbg("enable VDP_SRC\n");
	reg = readl(base + BC_CTRL2);
	reg &= ~BC_CTRL2_BC_PHY_CHRGSEL;
	reg |= (BC_CTRL2_BC_PHY_VDATARCENB | BC_CTRL2_BC_PHY_VDATDETENB);
	writel(reg, base + BC_CTRL2);
	usb_dbg("-\n");
}

static int is_dcd_timeout(void __iomem *base)
{
	unsigned long jiffies_expire;
	uint32_t reg;
	int ret = 0;
	int i = 0;

	jiffies_expire = jiffies + msecs_to_jiffies(900);
	msleep(50);
	while (1) {
		reg = readl(base + BC_STS0);
		if ((reg & BC_STS0_BC_PHY_FSVPLUS) == 0) {
			i++;
			if (i >= 10)
				break;
		} else {
			i = 0;
		}

		msleep(10);

		if (time_after(jiffies, jiffies_expire)) {
			usb_dbg("DCD timeout!\n");
			ret = -1;
			break;
		}
	}

	return ret;
}

enum hisi_charger_type detect_charger_type(struct hisi_dwc3_device *hisi_dwc3)
{
	enum hisi_charger_type type = CHARGER_TYPE_NONE;
	void __iomem *base = hisi_dwc3->otg_bc_reg_base;
	uint32_t reg;
	unsigned long flags;

	usb_dbg("+\n");

	if (hisi_dwc3->fpga_flag) {
		usb_dbg("this is fpga platform, charger is SDP\n");
		return CHARGER_TYPE_SDP;
	}

	if (hisi_dwc3->fake_charger_type != CHARGER_TYPE_NONE) {
		usb_dbg("fake type: %d\n", hisi_dwc3->fake_charger_type);
		return hisi_dwc3->fake_charger_type;
	}

	writel(BC_CTRL1_BC_MODE, base + BC_CTRL1);

	/* phy suspend */
	reg = readl(base + BC_CTRL0);
	reg &= ~BC_CTRL0_BC_SUSPEND_N;
	writel(reg, base + BC_CTRL0);

	spin_lock_irqsave(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
	/* enable DCD */
	reg = readl(base + BC_CTRL2);
	reg |= BC_CTRL2_BC_PHY_DCDENB;
	writel(reg, base + BC_CTRL2);
	spin_unlock_irqrestore(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/

	reg = readl(base + BC_CTRL0);
	reg |= BC_CTRL0_BC_DMPULLDOWN;
	writel(reg, base + BC_CTRL0);

	if (is_dcd_timeout(base)){
		type = CHARGER_TYPE_UNKNOWN;
	}

	reg = readl(base + BC_CTRL0);
	reg &= ~BC_CTRL0_BC_DMPULLDOWN;
	writel(reg, base + BC_CTRL0);

	spin_lock_irqsave(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
	/* disable DCD */
	reg = readl(base + BC_CTRL2);
	reg &= ~BC_CTRL2_BC_PHY_DCDENB;
	writel(reg, base + BC_CTRL2);

	usb_dbg("DCD done\n");

	if (type == CHARGER_TYPE_NONE) {
		/* enable vdect */
		reg = readl(base + BC_CTRL2);
		reg &= ~BC_CTRL2_BC_PHY_CHRGSEL;
		reg |= (BC_CTRL2_BC_PHY_VDATARCENB | BC_CTRL2_BC_PHY_VDATDETENB);
		writel(reg, base + BC_CTRL2);
		spin_unlock_irqrestore(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/

		msleep(40);

		/* we can detect sdp or cdp dcp */
		reg = readl(base + BC_STS0);
		if ((reg & BC_STS0_BC_PHY_CHGDET) == 0) {
			type = CHARGER_TYPE_SDP;
		}

		spin_lock_irqsave(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
		/* disable vdect */
		reg = readl(base + BC_CTRL2);
		reg &= ~(BC_CTRL2_BC_PHY_VDATARCENB | BC_CTRL2_BC_PHY_VDATDETENB);
		writel(reg, base + BC_CTRL2);
	}

	usb_dbg("Primary Detection done\n");

	if (type == CHARGER_TYPE_NONE) {
		/* enable vdect */
		reg = readl(base + BC_CTRL2);
		reg |= (BC_CTRL2_BC_PHY_VDATARCENB | BC_CTRL2_BC_PHY_VDATDETENB
				| BC_CTRL2_BC_PHY_CHRGSEL);
		writel(reg, base + BC_CTRL2);
		spin_unlock_irqrestore(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/

		msleep(40);

		/* we can detect sdp or cdp dcp */
		reg = readl(base + BC_STS0);
		if ((reg & BC_STS0_BC_PHY_CHGDET) == 0)
			type = CHARGER_TYPE_CDP;
		else
			type = CHARGER_TYPE_DCP;

		spin_lock_irqsave(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
		/* disable vdect */
		reg = readl(base + BC_CTRL2);
		reg &= ~(BC_CTRL2_BC_PHY_VDATARCENB | BC_CTRL2_BC_PHY_VDATDETENB
				| BC_CTRL2_BC_PHY_CHRGSEL);
		writel(reg, base + BC_CTRL2);
	}
	spin_unlock_irqrestore(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/

	usb_dbg("Secondary Detection done\n");

	/* If a PD detects that D+ is greater than VDAT_REF, it knows that it is
	 * attached to a DCP. It is then required to enable VDP_SRC or pull D+
	 * to VDP_UP through RDP_UP */
	if (type == CHARGER_TYPE_DCP) {
		usb_dbg("charger is DCP, enable VDP_SRC\n");
		spin_lock_irqsave(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
		enable_vdp_src(hisi_dwc3);
		spin_unlock_irqrestore(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
	} else {
		/* bc_suspend = 1, nomal mode */
		reg = readl(base + BC_CTRL0);
		reg |= BC_CTRL0_BC_SUSPEND_N;
		writel(reg, base + BC_CTRL0);

		msleep(10);

		/* disable BC */
		writel((readl(base + BC_CTRL1) & ~BC_CTRL1_BC_MODE), base + BC_CTRL1);
	}

	if (type == CHARGER_TYPE_CDP) {
		usb_dbg("it needs enable VDP_SRC while detect CDP!\n");
		spin_lock_irqsave(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
		enable_vdp_src(hisi_dwc3);
		spin_unlock_irqrestore(&hisi_dwc3->bc_again_lock, flags);/*lint !e550*/
	}

	usb_dbg("charger type: %s\n", charger_type_string(type));
	usb_dbg("-\n");
	return type;
}

void config_femtophy_param(struct hisi_dwc3_device *hisi_dwc)
{
	uint32_t reg;
	void __iomem *otg_bc_base = hisi_dwc->otg_bc_reg_base;

	if (hisi_dwc->fpga_flag != 0)
		return;

	/* set high speed phy parameter */
	if (hisi_dwc->host_flag) {
		writel(hisi_dwc->eye_diagram_host_param, otg_bc_base + USBOTG3_CTRL4);
		usb_dbg("set hs phy param 0x%x for host\n",
				readl(otg_bc_base + USBOTG3_CTRL4));
	} else {
		writel(hisi_dwc->eye_diagram_param, otg_bc_base + USBOTG3_CTRL4);
		usb_dbg("set hs phy param 0x%x for device\n",
				readl(otg_bc_base + USBOTG3_CTRL4));
	}

	/* set usb3 phy cr config for usb3.0 */

	if (hisi_dwc->host_flag) {
		phy_cr_write(otg_bc_base, DWC3_PHY_RX_OVRD_IN_HI,
				hisi_dwc->usb3_phy_host_cr_param);
	} else {
		phy_cr_write(otg_bc_base, DWC3_PHY_RX_OVRD_IN_HI,
				hisi_dwc->usb3_phy_cr_param);
	}

	usb_dbg("set ss phy rx equalization 0x%x\n",
			phy_cr_read(otg_bc_base, DWC3_PHY_RX_OVRD_IN_HI));

	/* enable RX_SCOPE_LFPS_EN for usb3.0 */
	reg = phy_cr_read(otg_bc_base, DWC3_PHY_RX_SCOPE_VDCC);
	reg |= RX_SCOPE_LFPS_EN;
	phy_cr_write(otg_bc_base, DWC3_PHY_RX_SCOPE_VDCC, reg);

	usb_dbg("set ss RX_SCOPE_VDCC 0x%x\n",
			phy_cr_read(otg_bc_base, DWC3_PHY_RX_SCOPE_VDCC));

	reg = readl(otg_bc_base + USBOTG3_CTRL6);
	reg &= ~TX_VBOOST_LVL_MASK;
	reg |= TX_VBOOST_LVL(hisi_dwc->usb3_phy_tx_vboost_lvl);
	writel(reg, otg_bc_base + USBOTG3_CTRL6);
	usb_dbg("set ss phy tx vboost lvl 0x%x\n", readl(otg_bc_base + USBOTG3_CTRL6));
}
