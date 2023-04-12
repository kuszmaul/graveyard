This directory contains experiments to support the paper.

1.  Experiment: Show the spike for hovering workloads for non-bucketed
    linear probing with tombstones.  Show it for 90%, 95% and 98%
    full.

2.  Experiment: At high load factors (ranging say from 90% to 99%
    full), insertion-only workload, show the average insertion probe
    length (just before rehashing).  Compare graveyard to Knuthian.


## Experiment 2

This is the experiment to show how an insertion-only workload works,
comparing an implementation that inserts graveyard tombstones, to one
that doesn't.  Here's how to read the data. Look at the first line
which says

```
90% full, no tombstones, add 2.5%(Knuth predicts 7.16667 89.3889):found=6.94481 notfound=84.3546
```

This means that we created a table that was 90% full with no tombstones. Then we added 2.5% more (so the table would be 92.5% full). Then we measured the average probe length for a successful lookup ("found=6.94481"), an insert (84.3546). (An insert is the same as a notfound). In this case, the table is 92.5% full, so X=13.33.

 * Knuth predicts that found will cost (X+1)/2, which is 7.17.  (So 6.94 seems about right).

 * Knuth predicts that notfound will cost (X^2+1)/2. (This is not
   *ordered* linear probing, so we don't get to stop looking
   early). Knuth predicts that notfound would cost 89.39, and the
   measured number is 84.35.

The second line shows what happens if we inserted 5% tombstones at the
rehash. So the table was 90% full with 5% of the tombstones spread
evenly, and 5% empties.

(TODO: Run the experiment 20 times and produce confidence intervals).

(TODO: I need to fix a little bug in the way not-found distances are
calculated. This will increase the not-found cost slightly) Here's the
data (analysis follows):

```shell
$ make insert-only  && ./insert-only
90% full, no tombstones, add 2.5%(Knuth predicts 7.16667 89.3889):found=6.94481 notfound=84.3546
90% full, 5% tombstones, add 2.5%(Knuth predicts 7.16667 89.3889):found=10.6359 notfound=21.4327
94% full, no tombstones, add 2% (Knuth predicts 13 313):found=12.9758 notfound=297.878
94% full, 4% tombstones, add 2% (Knuth predicts 13 313):found=26.0294 notfound=36.2858
96% full, no tombstones, add 1% (Knuth predicts 17.1667 556.056):found=17.2412 notfound=521.558
96% full, 2% tombstones, add 1% (Knuth predicts 17.1667 556.056):found=25.5589 notfound=67.0138
97% full, no tombstones, add 1% (Knuth predicts 25.5 1250.5):found=23.8672 notfound=1041.33
97% full, 2% tombstones, add 1% (Knuth predicts 25.5 1250.5):found=45.9451 notfound=79.585
98% full, no tombstones, add 0.5% (Knuth predicts 33.8333 2222.72):found=36.0105 notfound=3234.19
98% full, 1% tombstones, add 0.5% (Knuth predicts 33.8333 2222.72):found=60.3546 notfound=151.658
98% full, 0.8% tombstones, add 0.5% (Knuth predicts 33.8333 2222.72):found=48.6077 notfound=265.884
98% full, 0.625% tombstones, add 0.5% (Knuth predicts 33.8333 2222.72):found=41.1458 notfound=583.304
98% full, 0.5% tombstones, add 0.5% (Knuth predicts 33.8333 2222.72):found=37.8825 notfound=2085.02
98% full, 0.4% tombstones, add 0.5% (Knuth predicts 33.8333 2222.72):found=37.1492 notfound=2616.25
```

Analysis: At 90% the value of graveyard for inserts is starting to show up, by 94% it's strong and at 98% it' huge.

Also: We really do need to insert 2x as many tombstones as we do inserts.
