/// device.h: Interface of all devices.
///
/// - author: Zhengcheng Huang
/// - cited work:
///

#ifndef _DEVICE_H
#define _DEVICE_H

#include <types.h>

struct dev_op;
struct device;

typedef struct dev_op {
    int32_t (*read)(struct device * self, void * buf, uint32_t offset, uint32_t nbytes);
    int32_t (*write)(struct device * self, const void * buf, uint32_t offset, uint32_t nbytes);
} dev_op_t;

typedef struct device {
    int8_t * dev_name;
    uint8_t dev_id;
    dev_op_t * d_op;
    void * priv_data;
} device_t;

#endif /* _DEVICE_H */
