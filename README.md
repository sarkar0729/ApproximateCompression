# Approximate compression

For many applications, lossy compression of floating point numbers is acceptable, provided the loss is guaranteed to be within a certain limit (for example 1%) and there is a significant space saving. Most of such applications are concerned with finding statistical properties of a very large set rather than value of a particular member of the set. For example, how many temperature sensors (assuming there are thousands) have risen by more than 2 degrees within last one hour. Most probably, it would be okay if the count is slightly inaccurate because some sensors that rose by 1.999 degrees were included in the total by mistake. Another example is machine learning, where features are sometimes standardized to floating point numbers between 0.0 and 1.0. It is almost guaranteed that you will get same result (prediction) if the dataset is in millions and the floating point values are within half percent of their original values.

This package consists of compression and decompression function, which supports different accuracies and achieve compression of 3X to 30X. Best compression is obtained for time series data, where the difference between successive values are usually less than 5%. It also comes with three utilities - one for compression, one for decompression and one for comparing the two files.

Few sample compressed files are provided. These are real life data, stock prices for some well known companies for 25 years from 1994 January 3rd to 2018 December 31st, for each trading day. The files are named according to their trading symbol.
