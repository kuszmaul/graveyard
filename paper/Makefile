graveyard.pdf: graveyard.tex
	pdflatex graveyard

INCREASING_LOAD=increasing-load-found increasing-load-notfound increasing-load-findempty increasing-load-findempty-ratio
INCREASING_LOAD_PDFS = $(patsubst %,experiments/%.pdf, $(INCREASING_LOAD))

VARY_TOMBSTONES=vary-tombstones
VARY_TOMBSTONES_PDFS = $(patsubst %,experiments/%.pdf, $(VARY_TOMBSTONES))

EXPERIMENTAL_PDFS = $(INCREASING_LOAD_PDFS) $(VARY_TOMBSTONES_PDFS)

foo:
	echo $(INCREASING_LOAD_PDFS)

graveyard.pdf: $(EXPERIMENTAL_PDFS)

$(INCREASING_LOAD_PDFS) &: experiments/increasing-load.data experiments/increasing-load.gnuplot
	cd experiments; gnuplot increasing-load.gnuplot

$(VARY_TOMBSTONES_PDFS) &: experiments/vary-tombstones.data experiments/vary-tombstones.gnuplot
	cd experiments; gnuplot vary-tombstones.gnuplot

CXXFLAGS = -std=c++17 -Werror -Wall -W -O2 -ggdb
experiments/insert-only: experiments/insert-only.cc ../benchmark/statistics.h experiments/hash_function.h

experiments/vary-tombstones.data experiments/increasing-load.data &: experiments/insert-only
	(cd experiments; ./insert-only)

experiments/olp-spike: experiments/olp-spike.cc experiments/hash_function.h
	$(CXX) $(CXXFLAGS) $< -o $@

experiments/olp-spike.run: experiments/olp-spike
	cd experiments; ./olp-spike
