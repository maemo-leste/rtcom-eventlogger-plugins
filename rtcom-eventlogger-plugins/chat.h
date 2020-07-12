/**
 * Copyright (C) 2008 Nokia Corporation.
 * Contact: Tan Miaoqing <miaoqing.tan@nokia.com>
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

#ifndef RTCOM_EVENTLOGGER_PLUGINS_CHAT_H
#define RTCOM_EVENTLOGGER_PLUGINS_CHAT_H

#include <glib.h>
#include <rtcom-eventlogger/eventlogger.h>

enum
{
    RTCOM_EL_FLAG_CHAT_GROUP        = 1 << 0,
    RTCOM_EL_FLAG_CHAT_ROOM         = 1 << 1,
    RTCOM_EL_FLAG_CHAT_OPAQUE       = 1 << 2,
    RTCOM_EL_FLAG_CHAT_DELAYED      = 1 << 3,
    RTCOM_EL_FLAG_CHAT_OFFLINE      = 1 << 4,
};

/**
 * Logs a group title (e.g. topic) for a group chat.
 * @param el The RTComEl object
 * @param group_uid The group id of the chat to which we will
 * set the group title
 * @param title The new group title for this group chat
 * @param error A location for the error.
 */
gboolean
rtcom_el_plugin_chat_set_group_title(
        RTComEl * el,
        const gchar * group_uid,
        const gchar * title);

/**
 * Get the group title (e.g. topic) of a group chat.
 * @param el The RTComEl object
 * @param group_uid The group id of the chat from which we will
 * get the group title
 *
 * Return: the title string, or NULL if there is error.
 * Must be freed after use.
 */
gchar *
rtcom_el_plugin_chat_get_group_title(
        RTComEl * el,
        const gchar * group_uid);


/**
 * Set read state of multiple events, indexed by message-token.
 * @param el The RTComEl object
 * @param is_read Read state to set (TRUE = read, FALSE = unread)
 * @param message_tokens A NULL terminated array of strings containing
 * message tokens of events to be updated
 *
 * Return: TRUE if events were successfully updated, FALSE on error.
 */
gboolean
rtcom_el_plugin_chat_set_read_by_tokens (
        RTComEl * el,
        gboolean is_read,
        const gchar **message_tokens);

#endif /* RTCOM_EVENTLOGGER_PLUGINS_CHAT_H */
