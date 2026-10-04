/* Shared kernel/user ABI for /dev/virtual_sensor. All readings are SIMULATED. */
#ifndef VS_IOCTL_H
#define VS_IOCTL_H

#include <linux/types.h>
#include <linux/ioctl.h>

#define VS_DEVICE_NAME "virtual_sensor"
#define VS_DEVICE_PATH "/dev/virtual_sensor"

enum vs_mode { VS_MODE_NORMAL = 0, VS_MODE_HIGH = 1, VS_MODE_LOW = 2 };

/* Returned by read(); one sample per read() call. */
struct vs_sample {
	__s64 timestamp_ns;   /* ktime_get_real_ns() at generation time */
	__s32 temp_mdeg;      /* simulated temperature, milli-degrees C */
	__u32 seq;            /* sample counter since load/reset */
	__u32 mode;           /* enum vs_mode currently active */
	__u32 reserved;
};

#define VS_IOC_MAGIC    'v'
#define VS_IOC_SET_MODE _IOW(VS_IOC_MAGIC, 1, __u32)
#define VS_IOC_GET_MODE _IOR(VS_IOC_MAGIC, 2, __u32)
#define VS_IOC_RESET    _IO(VS_IOC_MAGIC, 3)

#endif
