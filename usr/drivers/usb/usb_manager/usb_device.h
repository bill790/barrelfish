/*
 * Copyright (c) 2007-2013 ETH Zurich.
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached LICENSE file.
 * If you do not find this file, copies can be found by writing to:
 * ETH Zurich D-INFK, Haldeneggsteig 4, CH-8092 Zurich. Attn: Systems Group.
 */


#ifndef USB_DEVICE_H_
#define USB_DEVICE_H_

struct usb_device;

#define USB_MAX_DEVICES 127


void usb_device_insert_pending(struct usb_device *new_device);
void usb_device_config_complete(struct usb_device *new_device);
struct usb_device * usb_device_get_pending(void);
struct usb_device *usb_device_get_by_address(uint8_t address);






#endif /* USB_DEVICE_H_ */