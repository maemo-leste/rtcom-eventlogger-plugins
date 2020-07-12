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

#include "chat.h"

#include <rtcom-eventlogger/event.h>
#include <sqlite3.h>
#include <string.h>
#include <sched.h>

#define SERVICE_NAME "RTCOM_EL_SERVICE_CHAT"

static gboolean
_execute_sql_query(
        sqlite3 * db,
        const gchar * sql)
{
    sqlite3_stmt * stmt;
    int ret;
    GTimer *t;

    g_return_val_if_fail (db!= NULL, FALSE);
    g_return_val_if_fail (sql!= NULL, FALSE);

    if (SQLITE_OK != sqlite3_prepare_v2(db, sql, -1, &stmt, NULL))
    {
        g_warning("%s: can't compile SQL: %s", G_STRFUNC, sql);
        return FALSE;
    }

    t = g_timer_new ();
    while (1)
    {
        ret = sqlite3_step (stmt);

        if ((ret == SQLITE_BUSY) || (ret == SQLITE_LOCKED))
        {
            if (g_timer_elapsed (t, NULL) < 3.0)
            {
              sched_yield ();
              continue;
            }
            else
            {
              g_warning ("%s: busyloop after %lf seconds, quitting",
                  G_STRFUNC, g_timer_elapsed (t, NULL));
            }
        }
        break;
    }
    g_timer_destroy (t);

    if ((ret != SQLITE_DONE) && (ret != SQLITE_OK))
    {
        g_warning ("%s: error executing '%s': (%d) %s", G_STRFUNC,
           sql, ret, sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return FALSE;
    }

    sqlite3_finalize(stmt);
    return TRUE;
}

static gint
_find_last_event_in_group(
        sqlite3 * db,
        const gchar * group_uid)
{
    sqlite3_stmt * stmt;
    gchar * sql;
    gint event_id = -1;
    int ret;

    sql = sqlite3_mprintf("SELECT MAX(id) FROM Events "
        "WHERE group_uid = %Q", group_uid);

    if (SQLITE_OK != sqlite3_prepare_v2(db, sql, -1, &stmt, NULL))
    {
        sqlite3_free(sql);
        return -1;
    }

    while (SQLITE_BUSY == (ret = sqlite3_step(stmt)));

    if (ret == SQLITE_ROW)
    {
        event_id = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_free(sql);

    return event_id;
}

gboolean
rtcom_el_plugin_chat_set_group_title(
        RTComEl * el,
        const gchar * group_uid,
        const gchar * title)
{
    sqlite3 * db;
    gchar * sql;
    gint event_id;

    g_return_val_if_fail(RTCOM_IS_EL(el), FALSE);

    g_object_get(el, "db", &db, NULL);

    sql = sqlite3_mprintf("INSERT OR REPLACE INTO chat_group_info "
        "(group_uid, group_title) VALUES (%Q, %Q);",
        group_uid, title);

    _execute_sql_query(db, sql);
    sqlite3_free(sql);

    event_id = _find_last_event_in_group(db, group_uid);
    if (event_id != -1)
    {
        rtcom_el_fire_event_updated(el, event_id);
    }

    return TRUE;
}

gchar *
rtcom_el_plugin_chat_get_group_title(
        RTComEl * el,
        const gchar * group_uid)
{
    sqlite3 * db;
    sqlite3_stmt * stmt;
    gchar * sql;
    int ret;
    gchar *title = NULL;

    g_return_val_if_fail(RTCOM_IS_EL(el), FALSE);

    g_object_get(el, "db", &db, NULL);

    sql = sqlite3_mprintf ("SELECT group_title FROM chat_group_info "
            "WHERE group_uid = %Q;", group_uid);

    if (SQLITE_OK != sqlite3_prepare_v2(db, sql, -1, &stmt, NULL))
    {
        g_warning("%s: can't compile SQL: %s", G_STRFUNC, sql);
        sqlite3_free(sql);
        return NULL;
    }

    while (SQLITE_BUSY == (ret = sqlite3_step(stmt)));

    if (ret == SQLITE_ROW)
        title = g_strdup((const gchar *)sqlite3_column_text (stmt, 0));
    else
        g_warning("%s: error while executing SQL", G_STRFUNC);

    sqlite3_finalize(stmt);
    sqlite3_free(sql);

    return title;
}

gboolean
rtcom_el_plugin_chat_set_read_by_tokens (
        RTComEl * el,
        gboolean is_read,
        const gchar **message_tokens)
{
    sqlite3 * db;
    gchar * sql;
    gint i;
    gint len;
    gint ret;
    gint *ids;
    GString *str;
    sqlite3_stmt * stmt;

    g_return_val_if_fail(RTCOM_IS_EL(el), FALSE);
    g_return_val_if_fail(message_tokens != NULL, FALSE);

    /* special-case nop so we don't get a sql error for empty IN clause */
    if (message_tokens[0] == NULL)
        return TRUE;

    g_object_get(el, "db", &db, NULL);

    /* First, get all the event ID's (we need those also for emitting
     * the EventUpdated signal), and then mark them as (un)read. */

    str = g_string_sized_new (1024); /* size hint for performance */
    g_string_append (str, "SELECT event_id FROM Headers WHERE "
        "name = 'message-token' AND value IN (");

    for (len = 0; message_tokens[len] != NULL; len++)
    {
        if (strchr (message_tokens[len], '\''))
        {
            g_warning ("%s: skipping illegal and unsafe message-token %s",
                G_STRFUNC, message_tokens[len]);
            continue;
        }

        g_string_append_printf (str, "'%s'", message_tokens[len]);

        if (message_tokens[len + 1] != NULL)
            g_string_append_c (str, ',');
    }

    g_string_append (str, ");");
    sql = g_string_free (str, FALSE);

    if (SQLITE_OK != sqlite3_prepare_v2(db, sql, -1, &stmt, NULL))
    {
        g_warning("%s: can't compile SQL: %s", G_STRFUNC, sql);
        g_free (sql);
        return FALSE;
    }

    g_free (sql);
    ids = g_new0 (gint, len + 1);
    i = 0;

    while (SQLITE_DONE != (ret = sqlite3_step(stmt)))
    {
        if (ret == SQLITE_BUSY)
            continue;

        if (ret == SQLITE_ROW)
        {
            gint event_id = sqlite3_column_int(stmt, 0);

            if (i >= len)
            {
                g_warning ("%s: number of events exceeds number of "
                    "message-tokens, ignoring surplus", G_STRFUNC);
            }
            else
            {
                ids[i++] = event_id;
            }
        }
        else
        {
            sqlite3_finalize(stmt);
            g_free (ids);
            return FALSE;
        }
    }

    sqlite3_finalize(stmt);
    len = i;

    if (len == 0)
    {
        g_warning ("%s: no events match the given tokens", G_STRFUNC);
        g_free (ids);
        return FALSE;
    }

    str = g_string_sized_new (1024); /* size hint for performance */
    g_string_printf (str, "UPDATE Events SET is_read = %d WHERE ID IN (",
        is_read);

    for (i = 0; i < len; i++)
    {
        g_string_append_printf (str, "%d%c", ids[i],
            ((i + 1) < len) ? ',' : ' ');
    }

    g_string_append (str, ");");
    sql = g_string_free (str, FALSE);

    if (!_execute_sql_query(db, sql))
    {
        g_free (sql);
        g_free (ids);
        return FALSE;
    }

    for (i = 0; i < len; i++)
    {
        rtcom_el_fire_event_updated(el, ids[i]);
    }

    g_free (sql);
    g_free (ids);

    return TRUE;
}

