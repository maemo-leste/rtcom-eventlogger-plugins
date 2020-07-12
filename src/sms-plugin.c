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
#include <unistd.h>
#include <string.h>
#include <glib/gi18n.h>

#include <rtcom-eventlogger/eventlogger-plugin.h>
#include <rtcom-eventlogger/eventlogger.h>

#include "sms.h"

#define PLUGIN_NAME  "SMS"
#define PLUGIN_DESC  "SMSs' plugin"
#define SERVICE_NAME "RTCOM_EL_SERVICE_SMS"
#define SERVICE_DESC "Service for logging of SMSs."

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
        {"RTCOM_EL_EVENTTYPE_SMS_MESSAGE", "SMS message"},
        {NULL, NULL}
    };

    GList * l = NULL;
    guint i;

    for(i = 0; types[i].name != NULL; i++)
    {
        l = g_list_prepend(l, rtcom_el_eventtype_new(
                    types[i].name, types[i].name));
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
        {"RTCOM_EL_FLAG_SMS_PENDING", RTCOM_EL_FLAG_SMS_PENDING,
        "The SMS is in pending state"},

        {"RTCOM_EL_FLAG_SMS_TEMPORARY_ERROR", RTCOM_EL_FLAG_SMS_TEMPORARY_ERROR,
        "The SMS has an error and sending should be retried"},

        {"RTCOM_EL_FLAG_SMS_PERMANENT_ERROR", RTCOM_EL_FLAG_SMS_PERMANENT_ERROR,
        "The SMS has an unrecoverable error"},

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
        GValue local_uid = {0}, group_uid = {0}, out_gval = {0};
        gint total_events = 0;
        gint unread_events = 0;
        gint group_flags = 0;
        gboolean outgoing = FALSE;

        RTComEl * el;
        RTComElQuery * query;
        RTComElQueryGroupBy group_by;

        g_value_init(value, G_TYPE_STRING);

        g_object_get(it, "el", &el, "query", &query, NULL);
        g_object_get(query, "group-by", &group_by, NULL);

        rtcom_el_iter_get_raw(it, "local-uid", &local_uid);
        rtcom_el_iter_get_raw(it, "outgoing", &out_gval);
        outgoing = g_value_get_boolean(&out_gval);
        g_value_unset(&out_gval);

        if(group_by == RTCOM_EL_QUERY_GROUP_BY_GROUP)
        {
            rtcom_el_iter_get_raw(it, "group-uid", &group_uid);
            g_debug("group-uid = %s", g_value_get_string(&group_uid));
            if (g_value_get_string(&group_uid) != NULL)
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

            /**
             * Priority for SMS icons:
             * 1. Error
             * 2. Pending
             * 3. New, All read, Answered.
             *
             * The ones in #3 have the same priority because they are mutually
             * exclusive.
             */

            if(group_flags & (RTCOM_EL_FLAG_SMS_PERMANENT_ERROR |
                  RTCOM_EL_FLAG_SMS_TEMPORARY_ERROR))
                g_value_set_string(value, "chat_failed_sms");
            else if(group_flags & RTCOM_EL_FLAG_SMS_PENDING)
                g_value_set_string(value, "chat_pending_sms");
            else if(unread_events)
                g_value_set_string(value, "chat_unread_sms");
            else if(outgoing)
                g_value_set_string(value, "chat_replied_sms");
            else
                g_value_set_string(value, "general_sms");
        }
        else
        {
            GValue flags = {0};
            GValue is_read = {0};

            rtcom_el_iter_get_raw(it, "flags", &flags);
            rtcom_el_iter_get_raw(it, "is-read", &is_read);

            if(g_value_get_int(&flags) & RTCOM_EL_FLAG_SMS_PERMANENT_ERROR)
                g_value_set_string(value, "chat_failed_sms");
            else if(g_value_get_int(&flags) & RTCOM_EL_FLAG_SMS_PENDING)
                g_value_set_string(value, "chat_pending_sms");
            else if (g_value_get_boolean(&is_read) == FALSE)
                g_value_set_string(value, "chat_unread_sms");
            else if(outgoing)
                g_value_set_string(value, "chat_replied_sms");
            else
                g_value_set_string(value, "general_sms");

            g_value_unset(&flags);
            g_value_unset(&is_read);
        }

        g_value_unset(&local_uid);

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
        {
            RTComElAttachIter *att_it = rtcom_el_iter_get_attachments(it);

            if (att_it)
            {
                g_object_unref(att_it);
                txt = dgettext("rtcom-messaging-ui", "messaging_fi_link_business_card");
            }
        }

        if (txt == NULL)
            txt = "";

        g_value_set_string(value, txt);
        g_value_unset(&free_text);
        retval = TRUE;
    }
    else if (!strcmp(item, "group-title"))
    {
        /* SMSes don't have group-title */
        g_value_init(value, G_TYPE_STRING);
        g_value_set_static_string(value, NULL);
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

