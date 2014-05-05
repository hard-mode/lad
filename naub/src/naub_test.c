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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "naub/naub.h"

struct {
	NaubWorld* naub;
	int        exit;
} app;

static void
interrupt(int signal)
{
	app.exit = 1;
}

static void
print_control(NaubControlID id)
{
	printf("Control %X:%u.%u.%u", id.device, id.group, id.x, id.y);
}

static void
event_cb(void*            handle,
         const NaubEvent* event)
{
	switch (event->type) {
	case NAUB_EVENT_TOUCH:
		print_control(event->touch.control);
		printf(" touch %d\n", (int)event->touch.touched);
		break;
	case NAUB_EVENT_BUTTON:
		print_control(event->button.control);
		printf(" button %d\n", event->button.pressed);
		naub_set_control(app.naub,
		                 event->button.control,
		                 event->button.pressed ? naub_rgb(1, 0, 0) : 0);
		break;
	case NAUB_EVENT_SET:
		print_control(event->set.control);
		printf(" set %d\n", event->set.value);
		break;
	case NAUB_EVENT_INCREMENT:
		print_control(event->increment.control);
		printf(" increment %d\n", event->increment.delta);
		break;
	default:
		printf("Unknown event\n");
		break;
	}

	naub_flush(app.naub);
}

int
main(int argc, char** argv)
{
	signal(SIGINT, interrupt);
	signal(SIGTERM, interrupt);

	app.exit = 0;
	app.naub = naub_world_new(NULL, event_cb);
	if (!app.naub) {
		fprintf(stderr, "Failed to open controller\n");
		return 1;
	}

	naub_world_open_all(app.naub);

	while (!app.exit && !naub_handle_events(app.naub)) {}

	naub_world_free(app.naub);
	return 0;
}
