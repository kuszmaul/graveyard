set datafile separator comma
set terminal svg
set key top left reverse
set xlabel "Final size"
set ylabel "Time per operation (ns)"
set yrange [0:300]

Shadecolor1 = "#80e0a080"
Shadecolor2 = "#90e0a080"

set output "plots/insert-time.svg"
set title "Insertion (from empty, no reserve)"
plot "data/insert_flatset.data"        using 1:($2/$1) with lines title "flatset",\
     "data/insert_flatset-nohash.data" using 1:($2/$1) with lines title "flatset-nohash",\
     "data/insert_OLP.data"            using 1:($2/$1) with lines title "OLP",\
     "data/insert_OLP-idhash.data"     using 1:($2/$1) with lines title "OLP-nohash",\
     "data/insert_F14.data"            using 1:($2/$1) with lines title "F14",\
     "data/insert_flatset.data"        using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 1 fs transparent solid 0.1 notitle,\
     "data/insert_flatset-nohash.data" using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 2 fs transparent solid 0.1 notitle,\
     "data/insert_OLP.data"            using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 3 fs transparent solid 0.1 notitle,\
     "data/insert_OLP-idhash.data"     using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 4 fs transparent solid 0.1 notitle,\
     "data/insert_F14.data"            using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 5 fs transparent solid 0.1 notitle,\

set output "plots/reserved-insert-time.svg"
set title "Insertion with Reserve"
plot "data/reserved-insert_flatset.data" using 1:($2/$1) with lines title "flatset",\
     "data/reserved-insert_flatset-nohash.data" using 1:($2/$1) with lines title "flatset-nohash",\
     "data/reserved-insert_OLP.data"            using 1:($2/$1) with lines title "OLP",\
     "data/reserved-insert_OLP-nohash.data"     using 1:($2/$1) with lines title "OLP-nohash",\
     "data/reserved-insert_F14.data"            using 1:($2/$1) with lines title "F14",\
     "data/reserved-insert_flatset.data"        using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 1 fs transparent solid 0.1 notitle,\
     "data/reserved-insert_flatset-nohash.data" using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 2 fs transparent solid 0.1 notitle,\
     "data/reserved-insert_OLP.data"            using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 3 fs transparent solid 0.1 notitle,\
     "data/reserved-insert_OLP-nohash.data"     using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 4 fs transparent solid 0.1 notitle,\
     "data/reserved-insert_F14.data"            using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 5 fs transparent solid 0.1 notitle,\

set yrange [0:80]
set output "plots/found-time.svg"
set title "Successful find"
plot "data/found_flatset.data" using 1:($2/$1) with lines title "flatset",\
     "data/found_flatset-nohash.data" using 1:($2/$1) with lines title "flatset-nohash",\
     "data/found_OLP.data"            using 1:($2/$1) with lines title "OLP",\
     "data/found_OLP-nohash.data"     using 1:($2/$1) with lines title "OLP-nohash",\
     "data/found_F14.data" using 1:($2/$1) with lines title "F14",\
     "data/found_flatset.data"        using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 1 fs transparent solid 0.1 notitle,\
     "data/found_flatset-nohash.data" using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 2 fs transparent solid 0.1 notitle,\
     "data/found_OLP.data"            using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 3 fs transparent solid 0.1 notitle,\
     "data/found_OLP-nohash.data"     using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 4 fs transparent solid 0.1 notitle,\
     "data/found_F14.data"            using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 5 fs transparent solid 0.1 notitle,\

set output "plots/notfound-time.svg"
set title "Unsuccessful find"
plot "data/notfound_flatset.data" using 1:($2/$1) with lines title "flatset",\
     "data/notfound_flatset-nohash.data" using 1:($2/$1) with lines title "flatset-nohash",\
     "data/notfound_OLP.data"            using 1:($2/$1) with lines title "OLP",\
     "data/notfound_OLP-idhash.data"     using 1:($2/$1) with lines title "OLP-nohash",\
     "data/notfound_F14.data" using 1:($2/$1) with lines title "F14",\
     "data/notfound_flatset.data"        using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 1 fs transparent solid 0.1 notitle,\
     "data/notfound_flatset-nohash.data" using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 2 fs transparent solid 0.1 notitle,\
     "data/notfound_OLP.data"            using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 3 fs transparent solid 0.1 notitle,\
     "data/notfound_OLP-nohash.data"     using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 4 fs transparent solid 0.1 notitle,\
     "data/notfound_F14.data"            using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor 5 fs transparent solid 0.1 notitle,\


set output "plots/memory.svg"
unset yrange
set ylabel "Memory bytes"
set title "Memory"
set key top left Left reverse

f(x) = a * x
fit f(x) "data/insert_flatset.data" using 1:3 via a
g(x) = b * x
fit g(x) "data/insert_OLP.data" using 1:3 via b
h(x) = c * x
fit h(x) "data/insert_OLP.data" using 1:3 via c

plot "data/insert_flatset.data" using 1:3 with lines title sprintf("flatset (%.2f bytes/element)", a), \
     "data/insert_OLP.data" using 1:3 with lines title sprintf("OLP (%.2f bytes/element) (%.1f%% less)",b,100*(b-a)/a) linestyle 3, \
     "data/insert_F14.data" using 1:3 with lines title sprintf("F14 (%.2f bytes/element) (%.1f%% less)",c,100*(c-a)/a) linestyle 4, \
     a * x linecolor 1 with dots notitle, \
     b * x linecolor 3 with dots notitle, \
     c * x linecolor 4 with dots notitle