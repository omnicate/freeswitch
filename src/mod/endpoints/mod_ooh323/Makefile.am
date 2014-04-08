include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_ooh323
OOH323C=ooh323c-0.1
OOH323C_DIR=$(switch_srcdir)/libs/$(OOH323C)
OOH323C_BUILDDIR=$(switch_builddir)/libs/$(OOH323C)

OOH323CA=$(OOH323C_BUILDDIR)/lib/libooh323c.a

mod_LTLIBRARIES = mod_ooh323.la
mod_ooh323_la_SOURCES  = mod_ooh323.c h323_glue.c
mod_ooh323_la_CFLAGS   = $(AM_CFLAGS) -I$(OOH323C_DIR)/src -I$(OOH323C_DIR)/src/h323_v6 -I$(OOH323C_DIR)/src/h323 -I$(OOH323C_BUILDDIR) -UHAVE_CONFIG_H
mod_ooh323_la_LIBADD   = $(switch_builddir)/libfreeswitch.la $(OOH323CA)
mod_ooh323_la_LDFLAGS  = -avoid-version -module -no-undefined


MY_DEFAULT_ARGS=--build=$(build) --host=$(host) --target=$(target) --disable-shared --with-pic

BUILT_SOURCES=$(OOH323CA)

$(OOH323C_DIR):
	$(GETLIB) $(OOH323C).tar.gz

$(OOH323C_BUILDDIR)/Makefile: $(OOH323C_DIR)
	mkdir -p $(OOH323C_BUILDDIR)
	echo $(MY_DEFAULT_ARGS)
	cd $(OOH323C_BUILDDIR) && cmake .
	$(TOUCH_TARGET)

$(OOH323CA): $(OOH323C_BUILDDIR)/Makefile
	cd $(OOH323C_BUILDDIR) && $(MAKE)
	$(TOUCH_TARGET)
