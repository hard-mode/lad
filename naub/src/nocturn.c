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
	STRIP_KNOBS,
	STRIP_BUTTONS,
	GLOBAL_BUTTONS,
	CENTER_CONTROLS
} NocturnGroup;

typedef enum {
	NOCTURN_KNOB_1 = 0x40,
	NOCTURN_KNOB_2,
	NOCTURN_KNOB_3,
	NOCTURN_KNOB_4,
	NOCTURN_KNOB_5,
	NOCTURN_KNOB_6,
	NOCTURN_KNOB_7,
	NOCTURN_KNOB_8,
	NOCTURN_FADER,
	NOCTURN_SPEED_DIAL,
	NOCTURN_BUTTON_1 = 0x70,
	NOCTURN_BUTTON_2,
	NOCTURN_BUTTON_3,
	NOCTURN_BUTTON_4,
	NOCTURN_BUTTON_5,
	NOCTURN_BUTTON_6,
	NOCTURN_BUTTON_7,
	NOCTURN_BUTTON_8,
	NOCTURN_BUTTON_LEARN,
	NOCTURN_BUTTON_VIEW,
	NOCTURN_BUTTON_PAGE_DOWN,
	NOCTURN_BUTTON_PAGE_UP,
	NOCTURN_BUTTON_USER,
	NOCTURN_BUTTON_FX,
	NOCTURN_BUTTON_INST,
	NOCTURN_BUTTON_MIXER,
} NocturnControl;

static NaubStatus
nocturn_read(NaubDevice* device, const uint8_t* buf, size_t size)
{
	NaubWorld*     naub   = device->naub;
	const uint32_t dev    = device->id;
	const uint8_t  status = buf[1];
	const uint8_t  data   = buf[2];
	if ((status & 0xF0) == 0x60) {  // Strip knob touch
		const uint8_t num = status & 0x0F;
		const NaubEventTouch ev = {
			NAUB_EVENT_TOUCH, { dev, STRIP_KNOBS, num, 0 }, data };
		naub->event_cb(naub->handle, (const NaubEvent*)&ev);
	} else if ((status & 0xF0) == 0x50) {  // Center control touch
		const uint8_t num = (status & 0x0F) - 2;
		const NaubEventTouch ev = {
			NAUB_EVENT_TOUCH, { dev, CENTER_CONTROLS, 0, num }, data };
		naub->event_cb(naub->handle, (const NaubEvent*)&ev);
	} else if (status == 0x48) {  // Fader set
		const NaubEventSet ev = {
			NAUB_EVENT_SET, { dev, CENTER_CONTROLS, 0, 1 }, data };
		naub->event_cb(naub->handle, (const NaubEvent*)&ev);
	} else if (status == 0x4A) {  // Speed dial increment
		NaubEventIncrement ev = {
			NAUB_EVENT_INCREMENT, { dev, CENTER_CONTROLS, 0, 0 }, data };
		if (data >= 0x40) {  // Negative increment
			ev.delta = 0 - (0x80 - ev.delta);
		}
		naub->event_cb(naub->handle, (const NaubEvent*)&ev);
	} else if ((status & 0xF0) == 0x40) {  // Knob increment
		const uint8_t num = status & 0x0F;
		NaubEventIncrement ev = {
			NAUB_EVENT_INCREMENT, { dev, STRIP_KNOBS, num, 0 }, data };
		if (data >= 0x40) {  // Negative increment
			ev.delta = 0 - (0x80 - ev.delta);
		}
		naub->event_cb(naub->handle, (const NaubEvent*)&ev);
	} else if ((status & 0xF0) == 0x70) {  // Button press/release
		const uint8_t num = status & 0x0F;
		NaubEventButton ev = {
			NAUB_EVENT_BUTTON, { dev, STRIP_BUTTONS, num, 0 }, data };
		if (num > 7) {  // Global button
			ev.control.group = GLOBAL_BUTTONS;
			ev.control.x     = num - 8;
		}
		naub->event_cb(naub->handle, (const NaubEvent*)&ev);
	} else {
		//fprintf(stderr, "Unknown message %X\n", naub->read_data[1]);
		fprintf(stderr, "Unknown message\n");
		return NAUB_ERR_BAD_DATA;
	}

	return NAUB_SUCCESS;
}

