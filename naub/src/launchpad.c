/*
  Copyright 2011 David Robillard <http://drobilla.net>

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

#include <stdio.h>
#include <stdlib.h>

#include "naub_internal.h"

typedef enum {
	GROUP_MATRIX,
	GROUP_TOP,
	GROUP_RIGHT
} LaunchpadGroup;

static const uint8_t STATUS_TOP    = 0xB0;
static const uint8_t STATUS_MATRIX = 0x90;

static NaubStatus
launchpad_read(NaubDevice* device, const uint8_t* buf, size_t size)
{
	NaubWorld*     naub  = device->naub;
	const uint32_t dev   = device->id;
	size_t         start = 0;
	if (buf[0] >= 0x80) {
		// Update running status
		device->status = buf[0];
		start          = 1;
	}

	NaubControlID id = { dev, 0, 0, 0 };
	if (device->status == STATUS_TOP) {
		id.group = GROUP_TOP;
		id.x     = buf[start] - 0x68;
	} else if (device->status == STATUS_MATRIX && buf[start] / 8 % 2 == 1) {
		id.group = GROUP_RIGHT;
		id.y     = buf[start] >> 4;
	} else if (device->status == STATUS_MATRIX) {
		id.group = GROUP_MATRIX;
		id.x     = buf[start] & 0x0F;
		id.y     = buf[start] >> 4;
	} else {
		return NAUB_ERR_BAD_DATA;
	}

	const NaubEventTouch ev = { NAUB_EVENT_BUTTON, id, buf[start + 1] };
	naub->event_cb(naub->handle, (const NaubEvent*)&ev);

	return NAUB_SUCCESS;
}

static inline int32_t
clamp(int32_t value, int32_t min, int32_t max)
{
	if (value < min) {
		return min;
	} else if (value > max) {
		return max;
	}
	return value;
}

static NaubStatus
launchpad_set_control(NaubDevice* device,
                      uint32_t    group,
                      uint32_t    x,
                      uint32_t    y,
                      int32_t     value)
{
	const float   red        = ((value & 0x00FF0000) >> 16) / 255.0f;  // 0..1
	const float   green      = ((value & 0x0000FF00) >> 8) / 255.0f;  // 0..1
	const uint8_t red_bits   = red * 3.0f;  // 0..3
	const uint8_t green_bits = green * 3.0f;  // 0..3
	const uint8_t vel        = (green_bits << 4) + red_bits;
	switch (group) {
	case GROUP_MATRIX:
		if (x < 8 && y < 8) {
			const uint8_t buf[] = { STATUS_MATRIX, y * 16 + x, vel };
			return naub_write(device, buf, sizeof(buf));
		}
	case GROUP_TOP:
		if (x < 8 && y == 0) {
			const uint8_t buf[] = { STATUS_TOP, 0x68 + x, vel };
			return naub_write(device, buf, sizeof(buf));
		}
	case GROUP_RIGHT:
		if (x == 0 && y < 8) {
			const uint8_t buf[] = { STATUS_MATRIX, y * 16 + 8, vel };
			return naub_write(device, buf, sizeof(buf));
		}
	}
	return NAUB_ERR_BAD_CONTROL;
}

static NaubStatus
launchpad_set_led_mode(NaubDevice* device,
                       uint32_t    group,
                       uint32_t    x,
                       uint32_t    y,
                       NaubLEDMode mode)
{
	return NAUB_ERR_UNSUPPORTED;
}

NaubDevice*
launchpad_open(NaubWorld* naub, libusb_device_handle* handle)
{
	NaubDevice* device = (NaubDevice*)malloc(sizeof(NaubDevice));
	if (!device) {
		return NULL;
	}

	device->naub         = naub;
	device->handle       = handle;
	device->read         = launchpad_read;
	device->set_control  = launchpad_set_control;
	device->set_led_mode = launchpad_set_led_mode;
	device->id           = naub->n_devices;

	if (libusb_kernel_driver_active(device->handle, 0)) {
		if (libusb_detach_kernel_driver(device->handle, 0)) {
			fprintf(stderr, "Error detaching existing kernel driver\n");
			libusb_close(device->handle);
			free(device);
			return NULL;
		}
	}

	if (libusb_claim_interface(device->handle, 0) != 0) {
		fprintf(stderr, "Failed to claim interface\n");
		libusb_close(device->handle);
		free(device);
		return NULL;
	}

	libusb_reset_device(device->handle);

	device->write_transfer = NULL;
	device->read_transfer  = libusb_alloc_transfer(0);
	device->read_data_len  = 16;
	device->read_data      = (uint8_t*)malloc(device->read_data_len);
	device->status         = 0;

	libusb_fill_interrupt_transfer(
		device->read_transfer, device->handle, NAUB_USB_IN,
		device->read_data, device->read_data_len,
		naub_usb_read_cb, device, 0);

	libusb_submit_transfer(device->read_transfer);

	return device;
}
