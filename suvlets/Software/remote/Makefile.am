AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = Software software-wrapper

Software_SOURCES = Software.c suva_receiver.c
Software_LDFLAGS = -static
software_wrapper_SOURCES = software-wrapper.c
software_wrapper_LDFLAGS = -static

EXTRA_DIST = Software.h suva_receiver.h
