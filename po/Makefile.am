LANGUAGES = fr pt
PO_FILES = fr.po pt.po

# This information is here to help translators create/update (pot/po files):
#
# To create a new pot file, you need to cd into this po directory first:
#   cd po
#   del gimp30-fourier.pot
#   make update-pot
#
# To update existing po files you need to cd into po directory and then do:
#   cd po
#   make update-po
#

.PHONY: all install uninstall clean $(LANGUAGES)
.PHONY: update-po update-pot

all: update-pot $(LANGUAGES)

$(LANGUAGES):
	if [[ -e $(srcdir)/$@.po ]]; \
	then $(MSGFMT) -c -v -o $(builddir)/$@.mo $(srcdir)/$@.po; \
	else $(MSGFMT) -c -v -o $(builddir)/$@.mo $(builddir)/$@.po; \
	fi

update-po: $(PO_FILES)

$(PO_FILES): $(GETTEXT_PACKAGE3).pot
	if [[ -e $(srcdir)/$@ ]]; \
	then $(MSGMERGE) -U $(srcdir)/$@ $^; \
	else $(MSGFMT) -l $(subst .po,,$@) -o $(builddir)/$@ -i $^; \
	fi

install-data-local: $(LANGUAGES)
	for L in $(LANGUAGES); \
	do $(MKDIR_P) -p "$(DESTDIR)$(localedir)/$$L/LC_MESSAGES"; \
	install -v -m 0644 $(builddir)/$$L.mo "$(DESTDIR)$(localedir)/$$L/LC_MESSAGES/$(GETTEXT_PACKAGE3).mo"; \
	done

uninstall-local: $(LANGUAGES)
	for L in $(LANGUAGES); \
	do rm -vf "$(DESTDIR)$(localedir)/$$L/LC_MESSAGES/$(GETTEXT_PACKAGE3).mo"; \
	done

clean-local:
	rm -f *.po~ *.mo *.mo~

update-pot: $(GETTEXT_PACKAGE3).pot

$(GETTEXT_PACKAGE3).pot: ../fourier.c
	$(XGETTEXT) -k_ -k_\" -kd_ -d $(GETTEXT_PACKAGE3) -o $@ \
	--package-version=$(FOURIER_VERSION) --msgid-bugs-address=$(FOURIER_EMAIL) \
	../*.c

EXTRA_DIST = $(PO_FILES) $(GETTEXT_PACKAGE3).pot
