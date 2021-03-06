/* Launchpad controller class.
 * Copyright 2010-2011 David Robillard <http://drobilla.net>
 *
 * Based on liblaunchpad,
 * Copyright 2010 Christian Loehnert <krampenschiesser@freenet.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "LaunchpadImpl.hpp"

#include <stdint.h>
#include <stdlib.h>

#include <iostream>

#define VENDOR_ID  0x1235
#define PRODUCT_ID 0x000e

using std::cerr;
using std::cout;
using std::endl;

static bool
hasError(libusb_transfer* transfer)
{
	switch (transfer->status) {
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_NO_DEVICE:
		return true;
	default:
		return false;
	}
}

void
LaunchpadImpl::callback(libusb_transfer* transfer)
{
	LaunchpadImpl* pad    = reinterpret_cast<LaunchpadImpl*>(transfer->user_data);
	bool           isRead = (pad->readTransfer_ == transfer);

	if (transfer->status == LIBUSB_TRANSFER_CANCELLED) {
		//ok?
	} else if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
		if (isRead) {
			pad->handleReadTransfer(transfer);
		} else {
			pad->handleWriteTransfer(transfer);
		}
		libusb_submit_transfer(transfer);
	} else if (hasError(transfer)) {
		cerr << "got error " << transfer->status << endl;
		pad->shallDisconnect_ = true;
		return;
	} else if (transfer->status == LIBUSB_TRANSFER_TIMED_OUT) {
		libusb_submit_transfer(transfer);
		// FIXME: retry?
	} else {
		cerr << "unkown error " << transfer->status << " occured in readcallback"
		     << endl;
	}
}

void
LaunchpadImpl::handleMidi(const MidiEvent& data)
{
	if (data.at(0) == LIVE) {
		const int pos = data.at(1) - 104;
		const ButtonID id(MARGIN_H, 0, pos);
		signal_(id, data.at(2) == 127);
	} else if (data.at(1) / 8 % 2 == 1) {
		const int pos = data.at(1) / 16;
		const ButtonID id(MARGIN_V, pos, 0);
		signal_(id, data.at(2) == 127);
	} else {
		const int row    = data.at(1) / 16;
		const int column = data.at(1) % 16;
		const ButtonID id(GRID, row, column);
		signal_(id, data.at(2) == 127);
	}
}

void
LaunchpadImpl::handleWriteTransfer(libusb_transfer* transfer)
{
	g_mutex_lock(writeMutex_);
	if (!writeData_.empty()) {
		unsigned int pos = 0;
		while (pos < writeData_.size() && pos < 8) {
			uint8_t status       = writeData_.front();
			bool    statusChange = status != lastMidiStatus_;

			if (statusChange && (pos > 5)) {
				break;
			} else if (!statusChange && (pos > 6)) {
				break;
			}

			if (statusChange) {
				transfer->buffer[pos] = status;
				++pos;
				lastMidiStatus_ = status;
			}
			writeData_.pop();

			for (int data = 0; data < 2; ++data, ++pos) {
				transfer->buffer[pos] = writeData_.front();
				writeData_.pop();
			}
		}
		for (int i = pos; i < 8; i++) {
			transfer->buffer[i] = 145; // fill rest of message with junk
		}
		lastMidiStatus_ = 0;
	} else {
		for (int i = 0; i < 8; i++) {
			transfer->buffer[i] = 145; // fill rest of message with junk
		}
	}
	g_mutex_unlock(writeMutex_);
}

void
LaunchpadImpl::handleReadTransfer(libusb_transfer* transfer)
{
	for (int i = 0; i < transfer->actual_length; i++) {
		if (transfer->buffer[i] == LIVE) {
			isMatrixMidiData_ = false;
		} else if (transfer->buffer[i] == MATRIX) {
			isMatrixMidiData_ = true;
		} else {
			if (isMatrixMidiData_) {
				readData_.push_back(MATRIX);
			} else {
				readData_.push_back(LIVE);
			}
			readData_.push_back(transfer->buffer[i]);
			++i;
			readData_.push_back(transfer->buffer[i]);
			handleMidi(readData_);
			readData_.clear();
		}
	}
}

LaunchpadImpl::LaunchpadImpl()
	: maxPacketSize_(8)
	, lastMidiPos_(-1)
	, lastMidiStatus_(0)
	, isMatrixMidiData_(false)
	, isConnected_(false)
	, shallDisconnect_(false)
	, quit_(false)
{
	reconnectWait_ = 1;
	thread_        = g_thread_create(&LaunchpadImpl::static_run, this, TRUE, NULL);
	writeMutex_    = g_mutex_new();
}

LaunchpadImpl::~LaunchpadImpl()
{
	shallDisconnect_ = true;
	quit_            = true;
	g_thread_join(thread_);
	g_mutex_free(writeMutex_);
}

void
LaunchpadImpl::connect()
{
	#define USB_OUT (2 | LIBUSB_ENDPOINT_OUT)
	#define USB_IN  (1 | LIBUSB_ENDPOINT_IN)
	if (!isConnected_ && (difftime(time(NULL), reconnectWait_) > 1)) {
		if (libusb_init(NULL) != 0) {
			cerr << "Could not load libusb." << endl;
		}

		libusb_set_debug(0, 3);
		handle_ = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
		if (handle_ == NULL) {
			libusb_exit(0);
			return;
		}

		if (libusb_kernel_driver_active(handle_, 0)) {
			int ret = libusb_detach_kernel_driver(handle_, 0);
			if (!ret) {
				cerr << "Detached existing kernel driver" << endl;
			} else {
				cerr << "Error detaching existing kernel driver" << endl;
				libusb_close(handle_);
				libusb_exit(0);
				return;
			}
		}

		if (libusb_claim_interface(handle_, 0)) {
			cerr << "Failed to claim interface" << endl;
			libusb_close(handle_);
			libusb_exit(0);
			return;
		}

		if (libusb_reset_device(handle_)) {
			cerr << "Failed to reset device" << endl;
			libusb_close(handle_);
			libusb_exit(0);
			return;
		}

		readTransfer_  = libusb_alloc_transfer(0);
		writeTransfer_ = libusb_alloc_transfer(0);

		libusb_fill_interrupt_transfer(
			readTransfer_, handle_, USB_IN,
			new uint8_t[maxPacketSize_], maxPacketSize_,
			callback, this, 200);

		uint8_t* writeData = new uint8_t[maxPacketSize_];
		libusb_fill_interrupt_transfer(
			writeTransfer_, handle_, USB_OUT,
			writeData, maxPacketSize_,
			callback, this, 200);

		libusb_submit_transfer(readTransfer_);
		libusb_submit_transfer(writeTransfer_);

		cout << "successfully connected to the launchpad" << endl;
		isConnected_ = true;
	}
}

bool
shouldCancel(libusb_transfer* transfer)
{
	switch (transfer->status) {
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_COMPLETED:
	case LIBUSB_TRANSFER_NO_DEVICE:
		return false;
	default:
		return true;
	}
}

void
LaunchpadImpl::disconnect()
{
	if (isConnected_) {
		if (shouldCancel(readTransfer_)) {
			libusb_cancel_transfer(readTransfer_);
		}
		if (shouldCancel(writeTransfer_)) {
			libusb_cancel_transfer(writeTransfer_);
		}

		libusb_free_transfer(readTransfer_);
		libusb_free_transfer(writeTransfer_);
		libusb_release_interface(handle_, 0);
		libusb_close(handle_);
		libusb_exit(0);

		reconnectWait_   = time(NULL);
		isConnected_     = false;
		shallDisconnect_ = false;

		cout << "disconnected from launchpad" << endl;
	}
}

void
LaunchpadImpl::run()
{
	while (!quit_) {
		if (!isConnected_) {
			connect();
			if (!isConnected_) {
				usleep(10000);
			}
		} else {
			// FIXME: This doesn't block for some reason
			libusb_handle_events(0);
		}
		if (shallDisconnect_ && isConnected_) {
			disconnect();
		}
	}
}

void
LaunchpadImpl::writeMidi(uint8_t b1, uint8_t b2, uint8_t b3)
{
	g_mutex_lock(writeMutex_);
	writeData_.push(b1);
	writeData_.push(b2);
	writeData_.push(b3);
	g_mutex_unlock(writeMutex_);
}

void
LaunchpadImpl::setButton(ButtonID id, uint8_t velocity)
{
	const int row = id.row;
	const int col = id.col;
	switch (id.group) {
	case GRID:
		if (row >= 0 && row <= 7 && col >= 0 && col <= 7) {
			writeMidi(144, row * 16 + col, velocity);
		}
		break;
	case MARGIN_H:
		if (col >= 0 && col <= 7) {
			writeMidi(176, 104 + col, velocity);
		}
		break;
	case MARGIN_V:
		if (row >= 0 && row <= 7) {
			writeMidi(144, row * 16 + 8, velocity);
		}
		break;
	}
}

