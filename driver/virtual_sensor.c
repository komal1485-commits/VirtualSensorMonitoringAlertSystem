// SPDX-License-Identifier: GPL-2.0
/*
 * virtual_sensor - character device producing SIMULATED temperature samples.
 * Targets Linux >= 6.4 (class_create() one-argument form).
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/random.h>
#include "../include/vs_ioctl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
MODULE_DESCRIPTION("Virtual (simulated) temperature sensor character driver");

#define TEMP_START_MDEG   25000
#define TEMP_MIN_MDEG    (-20000)
#define TEMP_MAX_MDEG    120000

static dev_t vs_devno;
static struct cdev vs_cdev;
static struct class *vs_class;
static DEFINE_MUTEX(vs_lock);

static s32 vs_temp = TEMP_START_MDEG;
static u32 vs_seq;
static u32 vs_mode = VS_MODE_NORMAL;

static s32 target_for_mode(u32 mode)
{
	switch (mode) {
	case VS_MODE_HIGH: return 60000;
	case VS_MODE_LOW:  return 5000;
	default:           return 25000;
	}
}

/* Caller holds vs_lock. Moves 1/5 of the way to target plus +-0.4 C noise. */
static void vs_step(void)
{
	s32 target = target_for_mode(vs_mode);
	s32 noise = (s32)get_random_u32_below(801) - 400;

	vs_temp += (target - vs_temp) / 5 + noise;
	vs_temp = clamp_t(s32, vs_temp, TEMP_MIN_MDEG, TEMP_MAX_MDEG);
	vs_seq++;
}

static ssize_t vs_read(struct file *f, char __user *buf, size_t count, loff_t *off)
{
	struct vs_sample s = { 0 };

	if (count < sizeof(s))
		return -EINVAL;
	if (mutex_lock_interruptible(&vs_lock))
		return -ERESTARTSYS;
	vs_step();
	s.timestamp_ns = ktime_get_real_ns();
	s.temp_mdeg = vs_temp;
	s.seq = vs_seq;
	s.mode = vs_mode;
	mutex_unlock(&vs_lock);

	if (copy_to_user(buf, &s, sizeof(s)))
		return -EFAULT;
	return sizeof(s);
}

static long vs_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	u32 val;

	if (_IOC_TYPE(cmd) != VS_IOC_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case VS_IOC_SET_MODE:
		if (get_user(val, (u32 __user *)arg))
			return -EFAULT;
		if (val > VS_MODE_LOW)
			return -EINVAL;
		mutex_lock(&vs_lock);
		vs_mode = val;
		mutex_unlock(&vs_lock);
		return 0;
	case VS_IOC_GET_MODE:
		mutex_lock(&vs_lock);
		val = vs_mode;
		mutex_unlock(&vs_lock);
		return put_user(val, (u32 __user *)arg);
	case VS_IOC_RESET:
		mutex_lock(&vs_lock);
		vs_mode = VS_MODE_NORMAL;
		vs_temp = TEMP_START_MDEG;
		vs_seq = 0;
		mutex_unlock(&vs_lock);
		return 0;
	default:
		return -ENOTTY;
	}
}

static const struct file_operations vs_fops = {
	.owner          = THIS_MODULE,
	.read           = vs_read,
	.unlocked_ioctl = vs_ioctl,
	.llseek         = noop_llseek,
};

static int __init vs_init(void)
{
	struct device *dev;
	int ret;

	ret = alloc_chrdev_region(&vs_devno, 0, 1, VS_DEVICE_NAME);
	if (ret)
		return ret;

	cdev_init(&vs_cdev, &vs_fops);
	vs_cdev.owner = THIS_MODULE;
	ret = cdev_add(&vs_cdev, vs_devno, 1);
	if (ret)
		goto err_region;

	vs_class = class_create(VS_DEVICE_NAME);
	if (IS_ERR(vs_class)) {
		ret = PTR_ERR(vs_class);
		goto err_cdev;
	}

	dev = device_create(vs_class, NULL, vs_devno, NULL, VS_DEVICE_NAME);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		goto err_class;
	}

	pr_info("virtual_sensor: loaded (major %d), SIMULATED data only\n", MAJOR(vs_devno));
	return 0;

err_class:
	class_destroy(vs_class);
err_cdev:
	cdev_del(&vs_cdev);
err_region:
	unregister_chrdev_region(vs_devno, 1);
	return ret;
}

static void __exit vs_exit(void)
{
	device_destroy(vs_class, vs_devno);
	class_destroy(vs_class);
	cdev_del(&vs_cdev);
	unregister_chrdev_region(vs_devno, 1);
	pr_info("virtual_sensor: unloaded\n");
}

module_init(vs_init);
module_exit(vs_exit);
