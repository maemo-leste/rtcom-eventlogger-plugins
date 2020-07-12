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

#include "call.h"

#include <rtcom-eventlogger/event.h>
#include <sqlite3.h>
#include <string.h>

static gint
_common_logging(
        RTComEl * el,
        RTComElEvent * ev,
        GError ** error)
{
    GQuark error_domain = g_quark_from_string("RTCOM_EL_PLUGIN_CALL_ERROR");

    if(!RTCOM_IS_EL(el))
    {
        g_set_error(
                error, error_domain, RTCOM_EL_INVALID_ARGUMENT_ERROR,
                "'el' must be a valid RTComEl.");
        return -1;
    }

    if(!ev)
    {
        g_set_error(
                error, error_domain, RTCOM_EL_INVALID_ARGUMENT_ERROR,
                "'ev' must be a valid RTComEvent");
        return -1;
    }

    RTCOM_EL_EVENT_SET_FIELD(ev, service, "RTCOM_EL_SERVICE_CALL");

    if(!RTCOM_EL_EVENT_IS_SET(ev, start_time))
        RTCOM_EL_EVENT_SET_FIELD(ev, start_time, time(0));

    return 0;
}

gint
rtcom_el_plugin_call_log_inbound(
        RTComEl * el,
        RTComElEvent * ev,
        GError ** error)
{
    gint event_id;

    if(_common_logging(el, ev, error) < 0)
    {
        g_assert(error == NULL || *error != NULL);
        return -1;
    }

    RTCOM_EL_EVENT_SET_FIELD(ev, event_type, "RTCOM_EL_EVENTTYPE_CALL");
    RTCOM_EL_EVENT_SET_FIELD(ev, outgoing, FALSE);

    event_id = rtcom_el_add_event(el, ev, error);
    if(event_id < 0)
    {
        g_assert(error == NULL || *error != NULL);
        return -1;
    }

    return event_id;
}

gboolean
rtcom_el_plugin_call_update_duration(
        RTComEl * el,
        guint event_id,
        gint duration,
        gboolean update_timers,
        RTComElPluginCallType type,
        GError ** error)
{
    RTComElQuery * query;
    RTComElIter * it;
    gchar * event_type = NULL;
    gboolean retval;

    GQuark error_domain = g_quark_from_string("RTCOM_EL_PLUGIN_CALL_ERROR");

    g_return_val_if_fail(RTCOM_IS_EL(el), FALSE);
    g_return_val_if_fail(event_id != 0, FALSE);

    if(duration == 0)
        /* nothing to do :) */
        return TRUE;

    if(rtcom_el_set_end_time(el, event_id, duration, error) == FALSE)
    {
        g_assert(error == NULL || *error != NULL);
        return FALSE;
    }

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
                query,
                "id", event_id, RTCOM_EL_OP_EQUAL,
                NULL))
    {
        g_set_error(
                error, error_domain, RTCOM_EL_INTERNAL_ERROR,
                "Unable to prepare query.");
        return FALSE;
    }

    it = rtcom_el_get_events(el, query);
    g_object_unref(query);
    if(it == NULL || rtcom_el_iter_first(it) == FALSE)
    {
        g_set_error(
                error, error_domain, RTCOM_EL_INTERNAL_ERROR,
                "Unable to obtain iterator.");
        return FALSE;
    }

    rtcom_el_iter_get_values(it, "event-type", &event_type, NULL);
    if(event_type == NULL)
    {
        g_set_error(
                error, error_domain, RTCOM_EL_INTERNAL_ERROR,
                "Unable to obtain event-type.");
        return FALSE;
    }

    retval = TRUE;

    if (!strcmp(event_type, "RTCOM_EL_EVENTTYPE_CALL"))
    {
        GValue tmp = { 0 };
        gboolean outgoing;

        outgoing = rtcom_el_iter_get_raw(it, "outgoing", &tmp);
        outgoing = g_value_get_boolean(&tmp);
        g_value_unset(&tmp);

        if (outgoing)
        {
            if(!rtcom_el_plugin_call_increase_outbound_meter(el, type, duration))
            {
                g_set_error(
                        error, error_domain, RTCOM_EL_INTERNAL_ERROR,
                        "Unable to increase outbound meter.");
                retval = FALSE;
            }
            else
            {
                g_debug("%s: inbound meter increased successfully.", G_STRLOC);
            }
        }
        else
        {
            if(!rtcom_el_plugin_call_increase_inbound_meter(el, type, duration))
            {
                g_set_error(
                        error, error_domain, RTCOM_EL_INTERNAL_ERROR,
                        "Unable to increase inbound meter.");
                retval = FALSE;
            }
            else
                g_debug("%s: inbound meter increased successfully.", G_STRLOC);
            }
    }

    g_object_unref(it);
    g_free(event_type);

    return retval;
}

