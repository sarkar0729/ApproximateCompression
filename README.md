# Approximate compression

For many applications, lossy compression of floating point numbers is acceptable, provided there is a significant space saving, and the loss is guaranteed to be within a certain limit (for example 1%). Most of such applications are concerned with finding statistical properties of a very large set rather than exact values of a particular member of the set. For example, how many temperature sensors (assuming there are thousands) have risen by more than 2 degrees within last one hour. Most probably, it would be okay if the count is slightly inaccurate due to a few sensors that rose by 1.9999 degrees being included in the total and a few sensors that rose by 2.0001 degrees being excluded from the total. Another example is machine learning, where features are sometimes standardized to floating point numbers between 0.0 and 1.0. It is almost guaranteed that you will get same result (prediction) if the dataset is in millions and the floating point values are within half of a percent of their original values.

This package consists of compression and decompression functions, which support different accuracies and achieve compression of 3X to 30X. The best compression is obtained for time series data, where the difference between successive values are usually less than 5%. In addition to the compression and decompression utilities, it also comes with a utility for comparing the two floating point files, the original and the other is decompressed file.

A few sample compressed files are provided. The sample data consists of real-life stock prices for some companies over a span of 25 years, each trading day from January 3 1994 to December 31 2018. The files are named according to their companies' stock ticker symbols.

#### Usage Instructions

First use the Makefile to build the executables.
To compress a file (for example CVX.dat) run:
```
./compressFloat CVX.dat CVX.cz
```

You should see a message that compression was successful and the size before the compression and after compression.

To decompress the generated compressed file CVX.cz, run:
```
./decompressFloat CVX.cz CVX2.dat
```

You should see a message that decompression was successful and the size of the decompressed file. It should match the size of the original file.

You can compare the original file with the decompressed file by typing:
```
./compareFloat CVX.dat CVX2.dat
```
You should see a message like:
```
Average err_percent = 0.482887030%      Maximum err_percent = 0.975002229%
````

This package has been tested on several flavors of Linux, Mac OSX, and Windows. For any issues related to compiling or running these programs, please send an email to support@coreset.in
