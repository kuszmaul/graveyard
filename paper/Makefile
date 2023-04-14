graveyard.pdf: graveyard.tex
	pdflatex graveyard

INCREASING_LOAD=increasing-load-found increasing-load-notfound increasing-load-findempty increasing-load-findempty-ratio
INCREASING_LOAD_PDFS = $(patsubst %,experiments/%.pdf, $(INCREASING_LOAD))
EXPERIMENTAL_PDFS = $(INCREASING_LOAD_PDFS)

foo:
	echo $(INCREASING_LOAD_PDFS)

graveyard.pdf: $(EXPERIMENTAL_PDFS)

$(INCREASING_LOAD_PDFS): experiments/increasing_load_pdfs
experiments/increasing_load_pdfs: experiments/increasing-load.data experiments/increasing-load.gnuplot
	(cd experiments; gnuplot increasing-load.gnuplot)
	touch $@

CXXFLAGS = -std=c++17 -Werror -Wall -W -O2 -ggdb
experiments/insert-only: experiments/insert-only.cc ../benchmark/statistics.h

experiments/increasing-load.data: experiments/insert-only
	(cd experiments; ./insert-only)
