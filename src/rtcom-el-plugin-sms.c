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

#include "sms.h"

gboolean
rtcom_el_plugin_sms_get_event_by_message_token(
        RTComEl * el,
        const gchar * token,
        RTComElEvent * ev)
{
    gint * ids, id;
    gboolean ret = FALSE;

    RTComElQuery * query;
    RTComElIter * it = NULL;

    g_warning ("%s: deprecated, use 'message-token' with RTComElQuery instead",
        G_STRFUNC);

    g_return_val_if_fail(RTCOM_IS_EL(el), FALSE);
    g_return_val_if_fail(token != NULL, FALSE);

    if(*token == '\0')
    {
        return ret;
    }

    ids = rtcom_el_get_events_by_header(el, "message-token", token);
    if(ids == NULL || ids[0] == -1)
    {
        g_debug("%s: no events have SMS UID '%s'.", G_STRLOC, token);
        g_free(ids);
        return ret;
    }

    id = ids[0];
    g_free(ids);

    query = rtcom_el_query_new(el);
    if(!rtcom_el_query_prepare(
                query,
                "id", id, RTCOM_EL_OP_EQUAL,
                NULL))
    {
        goto err_unref_query;
    }

    it = rtcom_el_get_events(el, query);
    if(it == NULL)
        goto err_unref_query;
 
    if(!rtcom_el_iter_first(it))
        goto err_unref_iter;

    if(!rtcom_el_iter_get_full(it, ev))
        goto err_unref_iter;

    ret = TRUE;

err_unref_iter:
    g_object_unref(it);

err_unref_query:
    g_object_unref(query);

    return ret;
}

/* vim: set ai et tw=75 ts=4 sw=4: */

