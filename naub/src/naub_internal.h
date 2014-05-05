/*
  Copyright 2011-2012 David Robillard <http: //drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef NAUB_INTERNAL_H
#define NAUB_INTERNAL_H

#include <libusb.h>
#include <stddef.h>

#include "naub/naub.h"

#include "zix/ring.h"

#define NAUB_USB_OUT (2 | LIBUSB_ENDPOINT_OUT)
#define NAUB_USB_IN  (1 | LIBUSB_ENDPOINT_IN)

#define NOVATION_VENDOR_ID   0x1235
#define NOCTURN_PRODUCT_ID   0x000A
#define LAUNCHPAD_PRODUCT_ID 0x000E

typedef struct NaubDeviceImpl NaubDevice;

struct NaubDeviceImpl {
	NaubWorld*              naub;
	libusb_device_handle*   handle;
	struct libusb_transfer* read_transfer;
	struct libusb_transfer* write_transfer;
	uint32_t                id;
	uint8_t                 status;
	uint8_t*                read_data;
	size_t                  read_data_len;

	NaubStatus (*read)(NaubDevice*    device,
	                   const uint8_t* buf,
	                   size_t         size);

	NaubStatus (*set_control)(NaubDevice* device,
	                          uint32_t    group,
	                          uint32_t    x,
	                          uint32_t    y,
	                          int32_t     value);

	NaubStatus (*set_led_mode)(NaubDevice* device,
	                           uint32_t    group,
	                           uint32_t    x,
	                           uint32_t    y,
	                           NaubLEDMode mode);
};

struct NaubWorldImpl {
	libusb_context* context;
	void*           handle;
	NaubEventFunc   event_cb;
	unsigned        n_devices;
	NaubDevice**    devices;
};

NaubDevice* nocturn_open(NaubWorld* naub, libusb_device_handle* handle);
NaubDevice* launchpad_open(NaubWorld* naub, libusb_device_handle* handle);

void naub_device_close(NaubDevice* device);

NaubStatus naub_write(NaubDevice* device, const uint8_t* buf, size_t len);

void naub_usb_read_cb(struct libusb_transfer* transfer);

static inline int32_t
naub_clamp(int32_t value, int32_t min, int32_t max)
{
	if (value < min) {
		return min;
	} else if (value > max) {
		return max;
	}
	return value;
}

#endif  /* NAUB_INTERNAL_H */
