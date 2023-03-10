set datafile separator comma
set terminal svg
set key top left reverse
set xlabel "Final size"
set ylabel "Time per operation (ns)"
set yrange [0:600]

FBcolor = "#4267B2"
Gcolor = "#0F9D58"
Kcolor = "#800080"
Tcolor = "#00FFFF"

set output "plots/insert-time.svg"
current_time = strftime("%a %b %d %H:%M:%S %Y", time(0))
set title "Insertion (from empty, no reserve) ".current_time
plot "data/insert_google.data"           using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 1 title "Google",\
     "data/insert_google-idhash.data"    using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 2 title "Google-idhash",\
     "data/insert_OLP.data"              using 1:($2/$1)                     with lines       linecolor rgb Kcolor  dt 1 title "OLP",\
     "data/insert_OLP-idhash.data"       using 1:($2/$1)                     with lines       linecolor rgb Kcolor  dt 2 title "OLP-idhash",\
     "data/insert_facebook.data"         using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 1 title "Facebook",\
     "data/insert_facebook-idhash.data"  using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 2 title "Facebook-idhash",\
     "data/insert_graveyard.data"        using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 1 title "Graveyard",\
     "data/insert_graveyard-idhash.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 2 title "Graveyard-idhash",\
     "data/insert_google.data"           using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Gcolor  fs transparent solid 0.1 notitle,\
     "data/insert_google-idhash.data"    using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Gcolor  fs transparent solid 0.1 notitle,\
     "data/insert_OLP.data"              using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Kcolor  fs transparent solid 0.1 notitle,\
     "data/insert_OLP-idhash.data"       using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Kcolor  fs transparent solid 0.1 notitle,\
     "data/insert_facebook.data"         using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "data/insert_facebook-idhash.data"  using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "data/insert_graveyard.data"        using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Tcolor  fs transparent solid 0.1 notitle,\
     "data/insert_graveyard-idhash.data" using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Tcolor  fs transparent solid 0.1 notitle,\

set output "plots/reserved-insert-time.svg"
set title "Insertion with Reserve ".current_time
plot "data/reserved-insert_google.data"           using 1:($2/$1)                     with lines       linecolor rgb Gcolor dt 1 title "Google",\
     "data/reserved-insert_google-idhash.data"    using 1:($2/$1)                     with lines       linecolor rgb Gcolor dt 2 title "Google-idhash",\
     "data/reserved-insert_OLP.data"              using 1:($2/$1)                     with lines       linecolor rgb Kcolor dt 1 title "OLP",\
     "data/reserved-insert_OLP-idhash.data"       using 1:($2/$1)                     with lines       linecolor rgb Kcolor dt 2 title "OLP-idhash",\
     "data/reserved-insert_facebook.data"         using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 1 title "Facebook",\
     "data/reserved-insert_facebook-idhash.data"  using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 2 title "Facebook-idhash",\
     "data/reserved-insert_graveyard.data"        using 1:($2/$1)                     with lines       linecolor rgb Tcolor dt 1 title "Graveyard",\
     "data/reserved-insert_graveyard-idhash.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor dt 2 title "Graveyard-idhash",\
     "data/reserved-insert_google.data"           using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Gcolor fs transparent solid 0.1 notitle,\
     "data/reserved-insert_google-idhash.data"    using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Gcolor fs transparent solid 0.1 notitle,\
     "data/reserved-insert_OLP.data"              using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Kcolor fs transparent solid 0.1 notitle,\
     "data/reserved-insert_OLP-idhash.data"       using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Kcolor fs transparent solid 0.1 notitle,\
     "data/reserved-insert_facebook.data"         using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "data/reserved-insert_facebook-idhash.data"  using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "data/reserved-insert_graveyard.data"        using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Tcolor fs transparent solid 0.1 notitle,\
     "data/reserved-insert_graveyard-idhash.data" using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Tcolor fs transparent solid 0.1 notitle,\

