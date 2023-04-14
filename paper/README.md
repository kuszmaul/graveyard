# The Graveyard Paper

This README is effectively acting as the TO-DO list for putting together the paper.

## To Do

- [x] Insertion-only workload up to 99.5% load factor.  Look at what
    happens to the probe lengths just before the rehash.  For both
    Graveyard and Query-Optimized Traditional.

    Experiment: Load a table up.  (Say to 90%).  Rehash it with
    graveyard tombstones, where the number of graveyard tombstones is
    half of the remaining space (say 5%).  Then insert items until you
    may have used up half the graveyard tombstones (say 2.5%).  That's
    when you would rehash.  Measure the probe lengths.

    Result: Yes.

- [ ] Insertion-only workload where we try to get away with fewer
    graveyard tombstones.  Graveyard tombstones increase the query
    probe lengths (for example, we see a 5x increase compared to
    $(X+1)/2$.  Can we use fewer graveyard tombstones without hurting
    insert too much?

    Result: Not much: If we try reducing the graveyard tombstones by
    even 20%, it doubles (?) the find-empty probe length.

## Story Issues

- [ ] The search-distance hack is also applicable to more traditional
    linear probing.  Knuth's analysis assumes that within a run, the
    hashed values are sorted by the hash.  Then you can quit a search
    early when you hit a hash that's too big.  Unfortunately that
    requires computing the hash of each item in the run to determine
    when you are done.  Without that, you end up searching to the end
    of the run which takes $O(X^2)$ probes.  What we do, is for each
    bucket, maintain a search-distance.  (Better name for this?)  This
    gives us an upper bound to the number of buckets that we need to
    examine.  Whenever we insert, we update the search distance (but
    we don't do anything to the search distance during an erase, so
    the search distance will sometimes become larger than neccessary.
    This idea could also be applied to other tables, such as
    Facebook's table (which comes with buckets).  It would be harder
    to figure out how to apply it to the Google table, since they
    don't have buckets *per se*.

     I'm calling it query-optimized linear probing.

- [ ] Our table is also space-optimized.  It scales gracefully to very
    high load factors, but it doesn't hurt anything at low load factors.
   

## Random Notes


----
Argument: for hovering, you don't really need graveyard. For growing you do.

Theory:
Analysis of google vs facebook for chunking.
Analysis of negative queries since we aren't ordering.

---
At a very high load factor for hovering, how does standard tombstones work; how does graveyard work; and no tombstones at all (move things when you delete something).
---
to demo tombstones: at high load factor
 1) With hovering:  Compare graveyard to normal tombstones to ordered-linear-probing-ish (bucketed)
 2) Insertion only: Compare graveyard to not-graveyard

Another experiments: What if you aren't doing bucketed linear probing: Just normal linear probing.  Two graphs:
 1) The spike for hovering workloads for normal linear probing with tombstones.   90%, 95%, 98% full.
 2) With graveyard hashing (pick parameters), not-bucketed.  A bunch of load factors from 90% to 99% full.  Average insertion probe length.  Compare to Knuthian.   Insertion-only workload.  If we had time, maybe you don't need that many tombstones.

4 contributions
 1) A principled stance about the right way to implement open-addressed hash table.
 2)  Make this table work with theoretical techinques that scale gracefully to very high load factors.
 3)  Vanilla presentation of how the tombstone techiques scale.
 4) Some new theoretical analyses related to our implementation of linear probing. 
--------------------

What could the theory be.

one cool interaction between graveyard and the way I implement negative queries.  Suppose you have an insert-only workload at a very high load factor.
Compare graveyard to not:
 No graveyard: If you don't have graveyard then any recently inserted items will have x^2 offset and negative queries will cost x^2.  And expected time is x^2.  (Without ordering- so it's because you have to find an empty slot)  
 Yes graveyard with search-distance: Add one more modification: reorder during rebuild.  Queries are O(x) for failed and successful.   After rehash there are no big search distances, and insertions between rehashes don't add big offsets because they find tombstones.

Kind of interesting: traditionally you hvae to reorder everything all the time and compute all the rehashes to determine when to stop.  But with graveyard you don't have to.

TODO: Put the partial sort back in (based on the source bucket).

Table where we rebuild duirng hash.  With-machine-words constant time lookup, O(X) insert all from the rehash.
It isn't really neccessary to do the ordering during rebuild.

Implementation: A little reordering.
Experiment: How much does a little reordering work.

Need a name that captures the search_distance counters (queries do vectorization, and no rehashing to determine when you are at the end of search). Gives you the ordering-performance without doing rehashing.
 query-optimized graveyard hashing
   all the perqs of ordered hashing without the perceived overheads
  Don't do ordering live, only during rebuild.  Queries just use the counter.
  Tombstones help not just insertions but they make the queries fast too.
Deemphasize vectorization (everyone does it, even theory)

Experiment: 
 on moderate load factor it's good
 some timed experiments at a high load factor showing that positive queries, negative queries, and insertions all scale gracefully at high load factors.

Note: it's a library hash table, full generality, so that it could be a viable plugin for the google or facebook tables (which in turn are a viable plugin for the standard library).

Even the resizing trick is almost good enough for a paper.  Be sure to emphasize it.
