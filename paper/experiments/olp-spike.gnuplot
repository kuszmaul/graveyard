set terminal pdfcairo
set key top left reverse
set xlabel "Insertion Number"
set ylabel "Average probe length"
set title "Insertion workload followed by hover"

set output "spike-1000000-0.950000.pdf"
set title "Tombstone Spike at 95% load factor"
plot "spike-1000000-0.950000.data" using 1:2 with lines title "found", "" using 1:3 with lines title "notfound", "" using 1:4 with lines title "insert"

set output "spike-1000000-0.980000.pdf"
set title "Tombstone Spike at 98% load factor"
plot "spike-1000000-0.980000.data" using 1:2 with lines title "found", "" using 1:3 with lines title "notfound", "" using 1:4 with lines title "insert"

set output "spike-1000000-0.990000.pdf"
set title "Tombstone Spike at 99% load factor"
plot "spike-1000000-0.990000.data" using 1:2 with lines title "found", "" using 1:3 with lines title "notfound", "" using 1:4 with lines title "insert"

# found and notfound are the same, so just call them query
set output "spike-1000000-0.995000.pdf"
set title "Tombstone Spike at 99.5% load factor"
set xtics ("0.5e6" 500000, "1e6" 1000000, "1.5e6" 1500000)
plot "spike-1000000-0.995000.data" using 1:2 with lines title "query", "" using 1:4 with lines title "insert"

set log y
set output "spike-1000000-0.995000-log.pdf"
set title "Tombstone Spike at 99.5% load factor"
plot "spike-1000000-0.995000.data" using 1:2 with lines title "query", "" using 1:4 with lines title "insert"
