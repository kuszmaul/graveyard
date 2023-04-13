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
90% full, no tombstones, add 2.5% (Knuth predicts 7.16667 89.3889):found=7.12803 notfound=5.71245 findempty=89.3774
```

This means that we created a table that was 90% full with no
tombstones. Then we added 2.5% more (so the table would be 92.5%
full). Then we measured the average probe length for a successful
lookup ("found=7.12803"), for an unsuccessful lookup
("notfound=5.71245") and to find an empty slot ("findempty=89.3774").
To do an insert in general you must first do a notfound and then find
an empty slot.  However for an insertion-only workload you don't have
to do the notfound search first, since there won't be any empty slots
before the end of the run.  In this case, the table is 92.5% full, so
X=13.33.

 * Knuth predicts that found will cost $(X+1)/2$, which is 7.17.  (So
   7.12803 seems about right).

 * Knuth preducts that notfound will also cost $(X+1)/2$, which is
   5.71245, which also sounds reasonable. Keep in mind that this is
   not ordered linear probing, which would allow us to stop looking
   early.  Instead, once a value is placed, we leave it there.  But we
   do keep track of the distance that must be searched for every hash,
   so we *do* get to stop looking early.  It's interesting that the
   notfound searches actually cost less than the found searches.
   That's because if a slot has never been used, we don't need to
   probe at all.

 * Knuth predicts that finding an empty slot will cost $(X^2+1)/2$.
   In this cas that's 89.3889, and the simulated value is 89.3774
   which is spookily accurate in this case.  It's less accurate in the
   other cases with no tombstones.  For the case with tombstones,
   Knuth's predictions are shown so we can see how much better
   graveyard hashing does.

The second line shows what happens if we inserted 5% tombstones at the
rehash. So the table was 90% full with 5% of the tombstones spread
evenly, and 5% empties.

(TODO: Run the experiment 20 times and produce confidence intervals).

```shell
$ make insert-only  && ./insert-only
90% full, no tombstones, add 2.5% (Knuth predicts 7.16667 89.3889):found=7.12803 notfound=5.71245 findempty=89.3774
90% full, 5% tombstones, add 2.5%       (Knuth predicts 7.16667 89.3889):found=11.0221 notfound=9.10819 findempty=22.0122
94% full, no tombstones, add 2%         (Knuth predicts 13 313):found=12.9918 notfound=11.307 findempty=297.798
94% full, 4% tombstones, add 2%         (Knuth predicts 13 313):found=25.1302 notfound=22.6016 findempty=36.5604
96% full, no tombstones, add 1%         (Knuth predicts 17.1667 556.056):found=17.2439 notfound=15.4212 findempty=510.938
96% full, 2% tombstones, add 1%         (Knuth predicts 17.1667 556.056):found=25.3078 notfound=23.0235 findempty=68.2908
97% full, no tombstones, add 1%         (Knuth predicts 25.5 1250.5):found=24.6617 notfound=22.6645 findempty=1069.98
97% full, 2% tombstones, add 1%         (Knuth predicts 25.5 1250.5):found=44.0987 notfound=41.3935 findempty=81.2281
98% full, no tombstones, add 0.5%       (Knuth predicts 33.8333 2222.72):found=31.8208 notfound=29.7285 findempty=1706.71
98% full, 1% tombstones, add 0.5%       (Knuth predicts 33.8333 2222.72):found=44.2069 notfound=41.7216 findempty=150.722
98% full, 0.8% tombstones, add 0.5%     (Knuth predicts 33.8333 2222.72):found=38.7417 notfound=36.3969 findempty=272.671
98% full, 0.625% tombstones, add 0.5%   (Knuth predicts 33.8333 2222.72):found=35.1971 notfound=32.9514 findempty=566.481
98% full, 0.5% tombstones, add 0.5%     (Knuth predicts 33.8333 2222.72):found=33.555 notfound=31.363 findempty=997.569
98% full, 0.4% tombstones, add 0.5%     (Knuth predicts 33.8333 2222.72):found=33.042 notfound=30.8714 findempty=1236.05
```

### Analysis

At 90%+2.5%: graveyard hashing costs a little more for lookups (7.12
vs. 11.02 for found; 5.7 vs. 9.1 for notfound).  But the cost of
insertion is about a factor of 4 (89.4 vs 22.0).

At 94%+2%: graveyard queries are costing a factor of two (because
tombstones have halved the amount free space that was available for
the first 94%).  Insertions are improved by a factor of 8.

At a very high load factor (98% + 0.5%): THe queries are 50% slower
and the inserts are 21 times faster.

The last four lines show what happens if we don't put in enough
tombstone.  Normally if we want to add $k$ values before rehasing, we
put in $2k$ grarveyard tombstones.  (Hence when we were adding 2.5%,
we put in 5% tombstones).  The last four lines show what happens if
there are fewer tombstonew: the insert cost grows substantially.  Even
removing 20% of the tombstones makes insertion nearly twice as slow.
