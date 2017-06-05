decode0:
	$(CC) decode0.c -c

seq2mid: decode0
	$(CC) seq2mid.c decode0.o -o seq2mid

.PHONY: clean
clean:
	rm -f *.o seq2mid
