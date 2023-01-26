# Graveyard Hashing

This is an implementation of Graveyard Hashing.

Michael A. Bender, Bradley C. Kuszmaul, and William Kuszmaul.  "Linear Probing
Revisted: Tombstones Mark the Death of Primary Clustering".  July 2021, 2021
*IEEE 62nd Annual Symposium on Foundations of Computer Science (FOCS)*.

The big question is whether Graveyard hashing can compete with industrial-grade
hash tables such as Abseil or F11.

## Benchmark

```shell
bazel run -c opt hash_tables_benchmark
```

## Benchmark Results
### insert

|Size|flatset Time/op|SimpleILP Time/op|SimpleILP Time Change|flatset Memory|SimpleILP Memory|SimpleILP Memory Change|
|---:|------:|---:|---:|---:|---:|---:|
|55000|16.6±0.2ns|92.6±1.9ns|+457.5%|590K|522K|-11.4%|
|60000|22.6±0.3ns|96.9±1.0ns|+328.9%|1180K|609K|-48.3%|
|65000|22.0±0.1ns|91.6±0.5ns|+317.1%|1180K|609K|-48.3%|
|70000|21.1±1.9ns|97.7±1.8ns|+363.4%|1180K|711K|-39.7%|
|75000|20.1±0.1ns|93.0±0.8ns|+362.0%|1180K|711K|-39.7%|
|80000|19.3±0.0ns|99.7±1.2ns|+416.7%|1180K|829K|-29.7%|
|85000|18.8±0.1ns|95.3±0.4ns|+408.1%|1180K|829K|-29.7%|
|90000|18.0±1.2ns|90.9±0.4ns|+404.0%|1180K|829K|-29.7%|
|95000|17.5±0.0ns|99.3±5.9ns|+468.7%|1180K|968K|-18.0%|
|100000|17.4±1.7ns|95.3±2.1ns|+447.1%|1180K|968K|-18.0%|

### contains-found

|Size|flatset Time/op|SimpleILP Time/op|SimpleILP Time Change|flatset Memory|SimpleILP Memory|SimpleILP Memory Change|
|---:|------:|---:|---:|---:|---:|---:|
|55000|4.7±0.5ns|16.5±0.5ns|+250.3%|590K|522K|-11.4%|
|60000|4.8±0.6ns|14.4±1.1ns|+201.7%|1180K|609K|-48.3%|
|65000|4.7±0.3ns|16.0±1.3ns|+240.2%|1180K|609K|-48.3%|
|70000|4.9±0.5ns|14.4±1.0ns|+193.3%|1180K|711K|-39.7%|
|75000|4.7±0.2ns|16.4±1.1ns|+246.6%|1180K|711K|-39.7%|
|80000|4.7±0.1ns|14.1±0.8ns|+198.9%|1180K|829K|-29.7%|
|85000|4.8±0.3ns|15.3±0.9ns|+219.9%|1180K|829K|-29.7%|
|90000|4.9±0.4ns|17.3±1.4ns|+254.5%|1180K|829K|-29.7%|
|95000|4.9±0.4ns|14.4±0.7ns|+193.4%|1180K|968K|-18.0%|
|100000|4.9±0.3ns|16.1±1.0ns|+227.8%|1180K|968K|-18.0%|

### contains-not-found

|Size|flatset Time/op|SimpleILP Time/op|SimpleILP Time Change|flatset Memory|SimpleILP Memory|SimpleILP Memory Change|
|---:|------:|---:|---:|---:|---:|---:|
|55000|10.9±0.2ns|15.3±0.9ns|+40.4%|590K|522K|-11.4%|
|60000|3.4±0.4ns|14.0±0.9ns|+316.2%|1180K|609K|-48.3%|
|65000|3.4±0.1ns|15.4±0.3ns|+352.1%|1180K|609K|-48.3%|
|70000|3.6±0.3ns|14.5±2.3ns|+303.6%|1180K|711K|-39.7%|
|75000|3.7±0.3ns|15.6±1.1ns|+320.5%|1180K|711K|-39.7%|
|80000|3.9±0.2ns|13.9±0.5ns|+255.3%|1180K|829K|-29.7%|
|85000|4.3±0.4ns|14.9±0.6ns|+244.3%|1180K|829K|-29.7%|
|90000|4.9±0.3ns|16.1±0.6ns|+229.6%|1180K|829K|-29.7%|
|95000|5.6±0.2ns|14.1±0.3ns|+154.7%|1180K|968K|-18.0%|
|100000|6.9±1.1ns|15.1±0.3ns|+119.8%|1180K|968K|-18.0%|

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