static NaubStatus
nocturn_set_control(NaubDevice* device,
                    uint32_t    group,
                    uint32_t    x,
                    uint32_t    y,
                    int32_t     value)
{
	if (group == CENTER_CONTROLS && y == 1) {
		return NAUB_ERR_UNSUPPORTED;  // Attempt to set fader
	} else if (group == STRIP_KNOBS) {
		if (y != 0 || x > 7) {
			return NAUB_ERR_BAD_CONTROL;
		}
		const uint8_t buf[] = { NOCTURN_KNOB_1 + x, naub_clamp(value, 0, 127) };
		return naub_write(device, buf, sizeof(buf));
	} else if (group == STRIP_BUTTONS || group == GLOBAL_BUTTONS) {
		if (y != 0 || x > 7) {
			return NAUB_ERR_BAD_CONTROL;
		}
		const uint8_t buf[] = { ((group == STRIP_BUTTONS)
		                         ? NOCTURN_BUTTON_1 + x
		                         : NOCTURN_BUTTON_LEARN + x),
		                        naub_clamp(value, 0, 1) };
		return naub_write(device, buf, sizeof(buf));
	}
	return NAUB_ERR_BAD_CONTROL;
}

static NaubStatus
nocturn_set_led_mode(NaubDevice* device,
                     uint32_t    group,
                     uint32_t    x,
                     uint32_t    y,
                     NaubLEDMode mode)
{
	if ((group == STRIP_KNOBS && x < 8 && y == 0) ||
	    (group == CENTER_CONTROLS && x == 0 && y == 0)) {
		const uint8_t buf[] = { 0x48 + x, mode << 4 };
		//naub_write(naub, 0x48 + (control - NAUB_KNOB_1), mode << 4);
		naub_write(device, buf, sizeof(buf));
		return NAUB_SUCCESS;
	} else {
		return NAUB_ERR_BAD_CONTROL;
	}
}

NaubDevice*
nocturn_open(NaubWorld* naub, libusb_device_handle* handle)
{
	NaubDevice* device = (NaubDevice*)malloc(sizeof(NaubDevice));
	if (!device) {
		return NULL;
	}

	device->naub         = naub;
	device->handle       = handle;
	device->read         = nocturn_read;
	device->set_control  = nocturn_set_control;
	device->set_led_mode = nocturn_set_led_mode;
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

	int transferred = 0;

	uint8_t init_1[] = { 0xB0, 0x00, 0x00 };
	uint8_t init_2[] = { 0x28, 0x00, 0x2B, 0x4A, 0x2C, 0x00, 0x2E, 0x35 };
	uint8_t init_3[] = { 0x2A, 0x02, 0x2C, 0x72, 0x2E, 0x30 };
	uint8_t init_4[] = { 0x7F, 0x00 };
	libusb_interrupt_transfer(
		device->handle, NAUB_USB_OUT, init_1, sizeof(init_1), &transferred, 0);
	libusb_interrupt_transfer(
		device->handle, NAUB_USB_OUT, init_2, sizeof(init_2), &transferred, 0);
	libusb_interrupt_transfer(
		device->handle, NAUB_USB_OUT, init_3, sizeof(init_3), &transferred, 0);
	libusb_interrupt_transfer(
		device->handle, NAUB_USB_OUT, init_4, sizeof(init_4), &transferred, 0);

	device->write_transfer = NULL;
	device->read_transfer  = libusb_alloc_transfer(0);
	device->read_data_len  = 16;
	device->read_data      = (uint8_t*)malloc(device->read_data_len);

	libusb_fill_interrupt_transfer(
		device->read_transfer, device->handle, NAUB_USB_IN,
		device->read_data, device->read_data_len,
		naub_usb_read_cb, device, 0);

	libusb_submit_transfer(device->read_transfer);

	/*
	  for (int x = 0; x < 8; ++x) {
	  nocturn_set_led_mode(
	  device, STRIP_KNOBS, x, 0, x % (NAUB_LED_SINGLE_INVERTED + 1));
	  nocturn_set_control(device, STRIP_KNOBS, x, 0, 32);//(128 / 8) * x);
	  }
	*/

	return device;
}
