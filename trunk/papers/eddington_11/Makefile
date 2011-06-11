TARGET = mar

TEXSRC = mar.tex

PDF = $(TARGET).pdf
FIGURES = $(wildcard Images/*.png)
BIBFILE = references.bib
.PHONY: pdf clean 

pdf: $(PDF)

$(PDF): $(TEXSRC) $(FIGURES) $(BIBFILE)
	pdflatex $(TARGET).tex
	bibtex $(TARGET)
	pdflatex $(TARGET).tex

tidy:
	rm -f *.dvi *.log *.aux *.log *.brf *.lof *.lot *.nlo *.out *.toc *.blg *.bbl

clean:
	rm -f *.ps *.pdf *.dvi *.log *.aux *.log *.brf *.lof *.lot *.nlo *.out *.toc

clean-all:
	rm -f *.ps *.pdf *.dvi *.log *.aux *.log *.brf *.lof *.lot *.nlo *.out *.toc *.blg *.bbl

