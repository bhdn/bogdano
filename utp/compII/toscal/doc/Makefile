all: mp.html mp.pdf mp.txt
mp.pdf: model1.eps
%.pdf: %.dvi
	dvipdfm $<
%.dvi: %.tex
	latex $<
	latex $<
%.tex: %.t2t
	txt2tags --toc -t tex --encoding utf8 $<
%.txt: %.t2t
	txt2tags --toc -t txt --encoding utf8 $<
%.html: %.t2t
	txt2tags --toc -t xhtml --encoding utf8 $<
%.eps: %.dia
	dia --export=$@ $<
