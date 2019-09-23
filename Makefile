CC=gcc
CFLAGS=-std=gnu99 -c

all: compressFloat decompressFloat compareFloat compressDouble decompressDouble compareDouble

compressFloat: compressFloatMain.o approximateCompression.o bitUtils.o bucket.o uint8.o
	$(CC) -o compressFloat compressFloatMain.o approximateCompression.o bitUtils.o bucket.o uint8.o

compressDouble: compressDoubleMain.o approximateCompression.o bitUtils.o bucket.o uint8.o
	$(CC) -o compressDouble compressDoubleMain.o approximateCompression.o bitUtils.o bucket.o uint8.o

decompressFloat: decompressFloatMain.o approximateCompression.o bitUtils.o bucket.o uint8.o
	$(CC) -o decompressFloat decompressFloatMain.o approximateCompression.o bitUtils.o bucket.o uint8.o

decompressDouble: decompressDoubleMain.o approximateCompression.o bitUtils.o bucket.o uint8.o
	$(CC) -o decompressDouble decompressDoubleMain.o approximateCompression.o bitUtils.o bucket.o uint8.o

compareFloat: compareFloat.o 
	$(CC) -o compareFloat compareFloat.o

compareDouble: compareDouble.o 
	$(CC) -o compareDouble compareDouble.o

compressFloatMain.o: compressFloatMain.c approximateCompression.h
	$(CC) $(CFLAGS) compressFloatMain.c

compressDoubleMain.o: compressDoubleMain.c approximateCompression.h
	$(CC) $(CFLAGS) compressDoubleMain.c

decompressFloatMain.o: decompressFloatMain.c approximateCompression.h
	$(CC) $(CFLAGS) decompressFloatMain.c

decompressDoubleMain.o: decompressDoubleMain.c approximateCompression.h
	$(CC) $(CFLAGS) decompressDoubleMain.c

approximateCompression.o: approximateCompression.c approximateCompression.h bitUtils.h uint8.h bucket.h
	$(CC) $(CFLAGS) approximateCompression.c

uint8.o: uint8.c bitUtils.h uint8.h
	$(CC) $(CFLAGS) uint8.c

bucket.o: bucket.c bitUtils.h bucket.h bucketArray.h
	$(CC) $(CFLAGS) bucket.c

bitUtils.o: bitUtils.c bitUtils.h
	$(CC) $(CFLAGS) bitUtils.c

compareFloat.o: compareFloat.c 
	$(CC) $(CFLAGS) compareFloat.c

compareDouble.o: compareDouble.c 
	$(CC) $(CFLAGS) compareDouble.c

clean:
	rm -f compressFloatMain.o decompressFloatMain.o compareFloat.o compressDoubleMain.o decompressDoubleMain.o compareDouble.o approximateCompression.o bitUtils.o bucket.o uint8.o