set yrange [0:110]
set output "plots/found-time.svg"
set title "Successful find ".current_time
plot "data/found_google.data"           using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 1 title "Google",\
     "data/found_google-idhash.data"    using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 2 title "Google-idhash",\
     "data/found_OLP.data"              using 1:($2/$1)                     with lines       linecolor rgb Kcolor  dt 1 title "OLP",\
     "data/found_OLP-idhash.data"       using 1:($2/$1)                     with lines       linecolor rgb Kcolor  dt 2 title "OLP-idhash",\
     "data/found_facebook.data"         using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 1 title "Facebook",\
     "data/found_facebook-idhash.data"  using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 2 title "Facebook-idhash",\
     "data/found_graveyard.data"        using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 1 title "Graveyard",\
     "data/found_graveyard-idhash.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 2 title "Graveyard-idhash",\
     "data/found_google.data"         using 1:($2/$1-$4/$1):($2/$1+$4/$1)   with filledcurve linecolor rgb Gcolor fs transparent solid 0.1 notitle,\
     "data/found_OLP.data"            using 1:($2/$1-$4/$1):($2/$1+$4/$1)   with filledcurve linecolor rgb Kcolor fs transparent solid 0.1 notitle,\
     "data/found_facebook.data"       using 1:($2/$1-$4/$1):($2/$1+$4/$1)   with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "data/found_graveyard.data"      using 1:($2/$1-$4/$1):($2/$1+$4/$1)   with filledcurve linecolor rgb Tcolor fs transparent solid 0.1 notitle,\

set output "plots/notfound-time.svg"
set title "Unsuccessful find ".current_time
plot "data/notfound_google.data"           using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 1 title "Google",\
     "data/notfound_google-idhash.data"    using 1:($2/$1)                     with lines       linecolor rgb Gcolor  dt 2 title "Google-idhash",\
     "data/notfound_OLP.data"              using 1:($2/$1)                     with lines       linecolor rgb Kcolor  dt 1 title "OLP",\
     "data/notfound_OLP-idhash.data"       using 1:($2/$1)                     with lines       linecolor rgb Kcolor  dt 2 title "OLP-idhash",\
     "data/notfound_facebook.data"         using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 1 title "Facebook",\
     "data/notfound_facebook-idhash.data"  using 1:($2/$1)                     with lines       linecolor rgb FBcolor dt 2 title "Facebook-idhash",\
     "data/notfound_graveyard.data"        using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 1 title "Graveyard",\
     "data/notfound_graveyard-idhash.data" using 1:($2/$1)                     with lines       linecolor rgb Tcolor  dt 2 title "Graveyard-idhash",\
     "data/notfound_google.data"           using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Gcolor  fs transparent solid 0.1 notitle,\
     "data/notfound_OLP.data"              using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Kcolor  fs transparent solid 0.1 notitle,\
     "data/notfound_facebook.data"         using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb FBcolor fs transparent solid 0.1 notitle,\
     "data/notfound_graveyard.data"        using 1:($2/$1-$4/$1):($2/$1+$4/$1) with filledcurve linecolor rgb Tcolor  fs transparent solid 0.1 notitle,\



set output "plots/memory.svg"
unset yrange
set ylabel "Memory bytes"
set title "Memory ".current_time
set key top left Left reverse

f(x) = a * x
fit f(x) "data/insert_google.data" using 1:3 via a
g(x) = b * x
fit g(x) "data/insert_OLP.data" using 1:3 via b
h(x) = c * x
fit h(x) "data/insert_facebook.data" using 1:3 via c
g(x) = d * x
fit g(x) "data/insert_graveyard.data" using 1:3 via d

plot "data/insert_google.data"    using 1:3 with lines linecolor rgb Gcolor title sprintf("Google (%.2f bytes/element)", a), \
     "data/insert_OLP.data"       using 1:3 with lines linecolor rgb Kcolor title sprintf("OLP (%.2f bytes/element) (%.1f%% less)",b,100*(b-a)/a), \
     "data/insert_facebook.data"  using 1:3 with lines linecolor rgb FBcolor title sprintf("Facebook (%.2f bytes/element) (%.1f%% less)",c,100*(c-a)/a), \
     "data/insert_graveyard.data" using 1:3 with lines linecolor rgb Tcolor title sprintf("Graveyard (%.2f bytes/element) (%.1f%% less)",d,100*(c-a)/a), \
     a * x linecolor rgb Gcolor  with dots notitle, \
     b * x linecolor rgb Kcolor  with dots notitle, \
     c * x linecolor rgb FBcolor with dots notitle, \
     d * x linecolor rgb Tcolor  with dots notitle
