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

#ifndef RTCOM_EVENTLOGGER_PLUGINS_CALL_H
#define RTCOM_EVENTLOGGER_PLUGINS_CALL_H

#include <glib.h>
#include <rtcom-eventlogger/eventlogger.h>
#include <rtcom-eventlogger/event.h>

typedef enum
{
    RTCOM_EL_PLUGIN_CALL_TYPE_GSM,
    RTCOM_EL_PLUGIN_CALL_TYPE_VOIP
} RTComElPluginCallType;

/**
 * Logs an inbound call.
 * @param el The RTComEl object
 * @param ev An RTComElEvent that you have partially populated
 * @param error A location for the error.
 */
gint
rtcom_el_plugin_call_log_inbound(
        RTComEl * el,
        RTComElEvent * ev,
        GError ** error);

/**
 * Logs an outbound call.
 * @param el The RTComEl object
 * @param ev An RTComElEvent that you have partially populated
 * @param error A location for the error.
 */
gint
rtcom_el_plugin_call_log_outbound(
        RTComEl * el,
        RTComElEvent * ev,
        GError ** error);

/**
 * Logs a missed call.
 * @param el The RTComEl object
 * @param ev An RTComElEvent that you have partially populated
 * @param error A location for the error.
 */
gint
rtcom_el_plugin_call_log_missed(
        RTComEl * el,
        RTComElEvent * ev,
        GError ** error);

/**
 * Logs a voicemail notification.
 * @param el The RTComEl object
 * @param ev An RTComElEvent that you have partially populated
 * @param error A location for the error.
 */
gint
rtcom_el_plugin_call_log_voicemail(
        RTComEl * el,
        RTComElEvent * ev,
        GError ** error);

/**
 * Updates the duration of a call and the global timers.
 * @param el The RTComEl object
 * @param event_id The event_id that needs to be updated
 * @param duration The duration in seconds
 * @param update_timers Set this to TRUE if you want to also increase the
 * global timer for that kind of call (inbound/outbound)
 * @param type If you're updating the timer, we need to know if you want to
 * update the GSM or VOIP meter.
 * @error A location for the error
 * @return TRUE in case of success, FALSE otherwise
 */
gboolean
rtcom_el_plugin_call_update_duration(
        RTComEl * el,
        guint event_id,
        gint duration,
        gboolean update_timers,
        RTComElPluginCallType type,
        GError ** error);

/**
 * Increases the inbound calls duration meter.
 * @param el The RTComEl object
 * @param type GSM or VOIP meter
 * @param secs The amount of seconds to add
 * @return TRUE in case of success, FALSE otherwise
 */
gboolean
rtcom_el_plugin_call_increase_inbound_meter(
        RTComEl * el,
        RTComElPluginCallType type,
        gint secs);

/**
 * Increases the outbound calls duration meter.
 * @param el The RTComEl object
 * @param type GSM or VOIP meter
 * @param secs The amount of seconds to add
 * @return TRUE in case of success, FALSE otherwise
 */
gboolean
rtcom_el_plugin_call_increase_outbound_meter(
        RTComEl * el,
        RTComElPluginCallType type,
        gint secs);

/**
 * Gets the inbound calls duration meter.
 * @param el The RTComEl object
 * @param type GSM or VOIP meter
 * @return The number of seconds in the inbound calls meter, or -1 in case
 * of error.
 */
gint
rtcom_el_plugin_call_get_inbound_meter(
        RTComEl * el,
        RTComElPluginCallType type);

/**
 * Gets the outbound calls duration meter.
 * @param el The RTComEl object
 * @param type GSM or VOIP meter
 * @return The number of seconds in the outbound calls meter, or -1 in case
 * of error.
 */
gint
rtcom_el_plugin_call_get_outbound_meter(
        RTComEl * el,
        RTComElPluginCallType type);

/**
 * Resets the calls duration meters.
 * @param el The RTComEl object
 * @return TRUE in case of success, FALSE otherwise
 */
gboolean
rtcom_el_plugin_call_reset_meters(
        RTComEl * el);

/**
 * Deletes all the calls from the database.
 * @param el The RTComEl object
 * @return TRUE in case of success, FALSE otherwise
 */
gboolean
rtcom_el_plugin_call_delete_all(
        RTComEl * el);

#endif

/* vim: set ai et tw=75 ts=4 sw=4: */

