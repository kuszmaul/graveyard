set terminal pdf
set key top left reverse
set xlabel "Graveyard factor"

## TODO: Can I make a second scale with X for the x axis?

set ylabel "Average Probe Length"

set log y

# Data:

# 1 Graveyard-factor
# 2 found-mean
# 3 found-sigma
# 4 notfound-mean
# 5 notfound-sigma
# 6 findempty-mean
# 7 findempty-sigma

set output "vary-tombstones.pdf"
set title "Successful Lookup Probe Length"
plot "vary-tombstones.data"\
     using 1:2 with lines title "found",\
  "" using 1:($2-2*$3):($2+2*$3) with filledcurve linecolor rgb "#800000" fs transparent solid 0.1 notitle,\
  "" using 1:4 with lines title "notfound",\
  "" using 1:($4-2*$5):($4+2*$5) with filledcurve linecolor rgb "#800000" fs transparent solid 0.1 notitle,\
  "" using 1:6 with lines title "findempty",\
  "" using 1:($6-2*$7):($6+2*$7) with filledcurve linecolor rgb "#800000" fs transparent solid 0.1 notitle,\

