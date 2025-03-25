SUBDIRS = driver examples
AUTOMAKE_OPTIONS = 1.4

CLEANFILES = ccver *~ \#*

package:
	./mkdoc
	./mkdist omega $(PACKAGE) $(VERSION)
	rm -f ../$(PACKAGE)-$(VERSION).tar.gz
#	$(MAKE) indent
	$(MAKE) distclean
	cd .. && rm -f $(PACKAGE)-$(VERSION) last && ln -s `basename $(CURDIR)` $(PACKAGE)-$(VERSION) && ln -s $(PACKAGE)-$(VERSION) last && tar -cf - $(PACKAGE)-$(VERSION)/. | gzip -c > $(PACKAGE)-$(VERSION).tar.gz

html:
#	$(MAKE) indent
	$(MAKE) clean-html
	gtags
	htags -fnvat 'Source Code Tour for $(PACKAGE) $(VERSION)'
	lynx HTML/index.html

# Do not indent, or will cause strange source-corruption when make indent.
# This problem is not found on manual indent but "make indent".
# The reson is still unknown.
# indent:
#	cd driver && $(MAKE) indent
#	cd include && $(MAKE) indent
#	cd examples && $(MAKE) indent

clean-html:
	rm -rf GPATH GRTAGS GSYMS GTAGS HTML

.PHONY: clean-html indent html all package
