/*
 * File:   fusb3601_driver.c
 */

/* Standard Linux includes */
#include <linux/init.h>       /* __init, __initdata, etc */
#include <linux/module.h>     /* Needed to be a module */
#include <linux/kernel.h>     /* Needed to be a kernel module */
#include <linux/i2c.h>        /* I2C functionality */
#include <linux/slab.h>       /* devm_kzalloc */
#include <linux/types.h>      /* Kernel datatypes */
#include <linux/errno.h>      /* EINVAL, ERANGE, etc */
#include <linux/of_device.h>  /* Device tree functionality */
#include <linux/delay.h>

/* Driver-specific includes */
#include "fusb3601_global.h"  /* Driver-specific structures/types */
#include "platform_helpers.h" /* I2C R/W, GPIO, misc, etc */

#ifdef FSC_DEBUG
#include "../core/core.h"     /* GetDeviceTypeCStatus */
#include "../core/hw_scp.h"
#include "../core/moisture_detection.h"
#include "dfs.h"
#endif /* FSC_DEBUG */

#include "fusb3601_driver.h"
#ifdef CONFIG_CONTEXTHUB_PD
#include <linux/hisi/contexthub/tca.h>
#endif

#ifdef CONFIG_CONTEXTHUB_PD
extern bool hisi_dptx_ready(void);
#endif
/******************************************************************************
* Driver functions
******************************************************************************/
static int __init fusb3601_init(void)
{
        pr_debug("FUSB  %s - Start driver initialization...vHW1.0.22\n", __func__);

        return i2c_add_driver(&fusb3601_driver);
}

static void __exit fusb3601_exit(void)
{
        i2c_del_driver(&fusb3601_driver);
        pr_debug("FUSB  %s - Driver deleted...\n", __func__);
}

static int fusb3601_i2c_resume(struct device* dev)
{
        struct fusb3601_chip *chip;
        struct i2c_client *client = to_i2c_client(dev);

        if (client) {
                chip = i2c_get_clientdata(client);
                if (chip)
                        up(&chip->suspend_lock);
        }
        return 0;
}

