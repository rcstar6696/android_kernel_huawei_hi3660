/*
 * Hisilicon Amba-pl011.h.
 *
 * Copyright (c) 2016 HUAWEI.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef HISI_AMBA_PL011_H
#define HISI_AMBA_PL011_H

#include <linux/kfifo.h>
#include <linux/debugfs.h>
#include <linux/of_address.h>
#include <linux/ioport.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/serial_core.h>
#include <linux/clk.h>
#include <linux/amba/bus.h>
#include <linux/amba/serial.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/kthread.h>

#define UART_NR			14

struct vendor_data {
	unsigned int		ifls;
	unsigned int		lcrh_tx;
	unsigned int		lcrh_rx;
	bool			oversampling;
	bool			dma_threshold;
	bool			cts_event_workaround;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
	bool			always_enabled;
	bool			fixed_options;
#endif

	unsigned int (*get_fifosize)(struct amba_device *dev);
};

struct pl011_sgbuf {
	struct scatterlist sg;
	char *buf;
};

struct pl011_dmarx_data {
	struct dma_chan		*chan;
	struct completion	complete;
	bool			use_buf_b;
	struct pl011_sgbuf	sgbuf_a;
	struct pl011_sgbuf	sgbuf_b;
	dma_cookie_t		cookie;
	bool			running;
	struct timer_list	timer;
	unsigned int last_residue;
	unsigned long last_jiffies;
	bool auto_poll_rate;
	unsigned int poll_rate;
	unsigned int poll_timeout;
};

struct pl011_dmatx_data {
	struct dma_chan		*chan;
	struct scatterlist	sg;
	char			*buf;
	bool			queued;
};

#ifdef CONFIG_SERIAL_AMBA_PL011_CONSOLE
#define PL011_UART_TX_WORK
static int console_uart_name_is_ttyAMA;
extern int get_console_index(void);
extern int get_console_name(char *name, int name_buf_len);
#define PL011_TX_LOCAL_BUF_SIZE	96
#define UART_TX_BUF_SIZE_DEF	0x20000	/* 256k */
#define UART_TX_CPUON_DEF		3		/* default cpu id */
#define UART_TX_CPUON_NOTSET	(-1)
struct uart_tx_unit {
	unsigned int			tx_valid; /* enable or not */
	int						tx_cpuid;   /* -1:not set */
	int						max_cpus;
	struct kfifo			tx_fifo;
	spinlock_t				tx_lock_in;
	unsigned long			tx_got;
	unsigned long			tx_queued;
	unsigned long			tx_out;
	unsigned long			tx_sent;
	unsigned long			tx_buf_times;
	unsigned long			tx_buf_max; /* fifo required size max */
	unsigned long			tx_buf_over_cnt;
	unsigned long			tx_buf_over_size;
	unsigned long			tx_uart_fifo_full;
	unsigned long			tx_uart_fifo_full_lost;
	unsigned long			tx_uart_tasklet_run;
	wait_queue_head_t		waitqueue;
	atomic_t			 	tx_sig;
	struct task_struct		*thread;
};
#endif

 struct uart_amba_port {
	struct uart_port	port;
	struct clk		*clk;
	/* Two optional pin states - default & sleep */
	struct pinctrl		*pinctrl;
	struct pinctrl_state	*pins_default;
	struct pinctrl_state	*pins_idle;
	const struct vendor_data *vendor;
	unsigned int		dmacr;		/* dma control reg */
	unsigned int		im;		/* interrupt mask */
	unsigned int		old_status;
	unsigned int		fifosize;	/* vendor-specific */
	unsigned int		lcrh_tx;	/* vendor-specific */
	unsigned int		lcrh_rx;	/* vendor-specific */
	unsigned int		old_cr;		/* state during shutdown */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
#else
	struct delayed_work	tx_softirq_work;
#endif
	bool			autorts;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
	unsigned int		fixed_baud;	/* vendor-set fixed baud rate */
#endif
	unsigned int		tx_irq_seen;	/* 0=none, 1=1, 2=2 or more */
	char			type[12];
#ifdef CONFIG_DMA_ENGINE
	/* DMA stuff */
	bool			using_tx_dma;
	bool			using_rx_dma;
	struct pl011_dmarx_data dmarx;
	struct pl011_dmatx_data	dmatx;
	bool			dma_probed;
#endif
	int			pm_op_in_progress;
#ifdef CONFIG_SERIAL_AMBA_PL011_CONSOLE
	bool reset;
	void __iomem *reset_reg_base;
	void __iomem *reset_clk_reg_base;
	u32 reset_clk_reg_offset;
	u32 reset_enable_off;
	u32 reset_disable_off;
	u32 reset_status_off;
	u32 reset_bit;
	u32 console_fbrd;
	u32 console_ibrd;
	u32 console_lcrh_rx;
	u32 console_lcrh_tx;
	struct uart_tx_unit tx_unit;
#endif
};
static struct uart_amba_port *amba_ports[UART_NR];

void uart_chip_reset_endisable(struct uart_amba_port *dev, unsigned int enable);
void hisi_pl011_console_write(struct console *co, const char *s, unsigned int count);
#ifdef CONFIG_SERIAL_AMBA_PL011_CONSOLE
int pl011_tx_work_init(struct uart_amba_port *uap, unsigned int aurt_tx_buf_size, int cpuid);
int pl011_tx_work_uninit(struct uart_amba_port *uap);
void pl011_console_write_tx(struct console *co, const char *s, unsigned int count);
void pl011_console_putchar(struct uart_port *port, int ch);
#endif
void hisi_pl011_probe_console_enable(struct amba_device *dev, struct uart_amba_port *uap, char* amba_console_name);
int hisi_pl011_probe_reset_func_enable(struct amba_device *dev, struct uart_amba_port *uap);
int hisi_pl011_probe_get_clk_freq(struct amba_device *dev, struct uart_amba_port *uap, int i);
void pl011_pm(struct uart_port *port, unsigned int state, unsigned int oldstate);
#ifdef CONFIG_PM_SLEEP
int hisi_pl011_suspend(struct device *dev);
int hisi_pl011_resume(struct device *dev);
#endif
int hisi_pl011_pinctrl(struct amba_device *dev, struct uart_amba_port *uap);
void hisi_pl011_disable_ms(struct uart_port *port);
#endif
void uartprint_enable(struct amba_device *dev, struct uart_amba_port *uap);
