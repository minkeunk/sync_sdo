TARGET = sync_kasi_sdo

CC = gcc
LD = gcc 

CFLAGS = -D__CYGWIN__
LFLAGS = -lcurl

DEPS = dirs.h list.h sdo_data.h kasi_server.h log.h
OBJ = main.o dirs.o log.o work.o util.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET) : $(OBJ)
	$(LD) -o $@ $^ $(LFLAGS)


clean:
	rm -f $(TARGET) $(OBJ)
