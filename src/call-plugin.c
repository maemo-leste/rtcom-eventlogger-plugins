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
#include <glib/gi18n.h>
#include <stdio.h>
#include <sqlite3.h>
#include <string.h>

#include <rtcom-eventlogger/eventlogger-plugin.h>

#include "call.h"

#define PLUGIN_NAME  "CALL"
#define PLUGIN_DESC  "Calls plugin"
#define SERVICE_NAME "RTCOM_EL_SERVICE_CALL"
#define SERVICE_DESC "Service for logging of calls."

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
    sqlite3_stmt * stmt;
    const gchar * sql;
    int ret;
    gchar * err;

    g_debug("%s: initializing plugin %s", G_STRLOC, PLUGIN_NAME);

    db = (sqlite3 *) data;
    if (db == NULL)
    {
        g_warning("%s: db not open, not initialising call plugin tables",
            G_STRFUNC);
        return FALSE;
    }

    sql = "CREATE TABLE IF NOT EXISTS call_duration ("
          "inbound_gsm INTEGER CHECK(inbound_gsm >= 0) DEFAULT 0, "
          "outbound_gsm INTEGER CHECK(outbound_gsm >= 0) DEFAULT 0, "
          "inbound_voip INTEGER CHECK(inbound_voip >= 0) DEFAULT 0, "
          "outbound_voip INTEGER CHECK(outbound_voip >= 0) DEFAULT 0);";
    ret = sqlite3_exec(db, sql, NULL, NULL, &err);
    if(ret != SQLITE_OK)
    {
        g_warning("SQL error: %s", err);
        sqlite3_free(err);
        return FALSE;
    }

    sql = "SELECT * FROM call_duration;";
    ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if(ret != SQLITE_OK)
    {
        g_warning("Unable to prepare: %s", sql);
        return FALSE;
    }

    ret = sqlite3_step(stmt);
    if(ret != SQLITE_ROW && ret != SQLITE_DONE)
    {
        g_warning("Unable to step statement for SQL: %s", sql);
        sqlite3_finalize(stmt);
        return FALSE;
    }
    sqlite3_finalize(stmt);

    if(ret == SQLITE_DONE)
    {
        /* If we got SQLITE_DONE from stepping once, it means that there
         * were no rows. */
        sql = "INSERT INTO call_duration VALUES(0, 0, 0, 0);";
        ret = sqlite3_exec(db, sql, NULL, NULL, &err);
        if(ret != SQLITE_OK)
        {
            g_warning("Unable to insert initial duration values: %s", err);
            sqlite3_free(err);
            return FALSE;
        }
    }

    return TRUE;
}

