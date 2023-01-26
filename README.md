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

|Implementation|Operation|Table size|Time/op|Memory Utilization|
|--------------|---------|---------:|------:|-----------------:|
|`SimpleIntegerLinearProbing`|contains(false)|550000|17.8±4.3ns|83.4%|
|`flat_hash_set`|contains(false)|550000|6.2±3.2ns|46.6%|
|`SimpleIntegerLinearProbing`|contains(false)|600000|16.9±4.6ns|78.0%|
|`flat_hash_set`|contains(false)|600000|6.7±3.1ns|50.9%|
|`SimpleIntegerLinearProbing`|contains(false)|650000|18.5±3.5ns|84.5%|
|`flat_hash_set`|contains(false)|650000|7.0±3.4ns|55.1%|
|`SimpleIntegerLinearProbing`|contains(false)|700000|18.5±4.4ns|78.0%|
|`flat_hash_set`|contains(false)|700000|7.1±3.4ns|59.3%|
|`SimpleIntegerLinearProbing`|contains(false)|750000|19.9±3.9ns|83.6%|
|`flat_hash_set`|contains(false)|750000|8.3±3.6ns|63.6%|
|`SimpleIntegerLinearProbing`|contains(false)|800000|20.4±3.4ns|76.4%|
|`flat_hash_set`|contains(false)|800000|10.2±3.8ns|67.8%|
|`SimpleIntegerLinearProbing`|contains(false)|850000|23.0±2.8ns|81.2%|
|`flat_hash_set`|contains(false)|850000|12.4±2.9ns|72.1%|
|`SimpleIntegerLinearProbing`|contains(false)|900000|27.3±6.9ns|86.0%|
|`flat_hash_set`|contains(false)|900000|16.6±2.9ns|76.3%|
|`SimpleIntegerLinearProbing`|contains(false)|950000|24.9±2.0ns|77.8%|
|`flat_hash_set`|contains(false)|950000|7.7±2.4ns|40.3%|
|`SimpleIntegerLinearProbing`|contains(false)|1000000|26.6±1.9ns|81.9%|
|`flat_hash_set`|contains(false)|1000000|8.3±2.1ns|42.4%|
|`SimpleIntegerLinearProbing`|contains(true)|550000|16.6±0.4ns|83.4%|
|`flat_hash_set`|contains(true)|550000|20.2±3.6ns|46.6%|
|`SimpleIntegerLinearProbing`|contains(true)|600000|14.9±0.3ns|78.0%|
|`flat_hash_set`|contains(true)|600000|19.3±2.4ns|50.9%|
|`SimpleIntegerLinearProbing`|contains(true)|650000|17.5±0.5ns|84.5%|
|`flat_hash_set`|contains(true)|650000|20.8±6.0ns|55.1%|
|`SimpleIntegerLinearProbing`|contains(true)|700000|16.6±0.8ns|78.0%|
|`flat_hash_set`|contains(true)|700000|19.6±1.9ns|59.3%|
|`SimpleIntegerLinearProbing`|contains(true)|750000|19.0±0.6ns|83.6%|
|`flat_hash_set`|contains(true)|750000|19.7±1.9ns|63.6%|
|`SimpleIntegerLinearProbing`|contains(true)|800000|19.3±2.0ns|76.4%|
|`flat_hash_set`|contains(true)|800000|20.1±1.8ns|67.8%|
|`SimpleIntegerLinearProbing`|contains(true)|850000|22.7±2.6ns|81.2%|
|`flat_hash_set`|contains(true)|850000|20.5±1.4ns|72.1%|
|`SimpleIntegerLinearProbing`|contains(true)|900000|28.3±3.7ns|86.0%|
|`flat_hash_set`|contains(true)|900000|20.5±1.5ns|76.3%|
|`SimpleIntegerLinearProbing`|contains(true)|950000|24.5±2.0ns|77.8%|
|`flat_hash_set`|contains(true)|950000|28.1±1.1ns|40.3%|
|`SimpleIntegerLinearProbing`|contains(true)|1000000|27.1±1.0ns|81.9%|
|`flat_hash_set`|contains(true)|1000000|28.2±1.3ns|42.4%|
|`SimpleIntegerLinearProbing`|insert|550000|113.2±15.6ns|83.4%|
|`flat_hash_set`|insert|550000|26.3±1.6ns|46.6%|
|`SimpleIntegerLinearProbing`|insert|600000|108.8±3.7ns|78.0%|
|`flat_hash_set`|insert|600000|25.9±0.9ns|50.9%|
|`SimpleIntegerLinearProbing`|insert|650000|103.3±2.9ns|84.5%|
|`flat_hash_set`|insert|650000|25.3±0.7ns|55.1%|
|`SimpleIntegerLinearProbing`|insert|700000|111.6±3.5ns|78.0%|
|`flat_hash_set`|insert|700000|24.6±0.6ns|59.3%|
|`SimpleIntegerLinearProbing`|insert|750000|106.4±2.5ns|83.6%|
|`flat_hash_set`|insert|750000|24.0±0.5ns|63.6%|
|`SimpleIntegerLinearProbing`|insert|800000|114.8±3.3ns|76.4%|
|`flat_hash_set`|insert|800000|24.0±0.6ns|67.8%|
|`SimpleIntegerLinearProbing`|insert|850000|109.3±2.4ns|81.2%|
|`flat_hash_set`|insert|850000|23.6±2.4ns|72.1%|
|`SimpleIntegerLinearProbing`|insert|900000|106.5±6.2ns|86.0%|
|`flat_hash_set`|insert|900000|23.7±2.2ns|76.3%|
|`SimpleIntegerLinearProbing`|insert|950000|112.6±2.6ns|77.8%|
|`flat_hash_set`|insert|950000|29.4±0.4ns|40.3%|
|`SimpleIntegerLinearProbing`|insert|1000000|109.5±3.6ns|81.9%|
|`flat_hash_set`|insert|1000000|29.4±1.2ns|42.4%|

