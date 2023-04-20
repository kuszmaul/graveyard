set terminal pdfcairo
set datafile separator comma
set key top left reverse

FBcolor = "#4267B2"
Gcolor = "#0F9D58"
Tcolor = "#006FaF"
Tcolor2 = "#00FF00"
Ccolor = "#FF0000"

set xlabel "Table size (elements)"
set ylabel "Time per operation (ns)"
set title "Insertion-only workload (from empty, no reserve)"
set output "library-insertion.pdf"
set yrange [0:700]
plot "../../data/insert_graveyard-low-load.data"        using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 1 title "Graveyard low load",\
     "../../data/insert_graveyard-medium-load.data"     using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 2 title "Graveyard medium load",\
     "../../data/insert_graveyard-high-load.data"       using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 3 title "Graveyard high load",\
     "../../data/insert_graveyard-high-load-no-tombstones.data"       using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 4 title "Graveyard high load no tombstones",\
     "../../data/insert_google.data"                    using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 1 title "Google",\
     "../../data/insert_facebook.data"                  using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 1 title "Facebook",\
     "../../data/insert_cuckoo.data"                    using 1:($2/$1)                     with lines       linecolor rgb Ccolor dt 1 title "Libcuckoo",\
     "../../data/insert_graveyard-low-load.data"        using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Tcolor  fs transparent solid 0.1 notitle,\
     "../../data/insert_google.data"                    using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Gcolor  fs transparent solid 0.1 notitle,\
     "../../data/insert_facebook.data"                  using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "../../data/insert_cuckoo.data"                    using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Ccolor  fs transparent solid 0.1 notitle,\

set title "Insertion-only workload (from empty, reserved)"
set output "library-reserved-insertion.pdf"
plot "../../data/reserved-insert_graveyard-low-load.data"  using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 1 title "Graveyard low load",\
     "../../data/reserved-insert_graveyard-medium-load.data" using 1:($2/$1)                   with lines       linecolor rgb Tcolor  dt 2 title "Graveyard medium load",\
     "../../data/reserved-insert_graveyard-high-load.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 3 title "Graveyard high load",\
     "../../data/reserved-insert_graveyard-high-load-no-tombtones.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 4 title "Graveyard high load no tombstones",\
     "../../data/reserved-insert_google.data"              using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 1 title "Google",\
     "../../data/reserved-insert_facebook.data"            using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 1 title "Facebook",\
     "../../data/reserved-insert_cuckoo.data"              using 1:($2/$1)                     with lines       linecolor rgb Ccolor dt 1 title "Libcuckoo",\
     "../../data/reserved-insert_graveyard-low-load.data"  using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Tcolor  fs transparent solid 0.1 notitle,\
     "../../data/reserved-insert_google.data"              using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Gcolor  fs transparent solid 0.1 notitle,\
     "../../data/reserved-insert_facebook.data"            using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "../../data/reserved-insert_cuckoo.data"              using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Ccolor  fs transparent solid 0.1 notitle,\

set title "Successful find"
set yrange [0:160]
set output "library-found.pdf"
plot "../../data/found_graveyard-low-load.data"  using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 1 title "Graveyard low load",\
     "../../data/found_graveyard-medium-load.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 2 title "Graveyard medium load",\
     "../../data/found_graveyard-high-load.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 3 title "Graveyard high load",\
     "../../data/found_graveyard-high-load-no-tombstones.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 4 title "Graveyard high load no tombstones",\
     "../../data/found_google.data"              using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 1 title "Google",\
     "../../data/found_facebook.data"            using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 1 title "Facebook",\
     "../../data/found_cuckoo.data"              using 1:($2/$1)                     with lines       linecolor rgb Ccolor dt 1 title "Libcuckoo",\
     "../../data/found_graveyard-low-load.data"  using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Tcolor  fs transparent solid 0.1 notitle,\
     "../../data/found_google.data"              using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Gcolor  fs transparent solid 0.1 notitle,\
     "../../data/found_facebook.data"            using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "../../data/found_cuckoo.data"              using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Ccolor  fs transparent solid 0.1 notitle,\

