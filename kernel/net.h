/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once
void net_init(void);
void net_poll(void);

/* TCP transport for TTY2; the kernel owns the remote shell lifecycle. */
int net_terminal_connected(void);
/* Changes on every accepted connection and disconnect. */
unsigned net_terminal_epoch(void);
int net_terminal_read(void *dst, unsigned size);
int net_terminal_write(const void *src, unsigned size);
