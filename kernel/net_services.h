/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Telnetとリモートデバイス制御サービスの内部インターフェース． */

#pragma once
int net_service_telnet_init(void);
void net_service_telnet_poll(void);
int net_service_remote_devctl_init(void);
void net_service_remote_devctl_poll(void);
