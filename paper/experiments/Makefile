CXXFLAGS = -std=c++17 -Werror -Wall -W -O2 -ggdb

insert-only: insert-only.cc ../../benchmark/statistics.h


increasing-load.data: insert-only
	./insert-only
increasing-load.pdf: increasing-load.data increasing-load.gnuplot
	gnuplot increasing-load.gnuplot
