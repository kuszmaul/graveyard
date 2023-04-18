set terminal pdfcairo
set datafile separator comma
set key top left reverse

FBcolor = "#4267B2"
Gcolor = "#0F9D58"
Tcolor = "#00FFFF"
Tcolor2 = "#00FF00"
Ccolor = "#FF0000"

set xlabel "Table size (elements)"
set ylabel "Time per operation (ns)"
set title "Insertion-only workload (from empty, no reserve)"
set output "library-insertion.pdf"
set yrange [0:600]
plot "../../data/insert_graveyard.data"            using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 1 title "Graveyard",\
     "../../data/insert_graveyard-likeabseil.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor2  dt 1 title "Graveyard Like Abseil",\
     "../../data/insert_google.data"               using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 1 title "Google",\
     "../../data/insert_facebook.data"             using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 1 title "Facebook",\
     "../../data/insert_cuckoo.data"               using 1:($2/$1)                     with lines       linecolor rgb Ccolor dt 1 title "Libcuckoo",\
     "../../data/insert_graveyard.data"            using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Tcolor  fs transparent solid 0.1 notitle,\
     "../../data/insert_google.data"               using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Gcolor  fs transparent solid 0.1 notitle,\
     "../../data/insert_facebook.data"             using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "../../data/insert_cuckoo.data"               using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Ccolor  fs transparent solid 0.1 notitle,\

set title "Insertion-only workload (from empty, reserved)"
set output "library-reserved-insertion.pdf"
plot "../../data/reserved-insert_graveyard.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 1 title "Graveyard",\
     "../../data/reserved-insert_graveyard-likeabseil.data" using 1:($2/$1)          with lines       linecolor rgb Tcolor2  dt 1 title "Graveyard Like Abseil",\
     "../../data/reserved-insert_google.data"    using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 1 title "Google",\
     "../../data/reserved-insert_facebook.data"  using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 1 title "Facebook",\
     "../../data/reserved-insert_cuckoo.data"    using 1:($2/$1)                     with lines       linecolor rgb Ccolor dt 1 title "Libcuckoo",\
     "../../data/reserved-insert_graveyard.data" using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Tcolor  fs transparent solid 0.1 notitle,\
     "../../data/reserved-insert_google.data"    using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Gcolor  fs transparent solid 0.1 notitle,\
     "../../data/reserved-insert_facebook.data"  using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "../../data/reserved-insert_cuckoo.data"    using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Ccolor  fs transparent solid 0.1 notitle,\

set title "Successful find"
set yrange [0:160]
set output "library-found.pdf"
plot "../../data/found_graveyard.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 1 title "Graveyard",\
     "../../data/found_graveyard-likeabseil.data" using 1:($2/$1)          with lines       linecolor rgb Tcolor2  dt 1 title "Graveyard Like Abseil",\
     "../../data/found_google.data"    using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 1 title "Google",\
     "../../data/found_facebook.data"  using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 1 title "Facebook",\
     "../../data/found_cuckoo.data"    using 1:($2/$1)                     with lines       linecolor rgb Ccolor dt 1 title "Libcuckoo",\
     "../../data/found_graveyard.data" using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Tcolor  fs transparent solid 0.1 notitle,\
     "../../data/found_google.data"    using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Gcolor  fs transparent solid 0.1 notitle,\
     "../../data/found_facebook.data"  using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "../../data/found_cuckoo.data"    using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Ccolor  fs transparent solid 0.1 notitle,\

set title "Unsuccessful find"
set yrange [0:160]
set output "library-notfound.pdf"
plot "../../data/notfound_graveyard.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 1 title "Graveyard",\
     "../../data/notfound_graveyard-likeabseil.data" using 1:($2/$1)          with lines       linecolor rgb Tcolor2  dt 1 title "Graveyard Like Abseil",\
     "../../data/notfound_google.data"    using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 1 title "Google",\
     "../../data/notfound_facebook.data"  using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 1 title "Facebook",\
     "../../data/notfound_cuckoo.data"    using 1:($2/$1)                     with lines       linecolor rgb Ccolor dt 1 title "Libcuckoo",\
     "../../data/notfound_graveyard.data" using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Tcolor  fs transparent solid 0.1 notitle,\
     "../../data/notfound_google.data"    using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Gcolor  fs transparent solid 0.1 notitle,\
     "../../data/notfound_facebook.data"  using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "../../data/notfound_cuckoo.data"    using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Ccolor  fs transparent solid 0.1 notitle,\
