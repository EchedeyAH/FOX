PACKAGE = 'ixpci'
VERSION = '0.8.19'

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
	@if [ -e lib/Makefile ] && [ -f lib/src/pcidaq.c ]; then           \
	cd lib/;                                \
	make;                                   \
	fi

clean:
	$(MAKE) -C driver/ clean
	@if [ -e examples/Makefile ]; then	\
        $(MAKE) -C examples/ clean;		\
        fi
	@if [ -e lib/Makefile ]; then		\
        $(MAKE) -C lib/ clean;			\
        fi

distclean:
	@if [ -e examples/Makefile ]; then	\
	$(MAKE) -C examples/ distclean;		\
	fi
	@if [ -e driver/Makefile ]; then	\
        $(MAKE) -C driver/ distclean;		\
        fi	
	@if [ -e lib/Makefile ]; then		\
        $(MAKE) -C lib clean;			\
        $(MAKE) -C lib distclean;		\
        fi

release:
	@if [ -e lib/Makefile ]; then   \
        cd lib/src;                     \
        rm -f *c *h;                    \
        fi

package:
	./mkdist omega $(PACKAGE) $(VERSION)
	rm -f ../$(PACKAGE)-$(VERSION).tar.gz
	$(MAKE) distclean
	cd .. && rm -f $(PACKAGE)-$(VERSION) last && ln -s `basename $(CURDIR)` $(PACKAGE)-$(VERSION) && ln -s $(PACKAGE)-$(VERSION) last && tar -cf - $(PACKAGE)-$(VERSION)/. | gzip -c > $(PACKAGE)-$(VERSION).tar.gz
