include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_say_nb

mod_LTLIBRARIES = mod_say_nb.la
mod_say_nb_la_SOURCES  = mod_say_nb.c
mod_say_nb_la_CFLAGS   = $(AM_CFLAGS)
mod_say_nb_la_LIBADD   = $(switch_builddir)/libfreeswitch.la
mod_say_nb_la_LDFLAGS  = -avoid-version -module -no-undefined -shared
