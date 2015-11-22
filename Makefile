TARGET = sync_kasi_sdo_data

CC = gcc
LD = gcc 

CFLAGS = 
LFLAGS = -lcurl

DEPS = dirs.h list.h sdo_data.h kasi_server.h log.h
OBJ = main.o dirs.o log.o work.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET) : $(OBJ)
	$(LD) -o $@ $^ $(LFLAGS)


clean:
	rm -f $(TARGET) $(OBJ)
