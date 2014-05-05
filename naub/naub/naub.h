/*
  Copyright 2011-2012 David Robillard <http://drobilla.net>

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

/**
   @file naub.h API for Naub, a simple C API for various USB controllers.
*/

#ifndef NAUB_NAUB_H
#define NAUB_NAUB_H

#include <stdint.h>

#ifdef NAUB_SHARED
#    ifdef _WIN32
#        define NAUB_LIB_IMPORT __declspec(dllimport)
#        define NAUB_LIB_EXPORT __declspec(dllexport)
#    else
#        define NAUB_LIB_IMPORT __attribute__((visibility("default")))
#        define NAUB_LIB_EXPORT __attribute__((visibility("default")))
#    endif
#    ifdef NAUB_INTERNAL
#        define NAUB_API NAUB_LIB_EXPORT
#    else
#        define NAUB_API NAUB_LIB_IMPORT
#    endif
#else
#    define NAUB_API
#endif

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

/**
   @defgroup naub Naub
   A simple C API for various USB controllers.
*/

typedef struct NaubWorldImpl NaubWorld;

/**
   Return status code.
*/
typedef enum {
	NAUB_SUCCESS,          /**< No error */
	NAUB_ERR_UNSUPPORTED,  /**< Requested operation is not supported */
	NAUB_ERR_NO_DEVICE,    /**< Device not found */
	NAUB_ERR_BAD_DATA,     /**< Invalid data received from device */
	NAUB_ERR_BAD_CONTROL   /**< No control that supports operation */
} NaubStatus;

/**
   Mode for an LED value indicator (e.g. an LED ring around a knob).
*/
typedef enum {
	NAUB_LED_FROM_MIN,        /**< Light range from 0 to value */
	NAUB_LED_FROM_MAX,        /**< Light range from value to max */
	NAUB_LED_FROM_MID,        /**< Light range from mid to value */
	NAUB_LED_MID_RANGE,       /**< Light range centered about middle */
	NAUB_LED_SINGLE,          /**< Light one LED at value */
	NAUB_LED_SINGLE_INVERTED  /**< Light all LEDs except one at value */
} NaubLEDMode;

/**
   Identifier for a control (e.g. knob or button) on a device.
*/
typedef struct {
	uint32_t device;  /**< Device number */
	uint32_t group;   /**< Group number on device */
	uint32_t x;       /**< X position in group (0 based) */
	uint32_t y;       /**< Y position in group (0 based) */
} NaubControlID;

/**
   Event type.
*/
typedef enum {
	NAUB_EVENT_TOUCH,     /**< Control touched */
	NAUB_EVENT_BUTTON,    /**< Button pressed */
	NAUB_EVENT_SET,       /**< Control value set (absolute) */
	NAUB_EVENT_INCREMENT  /**< Control value incremented (relative) */
} NaubEventType;

/**
   Control touch event (type NAUB_EVENT_TOUCH).
*/
typedef struct {
	NaubEventType type;     /**< Event type */
	NaubControlID control;  /**< Control ID */
	bool          touched;  /**< True iff control is touched */
} NaubEventTouch;

/**
   Button press or release event (type NAUB_EVENT_BUTTON).
*/
typedef struct {
	NaubEventType type;     /**< Event type */
	NaubControlID control;  /**< Control ID */
	bool          pressed;  /**< True iff button is pressed */
} NaubEventButton;

/**
   Control value set event (type NAUB_EVENT_SET).
*/
typedef struct {
	NaubEventType type;     /**< Event type */
	NaubControlID control;  /**< Control ID */
	int32_t       value;    /**< Control value */
} NaubEventSet;

/**
   Control value increment/decrement event (type NAUB_EVENT_INCREMENT).
*/
typedef struct {
	NaubEventType type;     /**< Event type */
	NaubControlID control;  /**< Control ID */
	int32_t       delta;    /**< Control value delta */
} NaubEventIncrement;

/**
   Naub Event.

   All notifications from the library, including controller input, are
   described using this union.  The type field is always present, it is used
   to determine which union field is set for this event (this pattern should
   be familiar to those with X11 or Glib experience).
*/
typedef union {
	NaubEventType      type;       /**< Event type (always valid) */
	NaubEventTouch     touch;      /**< Body for NAUB_EVENT_TOUCH */
	NaubEventButton    button;     /**< Body for NAUB_EVENT_BUTTON */
	NaubEventSet       set;        /**< Body for NAUB_EVENT_SET */
	NaubEventIncrement increment;  /**< Body for NAUB_EVENT_INCREMENT */
} NaubEvent;

/**
   USB vendor IDs of supported devices.
*/
typedef enum {
	NAUB_VENDOR_NOVATION = 0x1235
} NaubVendorID;

/**
   USB product IDs of supported devices.
*/
typedef enum {
	NAUB_PRODUCT_NOCTURN   = 0x000A,
	NAUB_PRODUCT_LAUNCHPAD = 0x000E
} NaubProductorID;

/**
   Event callback function.
*/
typedef void (*NaubEventFunc)(void*            handle,
                              const NaubEvent* event);

NAUB_API
NaubWorld*
naub_world_new(void*         handle,
               NaubEventFunc event_cb);

NAUB_API
NaubStatus
naub_world_open_all(NaubWorld* naub);

NAUB_API
NaubStatus
naub_world_open(NaubWorld* naub,
                uint16_t   vendor_id,
                uint16_t   product_id);

NAUB_API
unsigned
naub_world_num_devices(NaubWorld* naub);

NAUB_API
void
naub_world_free(NaubWorld* naub);

NAUB_API
NaubStatus
naub_handle_events(NaubWorld* naub);

NAUB_API
NaubStatus
naub_handle_events_timeout(NaubWorld* naub, unsigned ms);

NAUB_API
NaubStatus
naub_flush(NaubWorld* naub);

NAUB_API
NaubStatus
naub_set_control(NaubWorld*    naub,
                 NaubControlID control,
                 int32_t       value);

NAUB_API
NaubStatus
naub_set_led_mode(NaubWorld*    naub,
                  NaubControlID control,
                  NaubLEDMode   mode);

/**
   Return an RGB value for controls that display colours (usually buttons).

   @param r Red level [0..1]
   @param g Green level [0..1]
   @param b Blue level [0..1]
*/
NAUB_API
int32_t
naub_rgb(float r, float g, float b);

/**
   @}
*/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* NAUB_NAUB_H */
