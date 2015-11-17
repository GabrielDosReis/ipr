dist_data_DATA = macro.tex ref.tex user-guide.tex ipr.bib

dvi-local: ref.dvi user-guide.dvi

.tex.dvi: %.tex macro.tex ipr.bib
	$(LATEX) $<
