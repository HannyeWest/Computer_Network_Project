CC = gcc
OBJ = server_socket.o
TARGET = server_socket

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ)

clean:
	rm -f *.o
	rm -f $(TARGET)