gint
rtcom_el_plugin_call_log_outbound(
        RTComEl * el,
        RTComElEvent * ev,
        GError ** error)
{
    gint event_id;

    g_debug("%s: initial error = %p", G_STRFUNC, error);

    if(_common_logging(el, ev, error) < 0)
    {
        g_assert(error == NULL || *error != NULL);
        return -1;
    }

    g_debug("%s: error = %p", G_STRFUNC, error);
    RTCOM_EL_EVENT_SET_FIELD(ev, event_type, "RTCOM_EL_EVENTTYPE_CALL");
    RTCOM_EL_EVENT_SET_FIELD(ev, outgoing, TRUE);

    event_id = rtcom_el_add_event(el, ev, error);
    if(event_id < 0)
    {
        g_assert(error == NULL || *error != NULL);
        return -1;
    }

    return event_id;
}

gint
rtcom_el_plugin_call_log_missed(
        RTComEl * el,
        RTComElEvent * ev,
        GError ** error)
{
    gint event_id;

    if(_common_logging(el, ev, error) < 0)
    {
        g_assert(error == NULL || *error != NULL);
        return -1;
    }

    RTCOM_EL_EVENT_SET_FIELD(ev, event_type, "RTCOM_EL_EVENTTYPE_CALL_MISSED");

    event_id = rtcom_el_add_event(el, ev, error);
    if(event_id < 0)
    {
        g_assert(error == NULL || *error != NULL);
        return -1;
    }

    return event_id;
}

gint
rtcom_el_plugin_call_log_voicemail(
        RTComEl * el,
        RTComElEvent * ev,
        GError ** error)
{
    gint event_id;

    if(_common_logging(el, ev, error) < 0)
    {
        g_assert(error == NULL || *error != NULL);
        return -1;
    }

    RTCOM_EL_EVENT_SET_FIELD(ev, event_type, "RTCOM_EL_EVENTTYPE_CALL_VOICEMAIL");

    event_id = rtcom_el_add_event(el, ev, error);
    if(event_id < 0)
    {
        g_assert(error == NULL || *error != NULL);
        return -1;
    }

    return event_id;
}

gboolean
rtcom_el_plugin_call_increase_inbound_meter(
        RTComEl * el,
        RTComElPluginCallType type,
        gint secs)
{
    sqlite3 * db;
    sqlite3_stmt * stmt;
    const gchar * sql = NULL;
    int ret;

    g_return_val_if_fail(RTCOM_IS_EL(el), FALSE);

    g_debug("secs = %d", secs);

    g_object_get(el, "db", &db, NULL);

    switch(type)
    {
        case RTCOM_EL_PLUGIN_CALL_TYPE_GSM:
            sql = "UPDATE call_duration SET inbound_gsm = ? + inbound_gsm;";
            break;
        case RTCOM_EL_PLUGIN_CALL_TYPE_VOIP:
            sql = "UPDATE call_duration SET inbound_voip = ? + inbound_voip;";
            break;
    }

    ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if(ret != SQLITE_OK)
    {
        goto increase_inbound_meter_error;
    }

    ret = sqlite3_bind_int(stmt, 1, secs);
    if(ret != SQLITE_OK)
    {
        goto increase_inbound_meter_error;
    }

    ret = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return TRUE;

increase_inbound_meter_error:
    sqlite3_finalize(stmt);
    return FALSE;
}

