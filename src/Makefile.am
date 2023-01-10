## Process this file with automake to produce Makefile.in

plugins_LTLIBRARIES = libsms.la libcall.la libchat.la
pluginsdir = $(libdir)/rtcom-eventlogger/plugins

AM_CFLAGS = \
	$(RTCOM_EVENTLOGGER_PLUGINS_CFLAGS) \
	-I../rtcom-eventlogger-plugins/ \
	-DPIXBUFS_DIR="\"$(datadir)/rtcom-eventlogger/plugins/pixbufs\""

libsms_includedir = $(includedir)
libsms_la_SOURCES = sms-plugin.c
libsms_la_LIBADD = $(RTCOM_EVENTLOGGER_PLUGINS_LIBS) librtcom-el-plugin-sms.la
libsms_la_LDFLAGS = -module -avoid-version

libcall_includedir = $(includedir)
libcall_la_SOURCES = call-plugin.c
libcall_la_LIBADD = $(RTCOM_EVENTLOGGER_PLUGINS_LIBS) librtcom-el-plugin-call.la
libcall_la_LDFLAGS = -module -avoid-version

libchat_includedir = $(includedir)
libchat_la_SOURCES = chat-plugin.c
libchat_la_LIBADD = $(RTCOM_EVENTLOGGER_PLUGINS_LIBS) librtcom-el-plugin-chat.la
libchat_la_LDFLAGS = -module -avoid-version

plugins_api_LTLIBRARIES = librtcom-el-plugin-sms.la librtcom-el-plugin-call.la librtcom-el-plugin-chat.la
plugins_apidir = $(libdir)

librtcom_el_plugin_sms_includedir = $(includedir)
librtcom_el_plugin_sms_la_SOURCES = rtcom-el-plugin-sms.c
librtcom_el_plugin_sms_la_LIBADD = $(RTCOM_EVENTLOGGER_PLUGINS_LIBS)
librtcom_el_plugin_sms_la_LDFLAGS = -no-undefined \
        -export-symbols-regex "^rtcom_el_plugin_sms.*" \
        -version-info "$(LT_CURRENT)":"$(LT_REVISION)":"$(LT_AGE)"

librtcom_el_plugin_call_includedir = $(includedir)
librtcom_el_plugin_call_la_SOURCES = rtcom-el-plugin-call.c
librtcom_el_plugin_call_la_LIBADD = $(RTCOM_EVENTLOGGER_PLUGINS_LIBS)
librtcom_el_plugin_call_la_LDFLAGS = -no-undefined \
        -export-symbols-regex "^rtcom_el_plugin_call.*" \
        -version-info "$(LT_CURRENT)":"$(LT_REVISION)":"$(LT_AGE)"

librtcom_el_plugin_chat_includedir = $(includedir)
librtcom_el_plugin_chat_la_SOURCES = rtcom-el-plugin-chat.c
librtcom_el_plugin_chat_la_LIBADD = $(RTCOM_EVENTLOGGER_PLUGINS_LIBS)
librtcom_el_plugin_chat_la_LDFLAGS = -no-undefined \
        -export-symbols-regex "^rtcom_el_plugin_chat.*" \
        -version-info "$(LT_CURRENT)":"$(LT_REVISION)":"$(LT_AGE)"

