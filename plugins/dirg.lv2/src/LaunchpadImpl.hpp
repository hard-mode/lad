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

#ifndef LAUNCHPADIMPL_HPP_
#define LAUNCHPADIMPL_HPP_

#include <vector>
#include <queue>

#include <glib.h>
#include <libusb.h>
#include <sigc++/sigc++.h>

#include "dirg_internal.hpp"

typedef std::vector<uint8_t> MidiEvent;

/** Actual launchpad implementation.
 * Only one of these may exist at once.
 */
class LaunchpadImpl
{
public:
	LaunchpadImpl();
	~LaunchpadImpl();

	bool isConnected() const { return isConnected_; }

	sigc::signal<void, ButtonID, bool>& getSignal() { return signal_; }

	void setButton(ButtonID id, uint8_t velocity);

private:
	void connect();
	void disconnect();

	void run();

	static void* static_run(void* me) { ((LaunchpadImpl*)me)->run(); return 0; }

	void writeMidi(uint8_t b1, uint8_t b2, uint8_t b3);

	void handleReadTransfer(libusb_transfer* transfer);
	void handleWriteTransfer(libusb_transfer* transfer);
	void handleMidi(const MidiEvent& data);

	sigc::signal<void, ButtonID, bool> signal_;

	typedef std::pair<uint8_t, uint8_t> Key;

	libusb_device_handle* handle_;
	libusb_transfer*      readTransfer_;
	libusb_transfer*      writeTransfer_;
	int                   maxPacketSize_;

	time_t reconnectWait_;

	GThread* thread_;

	std::vector<uint8_t> readData_;
	std::queue<uint8_t>  writeData_;
	GMutex*              writeMutex_;
	int                  lastMidiPos_;
	uint8_t              lastMidiStatus_;

	bool isMatrixMidiData_;

	volatile bool isConnected_;
	volatile bool shallDisconnect_;
	volatile bool quit_;

private:
	LaunchpadImpl(const LaunchpadImpl&); // Undefined
	LaunchpadImpl& operator=(const LaunchpadImpl&); // Undefined

	static void callback(libusb_transfer* transfer);

	static const int LIVE   = 176;
	static const int MATRIX = 144;
};

#endif /* LAUNCHPADIMPL_HPP_ */
