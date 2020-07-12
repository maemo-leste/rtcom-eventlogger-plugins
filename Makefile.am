## Process this file with automake to produce Makefile.in

SUBDIRS = src

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rtcom-eventlogger-plugins.pc

pluginsdocdir=${prefix}/doc/rtcom-eventlogger-plugins
pluginsdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS

nobase_include_HEADERS = rtcom-eventlogger-plugins/sms.h \
                         rtcom-eventlogger-plugins/call.h \
			 rtcom-eventlogger-plugins/chat.h

EXTRA_DIST = $(pluginsdoc_DATA)

