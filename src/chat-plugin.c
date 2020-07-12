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

#include <glib.h>
#include <gmodule.h>
#include <sqlite3.h>

#include <string.h>

#include <rtcom-eventlogger/eventlogger-plugin.h>
#include <rtcom-eventlogger/eventlogger.h>

#include "chat.h"

#define PLUGIN_NAME  "CHAT"
#define PLUGIN_DESC  "CHATs' plugin"
#define SERVICE_NAME "RTCOM_EL_SERVICE_CHAT"
#define SERVICE_DESC "Service for logging of chat messages."

/* This is the max length of the name displayed in the event, because there
 * need to be enough space for the date and time. */
#define NAME_LENGTH "25"

/* Max length of text, in characters. */
#define TEXT_LENGTH 100

const gchar * g_module_check_init(
        GModule * module)
{
    g_message("Plugin registered: %s.", PLUGIN_NAME);
    return NULL; /* NULL means success */
}

gboolean rtcom_el_plugin_init(
        gpointer data)
{
    sqlite3 * db;
    const gchar * sql;
    int ret;
    gchar * err;

    g_debug("%s: initializing plugin %s", G_STRLOC, PLUGIN_NAME);

    db = (sqlite3 *) data;
    if (db == NULL)
    {
        g_warning("%s: db not open, not initialising chat plugin tables",
            G_STRFUNC);
        return FALSE;
    }

    sql = "CREATE TABLE IF NOT EXISTS chat_group_info ("
        "group_uid TEXT UNIQUE NOT NULL, "
        "group_title TEXT); "
        "CREATE INDEX IF NOT EXISTS idx_cgi_group_uid ON chat_group_info(group_uid);";
    ret = sqlite3_exec(db, sql, NULL, NULL, &err);
    if(ret != SQLITE_OK)
    {
        g_warning("SQL error: %s", err);
        sqlite3_free(err);
        return FALSE;
    }

    return TRUE;
}

const gchar * rtcom_el_plugin_name(void)
{
    return PLUGIN_NAME;
}

const gchar * rtcom_el_plugin_desc(void)
{
    return PLUGIN_DESC;
}

RTComElService * rtcom_el_plugin_service(void)
{
    RTComElService * service = rtcom_el_service_new(
            SERVICE_NAME,
            SERVICE_DESC);
    return service;
}

GList * rtcom_el_plugin_eventtypes(void)
{
    struct
    {
        gchar * name;
        gchar * desc;
    } types[] = {
        {"RTCOM_EL_EVENTTYPE_CHAT_MESSAGE", "Normal message"},
        {"RTCOM_EL_EVENTTYPE_CHAT_NOTICE", "Notice"},
        {"RTCOM_EL_EVENTTYPE_CHAT_ACTION", "Action message"},
        {"RTCOM_EL_EVENTTYPE_CHAT_AUTOREPLY", "Autoreply message"},
        {"RTCOM_EL_EVENTTYPE_CHAT_JOIN", "Groupchat join"},
        {"RTCOM_EL_EVENTTYPE_CHAT_LEAVE", "Groupchat leave"},
        {"RTCOM_EL_EVENTTYPE_CHAT_TOPIC", "Topic change"},
        {NULL, NULL}
    };

    GList * l = NULL;
    guint i;

    for(i = 0; types[i].name != NULL; ++i)
    {
        l = g_list_prepend(l, rtcom_el_eventtype_new(
                    types[i].name, types[i].desc));
    }

    return g_list_reverse(l);
}

GList * rtcom_el_plugin_flags(void)
{
    struct
    {
        gchar * name;
        gint    value;
        gchar * desc;
    } flags[] = {
        {"RTCOM_EL_FLAG_CHAT_GROUP", RTCOM_EL_FLAG_CHAT_GROUP,
            "Groupchat message (no valid channel_id)" },
        {"RTCOM_EL_FLAG_CHAT_ROOM", RTCOM_EL_FLAG_CHAT_ROOM,
            "MUC/Room message (with valid channel_id)"},
        {"RTCOM_EL_FLAG_CHAT_OPAQUE", RTCOM_EL_FLAG_CHAT_OPAQUE,
            "Channel identifier is opaque"},
        {"RTCOM_EL_FLAG_CHAT_OFFLINE", RTCOM_EL_FLAG_CHAT_OFFLINE,
            "Message was received while user was offline"},

        {NULL, -1, NULL}
    };

    GList * l = NULL;
    guint i;

    for(i = 0; flags[i].name != NULL; ++i)
    {
        l = g_list_prepend(l, rtcom_el_flag_new(
                    flags[i].name, flags[i].value, flags[i].desc));
    }

    return g_list_reverse(l);;
}

