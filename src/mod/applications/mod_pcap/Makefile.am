include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_pcap

mod_LTLIBRARIES = mod_pcap.la
mod_pcap_la_SOURCES  = mod_pcap.c
mod_pcap_la_CFLAGS   = $(AM_CFLAGS)
mod_pcap_la_LIBADD   = $(switch_builddir)/libfreeswitch.la
mod_pcap_la_LDFLAGS  = -avoid-version -module -no-undefined -shared
