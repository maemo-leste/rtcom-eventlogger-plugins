/**
 * Copyright (C) 2008 Nokia Corporation.
 * Contact: Salvatore Iovene <ext-salvatore.iovene@nokia.com>
 * Contact: Naba Kumar <naba.kumar@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef RTCOM_EVENTLOGGER_PLUGINS_SMS_H
#define RTCOM_EVENTLOGGER_PLUGINS_SMS_H

#include <glib.h>
#include <rtcom-eventlogger/eventlogger.h>

enum
{
    RTCOM_EL_FLAG_SMS_PENDING         = 1 << 0,
    RTCOM_EL_FLAG_SMS_TEMPORARY_ERROR = 1 << 1,
    RTCOM_EL_FLAG_SMS_PERMANENT_ERROR = 1 << 2
};

/**
 * Returns an event by message-token.
 * @param el The RTComEl object
 * @param token The message-token
 * @param ev A pointer to a (staticly) allocated RTComElEvent
 * @return TRUE in case of success, FALSE otherwise
 */
G_GNUC_DEPRECATED gboolean
rtcom_el_plugin_sms_get_event_by_message_token(
        RTComEl * el,
        const gchar * token,
        RTComElEvent * ev);

#endif

/* vim: set ai et tw=75 ts=4 sw=4: */