## insert

|Size |flatset Time/op|SimpleILP Time/op|SimpleILP Time Change|flatset Memory|
|----:|--------------:|----------------:|--------------------:|-------------:|
|55000|16.8±0.1ns     |92.5±1.5ns       |+452.0%              |          590K|


|Size |flatset Time/op|SimpleILP Time/op|SimpleILP Time Change||flatset Memory|SimpleILP Memory|SimpleILP Memory Change|
|----:|--------------:|----------------:|--------------------:||-------------:|---------------:|----------------------:|
|55000|16.8±0.1ns     |92.5±1.5ns       |+452.0%              ||          590K|            522K|                 -11.4%|

|Size |flatset Time/op|SimpleILP Time/op|SimpleILP Time Change||flatset Memory|SimpleILP Memory|SimpleILP Memory Change|
|----:|--------------:|----------------:|--------------------:||-------------:|---------------:|----------------------:|
|55000|16.8±0.1ns     |92.5±1.5ns       |+452.0%              ||          590K|            522K|                 -11.4%|
|60000|22.4±0.4ns|96.0±0.4ns|+329.7%||1180K|609K|-48.3%|
|65000|21.9±1.5ns|91.2±1.2ns|+316.9%||1180K|609K|-48.3%|
|70000|20.6±0.0ns|96.7±0.5ns|+370.3%||1180K|711K|-39.7%|
|75000|20.3±0.4ns|92.6±0.9ns|+357.3%||1180K|711K|-39.7%|
|80000|19.0±0.2ns|98.6±0.6ns|+418.5%||1180K|829K|-29.7%|
|85000|19.2±0.0ns|94.7±0.8ns|+394.0%||1180K|829K|-29.7%|
|90000|17.9±0.1ns|91.0±0.7ns|+407.8%||1180K|829K|-29.7%|
|95000|18.7±1.5ns|98.5±3.7ns|+427.9%||1180K|968K|-18.0%|
|100000|17.9±0.2ns|94.7±1.0ns|+429.5%||1180K|968K|-18.0%|

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
