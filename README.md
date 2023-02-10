# Graveyard Hashing

This is an implementation of Graveyard Hashing.

Michael A. Bender, Bradley C. Kuszmaul, and William Kuszmaul.  "Linear Probing
Revisted: Tombstones Mark the Death of Primary Clustering".  July 2021, 2021
*IEEE 62nd Annual Symposium on Foundations of Computer Science (FOCS)*.

The big question is whether Graveyard hashing can compete with industrial-grade
hash tables such as Google's Abseil or Facebook's F14.

## Benchmarks for unordered set of `uint64_t`.

`OLP` is simple linear probing running at between $3/4$ and $7/8$ load
factor.  This table has no vector instructions.  It uses `UINT64_MAX` as a
not-present sentinal (and special cases the case of `UINT64_MAX` by storing a
bit in the header.

`Google` is `absl::flat_hash_set`.

`Facebook` is `folly::F14FastSet`.

`idhash` means to use the identity function as the hash.  The numbers are
already from `std::uniform_int_distribution`, so we shouldn't need to hash them
further.  But there are some strange results, so TODO: See if the absl random
number generators produce better graphs.

In these graphs, the shaded regions are the 95% confidence interval (two
standard deviations).

Insertions for `OLP` are the slowest, and `Facebook` is slow too.

![Insertion time](plots/insert-time.svg)

Reserve closes the gap for `OLP`, showing that the performance difference is
caused by the OLP doing more rehashes.  The cost of the hash function shows up
here, where `Google` and `Google-idhash` have difference performance since the
the `idhash` code doesn't have to call the hash function. `Facebook` doesn't seem
to gain much from reserve.

`Facebook` has some weird jitter in the range of about 1.8e6 to 3.1e6.  It seems to
be repeatable.

![Insertion With Reserve time](plots/reserved-insert-time.svg)

Find is about the same.  It's surprising that the vector instructions in
`Google` don't seem to matter much.  `Facebook` is the fasted for large tables,
likely because of its strategy to have only one cache miss in most cases.

![Successful find time](plots/found-time.svg)

For unsucessful find, here the `Google` vector instructions seem to help.
`Facebook` is slow in this case: probably because it needs one cache miss to
process 14 elements, whereas `Google` can process an average of 32 elements in
the first cache miss.

![Unsuccessful find time](plots/notfound-time.svg)

On average `OLP` saves about 36% memory.  The curve fit is a linear fit
minimizign the sum of the squares of the differences.  `Facebook` uses the same
amount of memory as `Google`: they are both restricted to powers of two.

![Memory](plots/memory.svg)

To produce these plots:
```shell
$ bazel run -c opt hash_tables_benchmark
$ gnuplot makeplots.gnuplot
```

## Coding Style

Follow the Google style guide.

Keep code formatted with `clang-format`.  E.g.,
```shell
$ clang-format -i *.h *.cc
```

Keep includes clean with iwyu:

```shell
$ for x in *.h *.cc; do include-what-you-use -Xiwyu --no_fwd_decls -x c++ -std=c++17 -I/home/bradleybear/.cache/bazel/_bazel_bradleybear/06d43ecbdaa4800474a92f4f59e8b2b3/external/com_google_absl/ -I/home/bradleybear/github/folly $x; done
```

## TODO

*   Why are the idhash versions slower (e.g., for OLP)  Is it a bad random number generator?

*   Where is that jitter comming from  in facebook?

*   For Tombstone:
    *   Change from std::vector to a malloced, aligned memory allocation.
    *   Vectorize
    *   Put as much metadata as possible into the malloced part (but not the
        `logical_bucket_count_` which is in the critical path for `find`.
    *   Increase the size of `buckets_` to match the actual allocated memory.
        This doesn't seem to make any difference for libc malloc, but it
        probably makes a difference for a bucketed malloc such as tcmalloc.