gboolean
rtcom_el_plugin_call_increase_outbound_meter(
        RTComEl * el,
        RTComElPluginCallType type,
        gint secs)
{
    sqlite3 * db;
    sqlite3_stmt * stmt;
    const gchar * sql = NULL;
    int ret;

    g_return_val_if_fail(RTCOM_IS_EL(el), FALSE);

    g_object_get(el, "db", &db, NULL);
    switch(type)
    {
        case RTCOM_EL_PLUGIN_CALL_TYPE_GSM:
            sql = "UPDATE call_duration SET outbound_gsm = ? + outbound_gsm;";
            break;
        case RTCOM_EL_PLUGIN_CALL_TYPE_VOIP:
            sql = "UPDATE call_duration SET outbound_voip = ? + outbound_voip;";
            break;
    }
    ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if(ret != SQLITE_OK)
    {
        goto increase_outbound_meter_error;
    }

    ret = sqlite3_bind_int(stmt, 1, secs);
    if(ret != SQLITE_OK)
    {
        goto increase_outbound_meter_error;
    }

    ret = sqlite3_step(stmt);
    if(ret != SQLITE_OK)
    {
        goto increase_outbound_meter_error;
    }

    sqlite3_finalize(stmt);
    return TRUE;

increase_outbound_meter_error:
    sqlite3_finalize(stmt);
    return FALSE;
}

gint
rtcom_el_plugin_call_get_inbound_meter(
        RTComEl * el,
        RTComElPluginCallType type)
{
    sqlite3 * db;
    sqlite3_stmt * stmt;
    const gchar * sql = NULL;
    int ret;

    g_return_val_if_fail(RTCOM_IS_EL(el), -1);

    g_object_get(el, "db", &db, NULL);
    switch(type)
    {
        case RTCOM_EL_PLUGIN_CALL_TYPE_GSM:
            sql = "SELECT inbound_gsm FROM call_duration;";
            break;
        case RTCOM_EL_PLUGIN_CALL_TYPE_VOIP:
            sql = "SELECT inbound_voip FROM call_duration;";
            break;
    }
    ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if(ret != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return -1;
    }

    ret = sqlite3_step(stmt);
    if(ret == SQLITE_ROW)
    {
        gint count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);

        return count;
    }

    return -1;
}

gint
rtcom_el_plugin_call_get_outbound_meter(
        RTComEl * el,
        RTComElPluginCallType type)
{
    sqlite3 * db;
    sqlite3_stmt * stmt;
    const gchar * sql = NULL;
    int ret;

    g_return_val_if_fail(RTCOM_IS_EL(el), -1);

    g_object_get(el, "db", &db, NULL);
    switch(type)
    {
        case RTCOM_EL_PLUGIN_CALL_TYPE_GSM:
            sql = "SELECT outbound_gsm FROM call_duration;";
            break;
        case RTCOM_EL_PLUGIN_CALL_TYPE_VOIP:
            sql = "SELECT outbound_voip FROM call_duration;";
            break;
    }
    ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if(ret != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return -1;
    }

    ret = sqlite3_step(stmt);
    if(ret == SQLITE_ROW)
    {
        gint count = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);

        return count;
    }

    return -1;
}

gboolean
rtcom_el_plugin_call_reset_meters(
        RTComEl * el)
{
    sqlite3 * db;
    const gchar * sql;
    int ret;

    g_return_val_if_fail(RTCOM_IS_EL(el), FALSE);

    g_object_get(el, "db", &db, NULL);
    sql = "UPDATE call_duration SET inbound_gsm = 0, outbound_gsm = 0, "
          "inbound_voip = 0, outbound_voip = 0;";
    ret = sqlite3_exec(db, sql, NULL, NULL, NULL);

    return ret == SQLITE_OK;
}

gboolean
rtcom_el_plugin_call_delete_all(
        RTComEl * el)
{
    return rtcom_el_delete_by_service(el, "RTCOM_EL_SERVICE_CALL");
}

/* vim: set ai et tw=75 ts=4 sw=4: */