static int fusb3601_i2c_suspend(struct device* dev)
{
        struct fusb3601_chip* chip;
        struct i2c_client* client =  to_i2c_client(dev);

        if (client) {
          chip = i2c_get_clientdata(client);
          if (chip)
                  down(&chip->suspend_lock);
        }
        return 0;
}
static int fusb3601_probe_work(struct work_struct *work)
{
	struct fusb3601_chip *chip = container_of(work, struct fusb3601_chip, fusb3601_probe_work);
	struct i2c_client *client;
	int ret;
#ifdef CONFIG_CONTEXTHUB_PD
	int count = 10;/*wait until dp phy is ready, timeout is 10*100ms*/
#endif
	if (!chip || NULL == chip->client) {
                pr_err("FUSB  %s - Error: Client structure is NULL!\n",
                       __func__);
                return -EINVAL;
	}
	client = chip->client;
#ifdef CONFIG_CONTEXTHUB_PD
	do {
		msleep(100);
		count--;
		if(true == hisi_dptx_ready())
			break;
	} while(count);
#endif
	/* reset fusb3601*/
        FUSB3601_fusb_reset();

        /* Initialize the platform's GPIO pins and IRQ */
        ret = FUSB3601_fusb_InitializeGPIO();
        if (ret)
        {
                dev_err(&client->dev,
                        "FUSB  %s - Error: Unable to initialize GPIO!\n",
                        __func__);
                return ret;
        }
        pr_debug("FUSB  %s - GPIO initialized!\n", __func__);

#ifdef FSC_DEBUG
        /* Initialize debug sysfs file accessors */
        FUSB3601_fusb_Sysfs_Init();
        pr_debug("FUSB  %s - Sysfs device file created!\n", __func__);

        fusb_InitializeDFS();
        pr_debug("FUSB  %s - DebugFS entry created!\n", __func__);
#endif /* FSC_DEBUG */

        /* Initialize the core and enable the state machine
         * (NOTE: timer and GPIO must be initialized by now)
         * Interrupt must be enabled before starting 3601 initialization */
        FUSB3601_fusb_InitializeCore();
	FUSB3601_scp_initialize();
	moisture_detection_init();
        pr_debug("FUSB  %s - Core is initialized!\n", __func__);

        /* Enable interrupts after successful core/GPIO initialization */
        ret = FUSB3601_fusb_EnableInterrupts();
        if (ret)
        {
                dev_err(&client->dev,
            "FUSB  %s - Error: Unable to enable interrupts! Error code: %d\n",
                        __func__, ret);
                return -EIO;
        }

        dev_info(&client->dev,
                 "FUSB  %s - FUSB3601 Driver loaded successfully!\n",
                 __func__);
        return ret;

}
static int fusb3601_probe (struct i2c_client* client,
                          const struct i2c_device_id* id)
{
        int ret = 0;
        struct fusb3601_chip* chip;
        struct i2c_adapter* adapter;

        if (!client)
        {
                pr_err("FUSB  %s - Error: Client structure is NULL!\n",
                       __func__);
                return -EINVAL;
        }

        dev_info(&client->dev, "%s\n", __func__);

        /* Make sure probe was called on a compatible device */
        if (!of_match_device(fusb3601_dt_match, &client->dev))
        {
                dev_err(&client->dev,
                        "FUSB  %s - Error: Device tree mismatch!\n", __func__);
                return -EINVAL;
        }

        pr_debug("FUSB  %s - Device tree matched!\n", __func__);

        /* Alloc space for our chip struct (devm_* is managed by the device) */
        chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
        if (!chip)
        {
                dev_err(&client->dev,
                    "FUSB  %s - Error: Unable to allocate memory for g_chip!\n",
                        __func__);
                return -ENOMEM;
        }

        /* Assign our client handle to our chip */
        chip->client = client;

        /* Set our global chip's address to the newly allocated memory */
        fusb3601_SetChip(chip);

        pr_debug("FUSB  %s - Chip structure is set! Chip: %p ... g_chip: %p\n",
                 __func__, chip, fusb3601_GetChip());

        /* Initialize semaphore*/
        sema_init(&chip->suspend_lock, 1);

        /* Initialize the chip lock */
        mutex_init(&chip->lock);

        /* Initialize the chip's data members */
        FUSB3601_fusb_InitChipData();
        pr_debug("FUSB  %s - Chip struct data initialized!\n", __func__);

        /* Verify that the system has our required I2C/SMBUS functionality
         * (see <linux/i2c.h> for definitions)
         */
        adapter = to_i2c_adapter(client->dev.parent);
        if (i2c_check_functionality(adapter,
                                    FUSB3601_I2C_SMBUS_BLOCK_REQUIRED_FUNC))
        {
                chip->use_i2c_blocks = true;
        }
        else
        {
                /* If the platform doesn't support block reads, try with block
                 * writes and single reads (works with eg. RPi)
                 * It is likely that this may result in non-standard behavior,
                 * but will often be 'close enough' to work for most things
                 */
                dev_warn(&client->dev,
                    "FUSB %s - Warning: I2C/SMBus block rd/wr not supported,\n",
                         __func__);
                dev_warn(&client->dev,
                    "FUSB %s -     checking single-read mode...\n",
                         __func__);

                if (!i2c_check_functionality(adapter,
                                             FUSB3601_I2C_SMBUS_REQUIRED_FUNC))
                {
                        dev_err(&client->dev,
          "FUSB  %s - Error: Required I2C/SMBus functionality not supported!\n",
                                __func__);
                        dev_err(&client->dev,
                          "FUSB  %s - I2C Supported Functionality Mask: 0x%x\n",
                                __func__, i2c_get_functionality(adapter));
                        return -EIO;
                }
        }

        pr_err("FUSB  %s - I2C Functionality check passed! Block reads: %s\n",
                 __func__, chip->use_i2c_blocks ? "YES" : "NO");

        /* Assign our struct as the client's driverdata */
        i2c_set_clientdata(client, chip);
        pr_debug("FUSB  %s - I2C client data set!\n", __func__);

        /* Verify that our device exists and that it's what we expect */
        if (!FUSB3601_fusb_IsDeviceValid())
        {
                dev_err(&client->dev,
                      "FUSB  %s - Error: Unable to communicate with device!\n",
                        __func__);
                return -EIO;
        }
        pr_debug("FUSB  %s - Device check passed!\n", __func__);
	FUSB3601_charge_register_callback();

	INIT_WORK(&chip->fusb3601_probe_work, fusb3601_probe_work);
	schedule_work(&chip->fusb3601_probe_work);
	return 0;
}

static int fusb3601_remove(struct i2c_client* client)
{
        pr_debug("FUSB  %s - Removing fusb3601 device!\n", __func__);

        FUSB3601_fusb_GPIO_Cleanup();

#ifdef FSC_DEBUG
        fusb_DFS_Cleanup();
#endif /* FSC_DEBUG */

        pr_debug("FUSB  %s - FUSB3601 device removed from driver.\n", __func__);
        return 0;
}

static void fusb3601_shutdown(struct i2c_client *client)
{
        FUSB3601_fusb_reset();
        pr_debug("FUSB  %s - fusb3601 shutdown\n", __func__);
}


/*******************************************************************************
 * Driver macros
 ******************************************************************************/
module_init(fusb3601_init);    /* Defines the module's entrance function */
module_exit(fusb3601_exit);    /* Defines the module's exit function */

/* Exposed on call to modinfo */
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Fairchild FUSB3601 Driver");
