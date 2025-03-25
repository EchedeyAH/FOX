###########################################################################
# File level history, record changes for this file here.
#
# v 0.1.0 11 Jan 2012 by Golden Wang
#   Change version from 0.20.x to 0.21.0
#
# v 0.0.2 4 Sep 2007 by Golden Wang
#   Change version from 0.20.2 to 0.20.3
#
# v 0.0.1 28 Jan 2007 by Golden Wang
#   Change version from 0.20.1 to 0.20.2
#
# v 0.0.0 28 Jan 2007 by Golden Wang
#   Change version from 0.20.0 to 0.20.1
#
# v 0.0.0 28 Jan 2007 by Golden Wang
#   Give support to linux kernel 2.6.x
################################################################
PACKAGE = 'ixpio'
VERSION = '0.23.7'

all: modules library example

modules:
	cd driver/;\
	make

example:
	@if [ -e examples/Makefile ]; then      \
	cd examples/;				\
	make;					\
	fi

library:
	@if [ -e lib/Makefile ] && [ -f lib/src/piodio.c ]; then	\
	cd lib/;				\
	make;					\
	fi

clean:
	$(MAKE) -C driver/ clean
	@if [ -e examples/Makefile ]; then      \
	$(MAKE) -C examples/ clean;		\
	fi

distclean:
	@if [ -e examples/Makefile ]; then	\
	$(MAKE) -C examples/ distclean;		\
	fi
	@if [ -e lib/Makefile ] && [ -e lib/src ]; then		\
	$(MAKE) -C lib/ distclean;		\
	rm -f lib/*.a;				\
	rm -f lib/*.so lib/*.so.*;		\
	fi
	@if [ -e driver/Makefile ]; then	\
        $(MAKE) -C driver/ distclean;		\
        fi

release:
	@if [ -e lib/Makefile ]; then	\
	cd lib/src;			\
	rm -f *c *h;			\
	fi

package:
	cd .. && rm -f $(PACKAGE)-$(VERSION) last && ln -s `basename $(CURDIR)` $(PACKAGE)-$(VERSION) && ln -s $(PACKAGE)-$(VERSION) last
	./mkdist omega $(PACKAGE) $(VERSION)
	rm -f ../$(PACKAGE)-$(VERSION)*.tar.gz
	$(MAKE) distclean
	cd .. && tar -cf - $(PACKAGE)-$(VERSION)/. | gzip --best -c > $(PACKAGE)-$(VERSION).tar.gz