set title "Unsuccessful find"
set yrange [0:160]
set output "library-notfound.pdf"
plot "../../data/notfound_graveyard-low-load.data"  using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 1 title "Graveyard low load",\
     "../../data/notfound_graveyard-medium-load.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 2 title "Graveyard medium load",\
     "../../data/notfound_graveyard-high-load.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 3 title "Graveyard high load",\
     "../../data/notfound_graveyard-high-load-no-tombstones.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 4 title "Graveyard high load no tombstones",\
     "../../data/notfound_google.data"              using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 1 title "Google",\
     "../../data/notfound_facebook.data"            using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 1 title "Facebook",\
     "../../data/notfound_cuckoo.data"              using 1:($2/$1)                     with lines       linecolor rgb Ccolor dt 1 title "Libcuckoo",\
     "../../data/notfound_graveyard-low-load.data"  using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Tcolor  fs transparent solid 0.1 notitle,\
     "../../data/notfound_google.data"              using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Gcolor  fs transparent solid 0.1 notitle,\
     "../../data/notfound_facebook.data"           using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "../../data/notfound_cuckoo.data"             using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Ccolor  fs transparent solid 0.1 notitle,\

set title "Memory Consumed"
unset yrange
set ylabel "Memory Bytes"
set title "Memory"
set key top left Left reverse
set output "library-memory.pdf"

f_gl(x) = a_gl * x
fit f_gl(x) "../../data/insert_graveyard-low-load.data" using 1:3 via a_gl

f_gm(x) = a_gm * x
fit f_gm(x) "../../data/insert_graveyard-medium-load.data" using 1:3 via a_gm

f_gh(x) = a_gh * x
fit f_gh(x) "../../data/insert_graveyard-high-load.data" using 1:3 via a_gh

f_goog(x) = a_goog * x
fit f_goog(x) "../../data/insert_google.data" using 1:3 via a_goog

f_face(x) = a_face * x
fit f_face(x) "../../data/insert_facebook.data" using 1:3 via a_face

f_cuck(x) = a_cuck * x
fit f_cuck(x) "../../data/insert_cuckoo.data" using 1:3 via a_cuck

plot "../../data/insert_google.data"                    using 1:3 with lines linecolor rgb Gcolor  title sprintf("Google (%.2f bytes/element)", a_goog), \
     "../../data/insert_graveyard-low-load.data"        using 1:3 with lines linecolor rgb Tcolor  title sprintf("Graveyard low (%.2f bytes/element) (%.1f%%)",    a_gl,  100*(a_gl  -a_goog)/a_goog), \
     "../../data/insert_graveyard-medium-load.data"     using 1:3 with lines linecolor rgb Tcolor  title sprintf("Graveyard medium (%.2f bytes/element) (%.1f%%)", a_gm,  100*(a_gm  -a_goog)/a_goog), \
     "../../data/insert_graveyard-high-load.data"       using 1:3 with lines linecolor rgb Tcolor  title sprintf("Graveyard high (%.2f bytes/element) (%.1f%%)",   a_gh,  100*(a_gh  -a_goog)/a_goog), \
     "../../data/insert_facebook.data"                  using 1:3 with lines linecolor rgb FBcolor title sprintf("Facebook (%.2f bytes/element) (%.1f%%)",         a_face,100*(a_face-a_goog)/a_goog), \
     "../../data/insert_cuckoo.data"                    using 1:3 with lines linecolor rgb Ccolor      title sprintf("Libcuckoo (%.2f bytes/element) (%.1f%%)",        a_cuck,100*(a_cuck-a_goog)/a_goog), \
     a_goog * x linecolor rgb Gcolor      with dots notitle, \
     a_gl   * x linecolor rgb Tcolor      with dots notitle, \
     a_gm   * x linecolor rgb Tcolor      with dots notitle, \
     a_gh   * x linecolor rgb Tcolor      with dots notitle, \
     a_face * x linecolor rgb FBcolor     with dots notitle, \
     a_cuck * x linecolor rgb Ccolor      with dots notitle
