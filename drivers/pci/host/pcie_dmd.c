#include "pcie-kirin.h"
#include <dsm/dsm_pub.h>


#if defined (CONFIG_HUAWEI_DSM)
#define DSM_LOG_BUFFER_SIZE 256
#define RC_AER_OFFSET 0x100

static u32 dsm_record_info[DSM_LOG_BUFFER_SIZE];
u32 info_size = 0;

void dsm_pcie_dump_info(struct kirin_pcie *pcie, enum dsm_err_id id)
{
	u32 reg_val = 0;
	struct pcie_port *pp;
	u32 i = 0;
	int cap_aer;

	if (!pcie || !atomic_read(&(pcie->is_power_on)))
		return;

	dsm_record_info[i++] = id;
	dsm_record_info[i++] = gpio_get_value(pcie->gpio_id_reset);
	dsm_record_info[i++] = kirin_apb_phy_readl(pcie, SOC_PCIEPHY_STATE0_ADDR);
	dsm_record_info[i++] = kirin_elb_readl(pcie, SOC_PCIECTRL_STATE4_ADDR);
	dsm_record_info[i++] = kirin_elb_readl(pcie, SOC_PCIECTRL_STATE5_ADDR);
	dsm_record_info[i++] = kirin_elb_readl(pcie, PCIE_APP_LTSSM_ENABLE);

	/*RC aer register*/
	pp = &pcie->pp;
	/*for linkdown, read cfg will prevented by link status*/
	cap_aer = RC_AER_OFFSET;
	kirin_pcie_rd_own_conf(pp, cap_aer + 0x4, 4, &reg_val);
	dsm_record_info[i++] = reg_val;
	kirin_pcie_rd_own_conf(pp, cap_aer + 0x10, 4, &reg_val);
	dsm_record_info[i++] = reg_val;
	kirin_pcie_rd_own_conf(pp, cap_aer + 0x30, 4, &reg_val);
	dsm_record_info[i++] = reg_val;
	kirin_pcie_rd_own_conf(pp, cap_aer + 0x34, 4, &reg_val);
	dsm_record_info[i++] = reg_val;

	info_size = i;

	PCIE_PR_INFO("--");
}

void dsm_pcie_clear_info(void)
{
	u32 i;
	info_size = 0;
	for (i = 0; i < DSM_LOG_BUFFER_SIZE; i++)
		dsm_record_info[i] = 0;
}

/*
* return pcie dsm log_buffer addr
* log string is stroaged in this buffer
* param:
*	buf - storage register value for wifi dmd_log
*	buflen - buf size reserved for PCIe, max 384
*/
/*lint -e679 -esym(679,*) */
void dsm_pcie_dump_reginfo(char* buf, u32 buflen) {
    u32 i = 0;
    u32 seglen = 9;
    if (buf != NULL && buflen > (seglen * info_size)) {
        for (i = 0; i < info_size; i++) {
            snprintf(&buf[i * seglen], seglen + 1, "%08x ", dsm_record_info[i]);
        }
    }
}
EXPORT_SYMBOL_GPL(dsm_pcie_dump_reginfo);

#else

void dsm_pcie_dump_info(struct kirin_pcie *pcie, enum dsm_err_id id)
{
	return;
}

void dsm_pcie_clear_info(void)
{
	return;
}

/*
* return pcie dsm log_buffer addr
* log string is stroaged in this buffer
*/
u32 dsm_pcie_dump_reginfo(void *addr)
{
	return;
}
EXPORT_SYMBOL_GPL(dsm_pcie_dump_reginfo);
#endif

