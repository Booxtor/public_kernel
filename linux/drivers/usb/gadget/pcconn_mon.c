#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/usb/otg.h>
#include <linux/fsl_devices.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>

#include "arcotg_udc.h"
#include <mach/arc_otg.h>

#define DRIVER_DESC		"PC connection monitor driver"
#define DRIVER_AUTHOR	"ivan@onyx-international.com"
#define	DRIVER_VERSION	"1 July 2009"
#define FSL_UDC_RESET_TIMEOUT 1000

static const char driver_name[] = "fsl-usb2-udc";
volatile static struct usb_dr_device *dr_regs;
static struct fsl_usb2_platform_data *pdata = NULL;

static void deferred_notify(struct work_struct *work)
{
	kobject_uevent(&pdata->pdev->dev.kobj, KOBJ_ADD);
}
static DECLARE_WORK(notify_work, deferred_notify);

static int dr_controller_setup(void)
{
	unsigned int tmp = 0, portctrl = 0;
	unsigned int __attribute((unused)) ctrl = 0;
	unsigned long timeout;

	/* Stop and reset the usb controller */
	tmp = readl(&dr_regs->usbcmd);
	tmp &= ~USB_CMD_RUN_STOP;
	writel(tmp, &dr_regs->usbcmd);

	tmp = readl(&dr_regs->usbcmd);
	tmp |= USB_CMD_CTRL_RESET;
	writel(tmp, &dr_regs->usbcmd);

	/* Wait for reset to complete */
	timeout = jiffies + FSL_UDC_RESET_TIMEOUT;
	while (readl(&dr_regs->usbcmd) & USB_CMD_CTRL_RESET) {
		if (time_after(jiffies, timeout)) {
			ERR("udc reset timeout! \n");
			return -ETIMEDOUT;
		}
		cpu_relax();
	}

	/* Set the controller as device mode */
	tmp = readl(&dr_regs->usbmode);
	tmp &= ~USB_MODE_CTRL_MODE_MASK;	/* clear mode bits */
	tmp |= USB_MODE_CTRL_MODE_DEVICE;
	/* Disable Setup Lockout */
	tmp |= USB_MODE_SETUP_LOCK_OFF;
	if (pdata->es)
		tmp |= USB_MODE_ES;
	writel(tmp, &dr_regs->usbmode);

	fsl_platform_set_device_mode(pdata);

	/* Clear the setup status */
	writel(0, &dr_regs->usbsts);

	// tmp = udc->ep_qh_dma;
	// tmp &= USB_EP_LIST_ADDRESS_MASK;
	// writel(tmp, &dr_regs->endpointlistaddr);

	// VDBG("vir[qh_base] is %p phy[qh_base] is 0x%8x reg is 0x%8x",
	//	(int)udc->ep_qh, (int)tmp,
	//	readl(&dr_regs->endpointlistaddr));

	/* Config PHY interface */
	portctrl = readl(&dr_regs->portsc1);
	portctrl &= ~(PORTSCX_PHY_TYPE_SEL | PORTSCX_PORT_WIDTH);
	portctrl |= PORTSCX_PTS_ULPI;
	writel(portctrl, &dr_regs->portsc1);

	if (pdata->change_ahb_burst) {
		/* if usb should not work in default INCRx mode */
		tmp = readl(&dr_regs->sbuscfg);
		tmp = (tmp & ~0x07) | pdata->ahb_burst_mode;
		writel(tmp, &dr_regs->sbuscfg);
	}

	return 0;
}

/* Enable DR irq and set controller to run state */
static void dr_controller_run(void)
{
	u32 temp;

	/* Enable DR irq reg */
	temp = USB_INTR_INT_EN | USB_INTR_ERR_INT_EN
		| USB_INTR_PTC_DETECT_EN | USB_INTR_RESET_EN
		| USB_INTR_DEVICE_SUSPEND | USB_INTR_SYS_ERR_EN;
	writel(temp, &dr_regs->usbintr);

	temp = readl(&dr_regs->usbcmd);
	temp |= USB_CMD_RUN_STOP;
	writel(temp, &dr_regs->usbcmd);

	return;
}

static void dr_controller_stop(void)
{
	unsigned int tmp;

	/* disable all INTR */
	writel(0, &dr_regs->usbintr);

	/* set controller to Stop */
	tmp = readl(&dr_regs->usbcmd);
	tmp &= ~USB_CMD_RUN_STOP;
	writel(tmp, &dr_regs->usbcmd);

	return;
}

static irqreturn_t pcconn_mon_irq(int irq, void *dev_id)
{
	u32 irq_src = dr_regs->usbsts & dr_regs->usbintr;

	/* Clear notification bits */
	dr_regs->usbsts &= irq_src;

	irq_src = le32_to_cpu(irq_src);
	if (irq_src & USB_STS_RESET)
	{
		// Connected to PC
		dr_controller_stop();
		
		// Notify user space applications.
		schedule_work(&notify_work);
	}

	return IRQ_HANDLED;
}

static int __init pcconn_mon_probe(struct platform_device *pdev)
{
	int ret;
	int irq;
	struct resource *res;

	pdata = pdev->dev.platform_data;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
	{
		ret = -ENXIO;
		goto err1;
	}

	if (!request_mem_region(res->start, res->end - res->start + 1,
				driver_name)) {
		ERR("request mem region for %s failed \n", pdev->name);
		ret = -EBUSY;
		goto err1;
	}

	dr_regs = ioremap(res->start, res->end - res->start + 1);
	if (!dr_regs)
	{
		ret = -ENOMEM;
		goto err2;
	}

	/*
	 * do platform specific init: check the clock, grab/config pins, etc.
	 */
	if (pdata->platform_init && pdata->platform_init(pdev)) {
		ret = -ENODEV;
		goto err3;
	}

	irq = platform_get_irq(pdev, 0);
	ret = request_irq(irq, pcconn_mon_irq, IRQF_SHARED,
		driver_name, (void *)pdev);
	if (ret != 0)
	{
		ERR("cannot request irq %d err %d \n", irq, ret);
		goto err4;
	}

	// Start DR controller.
	dr_controller_setup();
	dr_controller_run();

	return 0;

err4:
	pdata->platform_uninit(pdata);
err3:
	iounmap(dr_regs);
err2:
	release_mem_region(res->start, res->end - res->start + 1);
err1:
	return ret;
}

static int __exit pcconn_mon_remove(struct platform_device *pdev)
{
	int irq;
	struct resource *res;

	irq = platform_get_irq(pdev, 0);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	// Stop DR controller.
	dr_controller_stop();
	free_irq(irq, (void *)pdev);
	iounmap(dr_regs);
	release_mem_region(res->start, res->end - res->start + 1);

	/*
	 * do platform specific un-initialization:
	 * release iomux pins, etc.
	 */
	if (pdata->platform_uninit)
		pdata->platform_uninit(pdata);

	return 0;
}

/*!
 * Register entry point for the peripheral controller driver
 */
static struct platform_driver udc_driver =
{
	.probe = pcconn_mon_probe,
	.remove = __exit_p(pcconn_mon_remove),
	.driver =
	{
		.name = driver_name,
	},
};

static int __init pcconn_mon_init(void)
{
	printk(KERN_INFO "%s (%s)\n", DRIVER_DESC, DRIVER_VERSION);
	return platform_driver_register(&udc_driver);
}
module_init(pcconn_mon_init);

static void __exit pcconn_mon_exit(void)
{
	platform_driver_unregister(&udc_driver);
	printk(KERN_INFO "%s unregistered \n", DRIVER_DESC);
}
module_exit(pcconn_mon_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE("GPL");
