SUBDIRS = lib driver examples
#SUBDIRS = include driver lib examples
AUTOMAKE_OPTIONS = 1.4

CLEANFILES = ccver *~ \#*

package:
	cd .. && rm -f $(PACKAGE)-$(VERSION) last && ln -s `basename $(CURDIR)` $(PACKAGE)-$(VERSION) && ln -s $(PACKAGE)-$(VERSION) last
	./mkdoc
	./mkdist omega $(PACKAGE) $(VERSION)
	rm -f ../$(PACKAGE)-$(VERSION)*.tar.gz
#	$(MAKE) indent
	$(MAKE) distclean
	cd .. && tar -cf - $(PACKAGE)-$(VERSION)/. | gzip --best -c > $(PACKAGE)-$(VERSION).tar.gz

html:
#	$(MAKE) indent
	$(MAKE) clean-html
	gtags
	htags -fnvat 'ICPDAS $(PACKAGE)-$(VERSION) Source Codes Tour'
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
