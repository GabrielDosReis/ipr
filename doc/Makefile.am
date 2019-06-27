dist_data_DATA = macro.tex ref.tex user-guide.tex ipr.bib

pdf-local: ref.pdf user-guide.pdf

.tex.pdf: %.tex macro.tex ipr.bib
	$(PDFLATEX) $<
