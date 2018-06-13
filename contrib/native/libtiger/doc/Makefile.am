AM_LIBTOOLFLAGS=--silent

# dependency to get doc to be built
noinst_DATA = .doc-dummy-target

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(docdir)
	for dir in $(top_srcdir)/doc/html; do \
	  if test -d "$$dir"; then \
	    b=`basename $$dir`; \
	    $(mkinstalldirs) $(DESTDIR)$(docdir)/$$b; \
	    for f in "$$dir/"*; do \
	      $(INSTALL_DATA) "$$f" "$(DESTDIR)$(docdir)/$$b"; \
            done \
	  fi \
	done
	for f in README AUTHORS COPYING THANKS ChangeLog; do \
	  $(INSTALL_DATA) $(top_srcdir)/$$f $(DESTDIR)$(docdir); \
	done

uninstall-local:
	rm -rf $(DESTDIR)$(docdir)

distclean-local:
	if test -d html; then rm -rf html; fi
	if test -d latex; then rm -rf latex; fi
	if test -f .doc-dummy-target; then rm -f .doc-dummy-target; fi

if HAVE_DOXYGEN
.doc-dummy-target: tiger.doxygen
	doxygen tiger.doxygen
	touch .doc-dummy-target
else
.doc-dummy-target:
	echo "doxygen not found, cannot build docs"
	/bin/false
endif

dist-hook: .doc-dummy-target
	cp -fR * $(distdir)
	rm -f $(distdir)/doc/tiger.doxygen

.PHONY: force-doc
force-doc: rm-doc-dummy-target .doc-dummy-target

.PHONY: rm-doc-dummy-target
rm-doc-dummy-target:
	rm -f .doc-dummy-target

