set terminal pdf
set key top left reverse
set xlabel "Final load factor"

## TODO: Can I make a second scale with X for the x axis?

set ylabel "Average Probe Length"

set log y

# Data:
# 1 initial-fullness
# 2 final-fullness
# 3 X
# 4 (X+1)/2
# 5 (X^2+1)/2
# 6 no-tombstones-found-mean
# 7 no-tombstones-found-sigma
# 8 no-tombstones-notfound-mean
# 9 no-tombstones-notfound-sigma
# 10 no-tombstones-findempty-mean
# 11 no-tombstones-findempty-sigma
# 12 graveyard-found-mean
# 13 graveyard-found-sigma
# 14 graveyard-notfound-mean
# 15 graveyard-notfound-sigma graveyard-findempty-mean graveyard-findempty-sigma

set output "increasing-load-found.pdf"
plot "increasing-load.data"\
     using 2:4 with lines title "(X+1)/2",\
  "" using 2:6 with lines title "No-Tombstones found",\
  "" using 2:($6-2*$7):($6+2*$7) with filledcurve linecolor rgb "#800000" fs transparent solid 0.1 notitle,\
  "" using 2:12 with lines title "Graveyard found",\
  "" using 2:($12-2*$13):($12+2*$13) with filledcurve linecolor rgb "#800000" fs transparent solid 0.1 notitle

set output "increasing-load-notfound.pdf"
plot "increasing-load.data"\
     using 2:4 with lines title "(X+1)/2",\
  "" using 2:8 with lines title "No-Tombstones not-found",\
  "" using 2:($8-2*$9):($8+2*$9) with filledcurve linecolor rgb "#800000" fs transparent solid 0.1 notitle,\
  "" using 2:14 with lines title "Graveyard found",\
  "" using 2:($14-2*$15):($14+2*$15) with filledcurve linecolor rgb "#800000" fs transparent solid 0.1 notitle

set output "increasing-load-findempty.pdf"
plot "increasing-load.data"\
     using 2:4 with lines title "(X+1)/2",\
  "" using 2:4 with lines title "(X+1)/2",\
  "" using 2:10 with lines title "No-Tombstones findempty",\
  "" using 2:($10-2*$11):($10+2*$11) with filledcurve linecolor rgb "#800000" fs transparent solid 0.1 notitle,\
  "" using 2:16 with lines title "Graveyard findempty",\
  "" using 2:($16-2*$17):($16+2*$17) with filledcurve linecolor rgb "#800000" fs transparent solid 0.1 notitle

