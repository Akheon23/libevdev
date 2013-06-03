/*
 * Copyright © 2013 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifndef libevdev_INT_H
#define libevdev_INT_H

#include <config.h>
#include "libevdev.h"

#define LONG_BITS (sizeof(long) * 8)
#define NLONGS(x) (((x) + LONG_BITS - 1) / LONG_BITS)
#define ARRAY_LENGTH(a) (sizeof(a) / (sizeof((a)[0])))
#define MAX_NAME 256
#define MAX_SLOTS 32
#define ABS_MT_MIN ABS_MT_SLOT
#define ABS_MT_MAX ABS_MT_TOOL_Y
#define ABS_MT_CNT (ABS_MT_MAX - ABS_MT_MIN + 1)

#undef min
#undef max
#define min(a,b) \
		({ __typeof__ (a) _a = (a); \
	          __typeof__ (b) _b = (b); \
		_a > _b ? _b : _a; \
		})
#define max(a,b) \
		({ __typeof__ (a) _a = (a); \
	          __typeof__ (b) _b = (b); \
		_a > _b ? _a : _b; \
		})

struct libevdev {
	int fd;
	libevdev_log_func_t log;

	char *name;
	char *phys;
	char *uniq;
	struct input_id ids;
	int driver_version;
	unsigned long bits[NLONGS(EV_CNT)];
	unsigned long props[NLONGS(INPUT_PROP_CNT)];
	unsigned long key_bits[NLONGS(KEY_CNT)];
	unsigned long rel_bits[NLONGS(REL_CNT)];
	unsigned long abs_bits[NLONGS(ABS_CNT)];
	unsigned long led_bits[NLONGS(LED_CNT)];
	unsigned long key_values[NLONGS(KEY_CNT)];
	struct input_absinfo abs_info[ABS_CNT];
	unsigned int mt_slot_vals[MAX_SLOTS][ABS_MT_CNT];
	int num_slots; /**< valid slots in mt_slot_vals */
	int current_slot;

	int need_sync;
	int grabbed;

	struct input_event *queue;
	size_t queue_size; /**< size of queue in elements */
	size_t queue_next; /**< next event index */
	size_t queue_nsync; /**< number of sync events */
};

/**
 * @return a pointer to the next element in the queue, or NULL if the queue
 * is full.
 */
static inline struct input_event*
queue_push(struct libevdev *dev)
{
	if (dev->queue_next >= dev->queue_size)
		return NULL;

	return &dev->queue[dev->queue_next++];
}

/**
 * Set ev to the last element in the queue, removing it from the queue.
 *
 * @return 0 on success, 1 if the queue is empty.
 */
static inline int
queue_pop(struct libevdev *dev, struct input_event *ev)
{
	if (dev->queue_next == 0)
		return 1;

	*ev = dev->queue[--dev->queue_next];

	return 0;
}

static inline int
queue_peek(struct libevdev *dev, size_t idx, struct input_event *ev)
{
	if (idx > dev->queue_next)
		return 1;
	*ev = dev->queue[idx];
	return 0;
}


/**
 * Shift the first n elements into ev and return the number of elements
 * shifted.
 * ev must be large enough to store n elements.
 *
 * @param ev The buffer to copy into, or NULL
 * @return The number of elements in ev.
 */
static inline int
queue_shift_multiple(struct libevdev *dev, int n, struct input_event *ev)
{
	int i;

	if (dev->queue_next == 0)
		return 0;

	n = min(n, dev->queue_next);

	if (ev) {
		for (i = 0; i < n; i++)
			ev[i] = dev->queue[i];
	}

	for (i = 0; i < dev->queue_next - n; i++)
		dev->queue[i] = dev->queue[n + i];

	dev->queue_next -= n;
	return n;
}

/**
 * Set ev to the first element in the queue, shifting everything else
 * forward by one.
 *
 * @return 0 on success, 1 if the queue is empty.
 */
static inline int
queue_shift(struct libevdev *dev, struct input_event *ev)
{
	return queue_shift_multiple(dev, 1, ev) == 1 ? 0 : 1;
}

static inline int
queue_alloc(struct libevdev *dev, int size)
{
	dev->queue = calloc(size, sizeof(struct input_event));
	if (!dev->queue)
		return -ENOSPC;

	dev->queue_size = size;
	dev->queue_next = 0;
	return 0;
}

static inline void
queue_free(struct libevdev *dev)
{
	free(dev->queue);
	dev->queue_size = 0;
	dev->queue_next = 0;
}

static inline int
queue_num_elements(struct libevdev *dev)
{
	return dev->queue_next;
}

static inline int
queue_size(struct libevdev *dev)
{
	return dev->queue_size;
}

static inline int
queue_num_free_elements(struct libevdev *dev)
{
	return dev->queue_size - dev->queue_next - 1;
}

static inline struct input_event *
queue_next_element(struct libevdev *dev)
{
	return &dev->queue[dev->queue_next];
}

static inline int
queue_set_num_elements(struct libevdev *dev, int nelem)
{
	if (nelem > dev->queue_size)
		return 1;

	dev->queue_next = nelem;

	return 0;
}
#endif

