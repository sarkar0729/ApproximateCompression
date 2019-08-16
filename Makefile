CC=gcc
CFLAGS=-std=gnu99 -c

compressFloat: compressFloatMain.o approximateCompression.o bitUtils.o bucket.o uint8.o
	$(CC) -o compressFloat compressFloatMain.o approximateCompression.o bitUtils.o bucket.o uint8.o

uncompressFloat: uncompressFloatMain.o approximateCompression.o bitUtils.o bucket.o uint8.o
	$(CC) -o uncompressFloat uncompressFloatMain.o approximateCompression.o bitUtils.o bucket.o uint8.o

compareFloat: compareFloat.o 
	$(CC) -o compareFloat compareFloat.o

compressFloatMain.o: compressFloatMain.c approximateCompression.h
	$(CC) $(CFLAGS) compressFloatMain.c

uncompressFloatMain.o: uncompressFloatMain.c approximateCompression.h
	$(CC) $(CFLAGS) uncompressFloatMain.c

approximateCompression.o: approximateCompression.c approximateCompression.h
	$(CC) $(CFLAGS) approximateCompression.c

compareFloat.o: compareFloat.c 
	$(CC) $(CFLAGS) compareFloat.c

clean:
	rm -f compressFloatMain.o uncompressFloatMain.o approximateCompression.o bitUtils.o bucket.o uint8.o
