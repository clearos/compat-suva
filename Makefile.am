AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = . ./suvlets/Blank/remote ./suvlets/BWMeter/remote ./suvlets/DeviceInfo/remote \
	./suvlets/SecurityAudit/remote ./suvlets/Snort/remote ./suvlets/Software/remote

lib_LTLIBRARIES = libcompat-suva.la

libcompat_suva_la_SOURCES = crc.c mem.c scl_inlines.c scl_reader.c \
	scl_writer.c sutil.c suva_api.c suva_conn.c suva_util.c

EXTRA_DIST = autogen.sh mem.h sutil.h suva_api.h suva_conf.h suva.h suva_util.h