static gint count_missed_calls(
        RTComElIter *it,
        const gchar *local_uid,
        const gchar *remote_uid)
{
    RTComEl *el;
    sqlite3 *db;
    sqlite3_stmt *stmt;
    char *sql;
    int ret;
    gint count = 0;

    g_return_val_if_fail (local_uid && local_uid[0], 0);

    if ((remote_uid == NULL) || (remote_uid[0] == '\0'))
        return 0;

    g_object_get(it, "el", &el, NULL);
    g_object_get(el, "db", &db, NULL);

    /* FIXME: This is ugly, but less evil than doing call-specific logic in
     * eventlogger core. The subquery finds the newest non-missed call
     * from the contact, and then counts number of missed calls that happened
     * after it. */

    sql = sqlite3_mprintf("SELECT COUNT(*) FROM Events " "JOIN EventTypes ON event_type_id = EventTypes.id "
        "WHERE EventTypes.name = 'RTCOM_EL_EVENTTYPE_CALL_MISSED' "
        "AND remote_uid = %Q AND local_uid = %Q AND Events.id > ("
            "SELECT CASE MAX(events.id) IS NOT NULL WHEN 1 THEN MAX(Events.id) ELSE 0 END FROM Events "
              "JOIN EventTypes ON event_type_id = EventTypes.id "
              "JOIN Services ON service_id = Services.id "
              "WHERE Services.name = 'RTCOM_EL_SERVICE_CALL' "
              "AND remote_uid = %Q AND local_uid = %Q AND "
              "EventTypes.name != 'RTCOM_EL_EVENTTYPE_CALL_MISSED');",
        remote_uid, local_uid, remote_uid, local_uid);

    if (SQLITE_OK != sqlite3_prepare_v2(db, sql, -1, &stmt, NULL))
    {
        g_warning("%s: can't compile SQL: %s", G_STRFUNC, sql);
        sqlite3_free(sql);
        return 0;
    }

    while (SQLITE_BUSY == (ret = sqlite3_step(stmt)));

    if (ret == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }
    else
    {
        g_warning("%s: error while executing SQL", G_STRFUNC);
    }

    sqlite3_finalize(stmt);
    sqlite3_free(sql);

    return count;
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
        {"RTCOM_EL_EVENTTYPE_CALL", "Call"},
        {"RTCOM_EL_EVENTTYPE_CALL_MISSED", "Missed call"},
        {"RTCOM_EL_EVENTTYPE_CALL_VOICEMAIL", "Voicemail message"},
        {NULL, NULL}
    };

    GList * l = NULL;
    guint i;

    for(i = 0; types[i].name != NULL; i++)
    {
        l = g_list_prepend(l, rtcom_el_eventtype_new(
                    types[i].name, types[i].desc));
    }

    return g_list_reverse(l);
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
        GValue tmp = { 0 };
        gboolean outgoing;

        g_value_init(value, G_TYPE_STRING);

        rtcom_el_iter_get_raw(it, "outgoing", &tmp);
        outgoing = g_value_get_boolean(&tmp);
        g_value_unset(&tmp);

        if (strcmp (type, "RTCOM_EL_EVENTTYPE_CALL_VOICEMAIL") == 0)
            g_value_set_string(value, "general_voicemail_avatar");
        else if (outgoing)
            g_value_set_string(value, "general_sent");
        else if (strcmp (type, "RTCOM_EL_EVENTTYPE_CALL") == 0)
            g_value_set_string(value, "general_received");
        else if (strcmp (type, "RTCOM_EL_EVENTTYPE_CALL_MISSED") == 0)
            g_value_set_string(value, "general_missed");
        else
            g_value_set_string(value, "general_call");

        retval = TRUE;
    }
    else if(!strcmp(item, "vcard-field"))
    {
        gchar * vcard_field = NULL;
        vcard_field = rtcom_el_iter_get_header_raw(
                it,
                item);

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
        gint missed_calls = 0;

        /* Counting number of missed calls is 2 SQL queries per item,
         * so we're doing it only when we know the last call was missed */
        if (strcmp (type, "RTCOM_EL_EVENTTYPE_CALL_MISSED") == 0)
        {
            RTComElQuery * query;
            RTComElQueryGroupBy group_by;

            g_object_get(it, "query", &query, NULL);
            g_object_get(query, "group-by", &group_by, NULL);

            if (group_by == RTCOM_EL_QUERY_GROUP_BY_NONE)
            {
                missed_calls = 1;
            }
            else
            {
                GValue luid = {0};
                GValue ruid = {0};

                rtcom_el_iter_get_raw(it, "local-uid", &luid);
                rtcom_el_iter_get_raw(it, "remote-uid", &ruid);
                missed_calls = count_missed_calls (it,
                    g_value_get_string(&luid), g_value_get_string(&ruid));
                g_value_unset(&luid);
                g_value_unset(&ruid);
            }
        }

        g_value_init(value, G_TYPE_INT);
        g_value_set_int(value, missed_calls);
        retval = TRUE;
    }
    else if(!strcmp(item, "content"))
    {
        /* Calls don't have text contents to show in the log */
        g_value_init(value, G_TYPE_STRING);
        g_value_set_static_string(value, "");

        retval = TRUE;
    }
    else if (!strcmp(item, "group-title"))
    {
        /* Calls don't have group-title */
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

