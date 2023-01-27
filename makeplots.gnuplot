set datafile separator comma
set terminal svg
set key top left
set xlabel "Final size"
set ylabel "Time (ns)"
set yrange [0:300]

set output "plots/insert-time.svg"
set title "Insertion (from empty, no reserve)"
plot "data/insert_flatset.data" using 1:($2/$1) with lines title "flatset","data/insert_flatset-nohash.data" using 1:($2/$1) with lines title "flatset-nohash","data/insert_SimpleILP.data" using 1:($2/$1) with lines title "SimpleILP"

set output "plots/reserved-insert-time.svg"
set title "Insertion with Reserve"
plot "data/reserved-insert_flatset.data" using 1:($2/$1) with lines title "flatset",\
     "data/reserved-insert_flatset-nohash.data" using 1:($2/$1) with lines title "flatset-nohash",\
     "data/reserved-insert_SimpleILP.data" using 1:($2/$1) with lines title "SimpleILP"

set output "plots/found-time.svg"
set title "Successful find"
plot "data/found_flatset.data" using 1:($2/$1) with lines title "flatset",\
     "data/found_flatset-nohash.data" using 1:($2/$1) with lines title "flatset-nohash",\
     "data/found_SimpleILP.data" using 1:($2/$1) with lines title "SimpleILP"

set output "plots/notfound-time.svg"
set title "Unsuccessful find"
plot "data/notfound_flatset.data" using 1:($2/$1) with lines title "flatset",\
     "data/notfound_flatset-nohash.data" using 1:($2/$1) with lines title "flatset-nohash",\
     "data/notfound_SimpleILP.data" using 1:($2/$1) with lines title "SimpleILP"

set output "plots/memory.svg"
unset yrange
set ylabel "Memory bytes"
set title "Memory"
plot "data/insert_flatset.data" using 1:3 with lines title "flatset","data/insert_SimpleILP.data" using 1:3 with lines title "SimpleILP" linestyle 3