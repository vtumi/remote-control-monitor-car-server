BIN = server

CC = gcc

objects = server.o jpegutil.o

server:$(objects)
	$(CC) -o $(BIN) server.o jpegutil.o -lwiringPi -lpthread -lm -lopencv_core -lopencv_ml -lopencv_imgproc -lopencv_highgui -ljpeg

server.o:server.c jpegutil.h
	$(CC) -c server.c

jpegutil.o: jpegutil.c jpegutil.h
	$(CC) -c jpegutil.c

clean:
	rm -rf $(objects) $(BIN)