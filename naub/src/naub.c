/*
  Copyright 2011 David Robillard <http: //drobilla.net>

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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "naub_internal.h"

static void
usb_write_cb(struct libusb_transfer* transfer)
{
	switch (transfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		//printf("USB write completed\n");
		break;
	case LIBUSB_TRANSFER_ERROR:
		printf("USB write error\n");
		break;
	case LIBUSB_TRANSFER_TIMED_OUT:
		printf("USB write timed out\n");
		break;
	case LIBUSB_TRANSFER_CANCELLED:
		printf("USB write cancelled\n");
		break;
	case LIBUSB_TRANSFER_STALL:
		printf("USB write stall\n");
		break;
	case LIBUSB_TRANSFER_NO_DEVICE:
		printf("USB write no device\n");
		break;
	case LIBUSB_TRANSFER_OVERFLOW:
		printf("USB write overflow\n");
		break;
	}
}

void
naub_usb_read_cb(struct libusb_transfer* transfer)
{
	NaubDevice* device = (NaubDevice*)transfer->user_data;

	switch (transfer->status) {
	case LIBUSB_TRANSFER_COMPLETED:
		//printf("USB read completed\n");
		device->read(device, transfer->buffer, transfer->actual_length);

		// Re-submit transfer to read the next data when it arrives
		libusb_submit_transfer(transfer);
		break;
	case LIBUSB_TRANSFER_ERROR:
		printf("USB read error\n");
		break;
	case LIBUSB_TRANSFER_TIMED_OUT:
		printf("USB read timed out\n");
		break;
	case LIBUSB_TRANSFER_CANCELLED:
		printf("USB read cancelled\n");
		break;
	case LIBUSB_TRANSFER_STALL:
		printf("USB read stall\n");
		break;
	case LIBUSB_TRANSFER_NO_DEVICE:
		printf("USB read no device\n");
		break;
	case LIBUSB_TRANSFER_OVERFLOW:
		printf("USB read overflow\n");
		break;
	}
}

NaubStatus
naub_write(NaubDevice* device, const uint8_t* buf, size_t len)
{
	if (!device->write_transfer) {
		device->write_transfer = libusb_alloc_transfer(0);
		libusb_fill_interrupt_transfer(
			device->write_transfer, device->handle, NAUB_USB_OUT,
			NULL, 0, usb_write_cb, device, 0);
		device->write_transfer->flags = (LIBUSB_TRANSFER_FREE_BUFFER |
		                                 LIBUSB_TRANSFER_FREE_TRANSFER);
	}

	struct libusb_transfer* transfer = device->write_transfer;

	uint8_t* msg = (uint8_t*)realloc(transfer->buffer, transfer->length + len);
	memcpy(msg + transfer->length, buf, len);
	device->write_transfer->buffer = msg;
	device->write_transfer->length += len;
	return NAUB_SUCCESS;
}

NAUB_API
NaubStatus
naub_flush(NaubWorld* naub)
{
	for (unsigned i = 0; i < naub->n_devices; ++i) {
		NaubDevice* device = naub->devices[i];
		if (device->write_transfer) {
			libusb_submit_transfer(device->write_transfer);
			device->write_transfer = NULL;
		}
	}
	return NAUB_SUCCESS;
}

NAUB_API
NaubStatus
naub_set_control(NaubWorld*    naub,
                 NaubControlID control,
                 int32_t       value)
{
	NaubDevice* device = naub->devices[control.device];
	return device->set_control(
		device, control.group, control.x, control.y, value);
}

NAUB_API
NaubStatus
naub_set_led_mode(NaubWorld*    naub,
                  NaubControlID control,
                  NaubLEDMode   mode)
{
	NaubDevice* device = naub->devices[control.device];
	return device->set_led_mode(
		device, control.group, control.x, control.y, mode);
}

static void
add_device(NaubWorld* naub, NaubDevice* device)
{
	if (device) {
		naub->devices                      = (NaubDevice**)realloc(
			naub->devices, ++naub->n_devices * sizeof(NaubDevice*));
		naub->devices[naub->n_devices - 1] = device;
	}
}

NAUB_API
NaubWorld*
naub_world_new(void*         handle,
               NaubEventFunc event_cb)
{
	NaubWorld* naub = (NaubWorld*)malloc(sizeof(NaubWorld));
	if (!naub) {
		return NULL;
	}

	naub->handle    = handle;
	naub->event_cb  = event_cb;
	naub->n_devices = 0;
	naub->devices   = NULL;

	if (libusb_init(&naub->context)) {
		fprintf(stderr, "Failed to initialise libusb\n");
		free(naub);
		return NULL;
	}

	libusb_set_debug(naub->context, 3);

	return naub;
}

NAUB_API
NaubStatus
naub_world_open_all(NaubWorld* naub)
{
	libusb_device** devs   = NULL;
	ssize_t         n_devs = libusb_get_device_list(naub->context, &devs);
	for (ssize_t i = 0; i < n_devs; ++i) {
		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor(devs[i], &desc);
		fprintf(stderr, "DEVICE: %X %X\n", desc.idVendor, desc.idProduct);
		libusb_device_handle* handle = NULL;
		if (desc.idVendor == NOVATION_VENDOR_ID) {
			if (desc.idProduct == NOCTURN_PRODUCT_ID) {
				if (!libusb_open(devs[i], &handle)) {
					fprintf(stderr, "FOUND NOCTURN!\n");
					add_device(naub, nocturn_open(naub, handle));
				} else {
					fprintf(stderr, "Failed to open Nocturn device\n");
				}
			} else if (desc.idProduct == LAUNCHPAD_PRODUCT_ID) {
				if (!libusb_open(devs[i], &handle)) {
					fprintf(stderr, "FOUND LAUNCHPAD!\n");
					add_device(naub, launchpad_open(naub, handle));
				} else {
					fprintf(stderr, "Failed to open Launchpad device\n");
				}
			}
		}
	}
	libusb_free_device_list(devs, 1);

	fprintf(stderr, "Opened %d USB devices\n", naub->n_devices);
	return NAUB_SUCCESS;
}

NAUB_API
NaubStatus
naub_world_open(NaubWorld* naub,
                uint16_t   vendor_id,
                uint16_t   product_id)
{
	if (vendor_id != NAUB_VENDOR_NOVATION ||
	    (product_id != NAUB_PRODUCT_NOCTURN &&
	     product_id != NAUB_PRODUCT_LAUNCHPAD)) {
		return NAUB_ERR_UNSUPPORTED;
	}

	libusb_device_handle* handle = libusb_open_device_with_vid_pid(
		naub->context, vendor_id, product_id);
	if (!handle) {
		return NAUB_ERR_NO_DEVICE;
	}

	if (product_id == NAUB_PRODUCT_NOCTURN) {
		add_device(naub, nocturn_open(naub, handle));
	} else {
		add_device(naub, launchpad_open(naub, handle));
	}

	return NAUB_SUCCESS;
}

static void
close_device(NaubDevice* device)
{
	libusb_cancel_transfer(device->read_transfer);
	while (device->read_transfer->status != LIBUSB_TRANSFER_CANCELLED) {
		libusb_handle_events(device->naub->context);
	}
	libusb_reset_device(device->handle);
	libusb_free_transfer(device->read_transfer);
	libusb_release_interface(device->handle, 0);
	libusb_close(device->handle);
	free(device->read_data);
	free(device);
}

NAUB_API
void
naub_world_free(NaubWorld* naub)
{
	fprintf(stderr, "Cleaning up...\n");

	for (unsigned i = 0; i < naub->n_devices; ++i) {
		close_device(naub->devices[i]);
	}
	libusb_exit(naub->context);

	free(naub);
}

NAUB_API
unsigned
naub_world_num_devices(NaubWorld* naub)
{
	return naub->n_devices;
}

NAUB_API
NaubStatus
naub_handle_events(NaubWorld* naub)
{
	libusb_handle_events(naub->context);
	return NAUB_SUCCESS;
}

NAUB_API
NaubStatus
naub_handle_events_timeout(NaubWorld* naub, unsigned ms)
{
	struct timeval wait_time = { ms / 1000, (ms % 1000) * 1000 };
	libusb_handle_events_timeout(naub->context, &wait_time);
	return NAUB_SUCCESS;
}

static inline uint8_t
fbyte(float f)
{
	if (f < 0.0f) {
		return 0;
	} else if (f > 1.0f) {
		return 255;
	}
	return f * 255;
}

NAUB_API
int32_t
naub_rgb(float r, float g, float b)
{
	return (fbyte(r) << 16) + (fbyte(g) << 8) + fbyte(b);
}
