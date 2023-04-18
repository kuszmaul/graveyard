set terminal pdfcairo
set datafile separator comma
set key top left reverse

FBcolor = "#4267B2"
Gcolor = "#0F9D58"
Tcolor = "#00FFFF"
Ccolor = "#FF0000"

set xlabel "Table size (elements)"
set ylabel "Time per operation (ns)"
set title "Insertion-only workload (from empty, no reserve)"
set output "library-insertion.pdf"
set yrange [0:600]
plot "../../data/insert_graveyard.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 1 title "Graveyard",\
     "../../data/insert_google.data"           using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 1 title "Google",\
     "../../data/insert_facebook.data"         using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 1 title "Facebook",\
     "../../data/insert_cuckoo.data"         using 1:($2/$1)                     with lines       linecolor rgb Ccolor dt 1 title "Libcuckoo",\
     "../../data/insert_graveyard.data"        using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Tcolor  fs transparent solid 0.1 notitle,\
     "../../data/insert_google.data"           using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Gcolor  fs transparent solid 0.1 notitle,\
     "../../data/insert_facebook.data"         using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "../../data/insert_cuckoo.data"           using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Ccolor  fs transparent solid 0.1 notitle,\
