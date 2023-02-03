# Graveyard Hashing

This is an implementation of Graveyard Hashing.

Michael A. Bender, Bradley C. Kuszmaul, and William Kuszmaul.  "Linear Probing
Revisted: Tombstones Mark the Death of Primary Clustering".  July 2021, 2021
*IEEE 62nd Annual Symposium on Foundations of Computer Science (FOCS)*.

The big question is whether Graveyard hashing can compete with industrial-grade
hash tables such as Abseil or F11.

## Benchmarks for unordered set of `uint64_t`.

`SimpleILP` is simple linear probing running at between $3/4$ and $7/8$ load
factor.  This table has no vector instructions.  It uses `UINT64_MAX` as a
not-present sentinal (and special cases the case of `UINT64_MAX` by storing a
bit in the header.

`flatset` is `absl::flat_hash_set`.

Insertions for `SimpleILP` are quite a bit slower.

![Insertion time](/plots/insert-time.svg)

Reserve closes the gap, showing that the performance difference is caused by the
SimpleILP doing more rehashes.  The cost of the hash function shows up here,
where `flatset` and `flatset-nohash` have difference performance sine the the
nohash code doesn't have to call the hash function.

![Insertion With Reserve time](/plots/reserved-insert-time.svg)

Find is about the same.  It's surprising that the vector instructions in
`flatset` don't seem to matter much.

![Successful find time](/plots/found-time.svg)

For unsucessful find, here the `flatset` vector instructions seem to help.

![Unsuccessful find time](/plots/notfound-time.svg)

On average `SimpleILP` saves about 36% memory.  The curve fit is a linear fit
minimizign the sum of the squares of the differences.

![Memory](/plots/memory.svg)

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
$ for x in *.h *.cc; do include-what-you-use -x c++ -std=c++17 -I/home/bradleybear/.cache/bazel/_bazel_bradleybear/06d43ecbdaa4800474a92f4f59e8b2b3/external/com_google_absl/ $x; done
```

#### Folly install notes

python3 ./build/fbcode_builder/getdeps.py --allow-system-packages build --install-prefix ~/folly2
(cd /usr/include;sudo ln -s ~/folly2/folly/include/folly)

python3 ./build/fbcode_builder/getdeps.py --allow-system-packages build --install-prefix=

----
Try again 2023-02-02 15:55:13

sudo python3 ./build/fbcode_builder/getdeps.py --allow-system-packages build --install-prefix /usr/local

folly is a pain...

----
