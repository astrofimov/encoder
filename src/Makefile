OUT:=encoder
SRC:=encoder.c wav.c mp3.c
LIBS+=-lmp3lame -lpthread -lm
CFLAGS+=--static
# for build from source lib on windows cygwin:
CFLAGS+=-L/usr/local/lib

all:
	gcc $(CFLAGS) -o $(OUT) $(SRC) $(LIBS)
	strip -s $(OUT)
	./generate_tests.sh

clean:
	rm -f encoder 
	rm -rf test
