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

|Implementation|Operation|Table size|Mean Time|±   |Memory Utilization|
|--------------|---------|---------:|--------:|---:|-----------------:|
|SimpleIntegerLinearProbing|contains(false)|550000|17.7484ns|±4.72292ns/op|83.4414%|
|flat_hash_set|contains(false)|550000|6.17088ns|±3.22623ns/op|46.6241%|
|SimpleIntegerLinearProbing|contains(false)|600000|16.295ns|±3.92106ns/op|78.0235%|
|flat_hash_set|contains(false)|600000|6.55476ns|±3.34235ns/op|50.8627%|
|SimpleIntegerLinearProbing|contains(false)|650000|19.5707ns|±5.56598ns/op|84.5255%|
|flat_hash_set|contains(false)|650000|12.4314ns|±2.20955ns/op|55.1012%|
|SimpleIntegerLinearProbing|contains(false)|700000|17.8549ns|±3.97794ns/op|78.0239%|
|flat_hash_set|contains(false)|700000|9.59747ns|±2.67409ns/op|59.3398%|
|SimpleIntegerLinearProbing|contains(false)|750000|23.2542ns|±4.44143ns/op|83.597%|
|flat_hash_set|contains(false)|750000|9.37124ns|±4.42897ns/op|63.5783%|
|SimpleIntegerLinearProbing|contains(false)|800000|24.9962ns|±6.52282ns/op|76.4319%|
|flat_hash_set|contains(false)|800000|12.5878ns|±5.20171ns/op|67.8169%|
|SimpleIntegerLinearProbing|contains(false)|850000|21.2787ns|±3.55503ns/op|81.2088%|
|flat_hash_set|contains(false)|850000|13.4403ns|±3.46444ns/op|72.0555%|
|SimpleIntegerLinearProbing|contains(false)|900000|25.7964ns|±2.33003ns/op|85.9858%|
|flat_hash_set|contains(false)|900000|17.2359ns|±2.86555ns/op|76.294%|
|SimpleIntegerLinearProbing|contains(false)|950000|28.4565ns|±5.47884ns/op|77.797%|
|flat_hash_set|contains(false)|950000|8.43128ns|±1.91345ns/op|40.2663%|
|SimpleIntegerLinearProbing|contains(false)|1000000|27.3509ns|±2.46874ns/op|81.8916%|
|flat_hash_set|contains(false)|1000000|8.39515ns|±2.70608ns/op|42.3855%|
|SimpleIntegerLinearProbing|contains(true)|550000|16.544ns|±0.623392ns/op|83.4414%|
|flat_hash_set|contains(true)|550000|19.8508ns|±2.88644ns/op|46.6241%|
|SimpleIntegerLinearProbing|contains(true)|600000|15.8057ns|±2.11491ns/op|78.0235%|
|flat_hash_set|contains(true)|600000|21.5351ns|±4.21114ns/op|50.8627%|
|SimpleIntegerLinearProbing|contains(true)|650000|18.1944ns|±1.55687ns/op|84.5255%|
|flat_hash_set|contains(true)|650000|21.2974ns|±3.61431ns/op|55.1012%|
|SimpleIntegerLinearProbing|contains(true)|700000|16.9602ns|±0.896303ns/op|78.0239%|
|flat_hash_set|contains(true)|700000|20.6158ns|±2.2804ns/op|59.3398%|
|SimpleIntegerLinearProbing|contains(true)|750000|22.3348ns|±2.7335ns/op|83.597%|
|flat_hash_set|contains(true)|750000|20.9617ns|±3.20003ns/op|63.5783%|
|SimpleIntegerLinearProbing|contains(true)|800000|26.6232ns|±9.18883ns/op|76.4319%|
|flat_hash_set|contains(true)|800000|22.1237ns|±4.12824ns/op|67.8169%|
|SimpleIntegerLinearProbing|contains(true)|850000|21.5221ns|±1.38711ns/op|81.2088%|
|flat_hash_set|contains(true)|850000|21.469ns|±0.776895ns/op|72.0555%|
|SimpleIntegerLinearProbing|contains(true)|900000|26.0229ns|±1.38216ns/op|85.9858%|
|flat_hash_set|contains(true)|900000|20.6546ns|±1.47281ns/op|76.294%|
|SimpleIntegerLinearProbing|contains(true)|950000|29.0581ns|±4.41806ns/op|77.797%|
|flat_hash_set|contains(true)|950000|28.6814ns|±1.85448ns/op|40.2663%|
|SimpleIntegerLinearProbing|contains(true)|1000000|29.1856ns|±2.20744ns/op|81.8916%|
|flat_hash_set|contains(true)|1000000|28.8436ns|±2.0069ns/op|42.3855%|
|SimpleIntegerLinearProbing|insert|550000|98.3914ns|±1.69091ns/op|83.4414%|
|flat_hash_set|insert|550000|25.9141ns|±0.971014ns/op|46.6241%|
|SimpleIntegerLinearProbing|insert|600000|103.416ns|±1.86869ns/op|78.0235%|
|flat_hash_set|insert|600000|25.5773ns|±0.118736ns/op|50.8627%|
|SimpleIntegerLinearProbing|insert|650000|99.2379ns|±3.68472ns/op|84.5255%|
|flat_hash_set|insert|650000|24.7071ns|±0.303122ns/op|55.1012%|
|SimpleIntegerLinearProbing|insert|700000|104.806ns|±2.27341ns/op|78.0239%|
|flat_hash_set|insert|700000|24.0366ns|±0.992052ns/op|59.3398%|
|SimpleIntegerLinearProbing|insert|750000|100.203ns|±1.55311ns/op|83.597%|
|flat_hash_set|insert|750000|23.9005ns|±0.698539ns/op|63.5783%|
|SimpleIntegerLinearProbing|insert|800000|116.868ns|±24.6646ns/op|76.4319%|
|flat_hash_set|insert|800000|26.7149ns|±5.11104ns/op|67.8169%|
|SimpleIntegerLinearProbing|insert|850000|110.953ns|±11.2295ns/op|81.2088%|
|flat_hash_set|insert|850000|26.3564ns|±7.94875ns/op|72.0555%|
|SimpleIntegerLinearProbing|insert|900000|102.464ns|±9.1834ns/op|85.9858%|
|flat_hash_set|insert|900000|23.1841ns|±0.206381ns/op|76.294%|
|SimpleIntegerLinearProbing|insert|950000|112.918ns|±10.3573ns/op|77.797%|
|flat_hash_set|insert|950000|29.3089ns|±2.12847ns/op|40.2663%|
|SimpleIntegerLinearProbing|insert|1000000|108.113ns|±5.07784ns/op|81.8916%|
|flat_hash_set|insert|1000000|30.1137ns|±0.777363ns/op|42.3855%|
|nop|nop|550000|0.474902ns|±0.0369645ns/op|inf%|
|nop|nop|550000|0.238978ns|±0.00223021ns/op|inf%|
|nop|nop|600000|0.475779ns|±0.0147927ns/op|inf%|
|nop|nop|600000|0.239602ns|±0.00771602ns/op|inf%|
|nop|nop|650000|0.479088ns|±0.0111781ns/op|inf%|
|nop|nop|650000|0.365331ns|±0.019696ns/op|inf%|
|nop|nop|700000|0.48206ns|±0.0335858ns/op|inf%|
|nop|nop|700000|0.273909ns|±0.0204799ns/op|inf%|
|nop|nop|750000|0.479791ns|±0.0119773ns/op|inf%|
|nop|nop|750000|0.238918ns|±0.001875ns/op|inf%|
|nop|nop|800000|0.616211ns|±0.0617714ns/op|inf%|
|nop|nop|800000|0.238891ns|±0.00166295ns/op|inf%|
|nop|nop|850000|0.529353ns|±0.111717ns/op|inf%|
|nop|nop|850000|0.240077ns|±0.00868675ns/op|inf%|
|nop|nop|900000|0.522299ns|±0.0631018ns/op|inf%|
|nop|nop|900000|0.238865ns|±0.00148397ns/op|inf%|
|nop|nop|950000|0.598598ns|±0.0553874ns/op|inf%|
|nop|nop|950000|0.292349ns|±0.0299294ns/op|inf%|
|nop|nop|1000000|0.566504ns|±0.0532927ns/op|inf%|
|nop|nop|1000000|0.2929ns|±0.0187852ns/op|inf%|


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
