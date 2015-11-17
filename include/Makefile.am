STAMP	= echo timestamp >

# We want this path to be absolute, and not relative, because otheriwse
# symbolic links will not work properly.
ipr_header_srcdir = @abs_srcdir@

# By symmetry, we apply the above requirement to the build dir.
ipr_header_builddir = @abs_builddir@/ipr

# Headers from the IPR source code; they are not installed
# dorectly as.  See install-data-local for the rules governing
# their installation.  We list them here, otherwise they would
# be missed by `make dist'.
ipr_header_srcfiles = interface.hxx \
		      impl.hxx \
		      io.hxx traversal.hxx \
		      utility.hxx \
		      node-category.def \
		      lexer.hxx
noinst_HEADERS	= $(ipr_header_srcfiles) 

# Header file names as we publically advertise them
ipr_public_header_files = interface impl io traversal utility lexer

# Rule for building header files.
all-local: stamp-ipr

# Rename IPR source header files from foo.h to ipr/foo
stamp-ipr: $(ipr_header_srcfiles) Makefile
	if [ ! -d ${ipr_header_builddir} ]; then \
	    mkdir -p -- ${ipr_header_builddir} || exit 1;\
	fi ;\
	if [ ! -f stamp-ipr ]; then \
	   for h in $(ipr_public_header_files); do \
              $(LN_S) ${ipr_header_srcdir}/$$h.hxx \
                      ${ipr_header_builddir}/$$h || exit 1;\
	    done ;\
	$(LN_S) ${ipr_header_srcdir}/node-category.def \
		${ipr_header_builddir}/node-category || exit 1;\
	$(STAMP) stamp-ipr;\
	fi ;

install-data-local:
	$(mkinstalldirs) $(includedir)/ipr
	for h in ${ipr_public_header_files}; do \
	  $(INSTALL_DATA) ${ipr_header_builddir}/$$h $(includedir)/ipr ;\
	done;\
	$(INSTALL_DATA) ${ipr_header_builddir}/node-category $(includedir)/ipr

clean-local:
	rm -rf $(ipr_header_builddir);\
	rm -f stamp-ipr;