gboolean rtcom_el_plugin_get_value(
        RTComElIter * it,
        const gchar * item,
        GValue * value)
{
    GValue event_type = {0};
    const gchar * type;
    gboolean retval;

    g_return_val_if_fail(it, FALSE);
    g_return_val_if_fail(item, FALSE);
    g_return_val_if_fail(value, FALSE);

    rtcom_el_iter_get_raw(it, "event-type", &event_type);
    type = g_value_get_string(&event_type);

    if(!type)
    {
        g_debug("Corrupted db.");
        retval = FALSE;
    }
    else if(!strcmp(item, "additional-text"))
    {
        /* We don't care about this value, let's set it empty to
         * avoid glib warnings. */
        g_value_init(value, G_TYPE_STRING);
        g_value_set_string(value, "");
        retval = TRUE;
    }
    else if(!strcmp(item, "icon-name"))
    {
        GValue group_uid = {0}, out_gval = {0};
        gint total_events = 0;
        gint unread_events = 0;
        gint group_flags = 0;
        gboolean outgoing = FALSE;

        RTComEl * el;
        RTComElQuery * query;
        RTComElQueryGroupBy group_by;

        g_debug(G_STRLOC ": somebody requested the icon-path.");

        g_value_init(value, G_TYPE_STRING);

        g_object_get(it, "el", &el, "query", &query, NULL);
        g_object_get(query, "group-by", &group_by, NULL);

        rtcom_el_iter_get_raw(it, "outgoing", &out_gval);
        outgoing = g_value_get_boolean(&out_gval);
        g_value_unset(&out_gval);

        if(group_by == RTCOM_EL_QUERY_GROUP_BY_GROUP)
        {
            rtcom_el_iter_get_raw(it, "group-uid", &group_uid);
            g_debug("group-uid = %s", g_value_get_string(&group_uid));
            rtcom_el_get_group_info(
                    el,
                    g_value_get_string(&group_uid),
                    &total_events,
                    &unread_events,
                    &group_flags);

            g_debug(G_STRLOC ": %d events in the group.", total_events);
            g_debug(G_STRLOC ": %d unread events in the group.", unread_events);
            g_debug(G_STRLOC ": group flags: %d", group_flags);

            g_value_unset(&group_uid);

            if(unread_events)
                g_value_set_string(value, "chat_unread_chat");
            else if(outgoing)
                g_value_set_string(value, "chat_replied_chat");
            else
                g_value_set_string(value, "general_chat");
        }
        else
        {
            GValue is_read = {0};

            rtcom_el_iter_get_raw(it, "is-read", &is_read);

            if (g_value_get_boolean(&is_read) == FALSE)
                g_value_set_string(value, "chat_unread_chat");
            else if (outgoing)
                g_value_set_string(value, "chat_replied_chat");
            else
                g_value_set_string(value, "general_chat");

            g_value_unset(&is_read);
        }

        retval = TRUE;
    }
    else if(!strcmp(item, "vcard-field"))
    {
        gchar * vcard_field = rtcom_el_iter_get_header_raw(it, item);

        g_value_init(value, G_TYPE_STRING);
        if(vcard_field)
        {
            g_value_take_string(value, vcard_field);
        }
        else
        {
            g_debug("Plugin %s couldn't find item %s", PLUGIN_NAME, item);
            g_value_set_static_string(value, NULL);
        }
        retval = TRUE;
    }
    else if(!strcmp(item, "event-count"))
    {
        GValue group_uid = {0};
        gint total_events = 0;
        gint unread_events = 0;
        gint group_flags = 0;

        RTComEl * el;
        RTComElQuery * query;
        RTComElQueryGroupBy group_by;

        g_debug(G_STRLOC ": somebody requested the event-count");

        g_object_get(it, "el", &el, "query", &query, NULL);
        g_object_get(query, "group-by", &group_by, NULL);

        if(group_by == RTCOM_EL_QUERY_GROUP_BY_GROUP)
        {
            rtcom_el_iter_get_raw(it, "group-uid", &group_uid);
            g_debug("group-uid = %s", g_value_get_string(&group_uid));
            rtcom_el_get_group_info(
                    el,
                    g_value_get_string(&group_uid),
                    &total_events,
                    &unread_events,
                    &group_flags);

            g_debug(G_STRLOC ": %d events in the group.", total_events);
            g_debug(G_STRLOC ": %d unread events in the group.", unread_events);
            g_debug(G_STRLOC ": group flags: %d", group_flags);

            g_value_unset(&group_uid);
        }
        else
        {
            GValue is_read = {0};

            rtcom_el_iter_get_raw(it, "is-read", &is_read);
            unread_events = g_value_get_boolean(&is_read) ? 0 : 1;
            g_value_unset(&is_read);
        }

        g_value_init(value, G_TYPE_INT);
        g_value_set_int(value, unread_events);
        retval = TRUE;
    }
    else if(!strcmp(item, "content"))
    {
        const gchar *txt;
        GValue free_text = {0};

        g_value_init(value, G_TYPE_STRING);

        rtcom_el_iter_get_raw (it, "free-text", &free_text);
        txt = g_value_get_string (&free_text);

        if (txt == NULL)
            txt = "";

        g_value_set_string(value, txt);
        g_value_unset(&free_text);
        retval = TRUE;
    }
    else if (!strcmp(item, "group-title"))
    {
        GValue flags = {0};
        gint flags_val;

        rtcom_el_iter_get_raw(it, "flags", &flags);
        flags_val = g_value_get_int(&flags);
        g_value_unset(&flags);

        g_value_init(value, G_TYPE_STRING);

        if (flags_val & RTCOM_EL_FLAG_CHAT_GROUP)
        {
            GValue group_uid = {0};
            RTComEl *el;
            gchar *title;
            const gchar *group_uid_val;

            rtcom_el_iter_get_raw(it, "group-uid", &group_uid);
            group_uid_val = g_value_get_string(&group_uid);

            g_object_get(it, "el", &el, NULL);
            title = rtcom_el_plugin_chat_get_group_title(el, group_uid_val);

            g_value_unset(&group_uid);

            if(title)
            {
                g_value_set_string(value, title);
                g_free (title);
            }
            else
            {
                g_debug("Plugin %s couldn't find item %s", PLUGIN_NAME, item);
                g_value_set_static_string(value, NULL);
            }
        }
        else {
                g_value_set_static_string(value, NULL);
        }

        retval = TRUE;
    }
    else
    {
        retval = FALSE;
    }

    g_value_unset(&event_type);
    return retval;
}

/* vim: set ai et tw=75 ts=4 sw=4: */

