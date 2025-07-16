#DEBUG=-DARM
DEBUG=
Target=main
Object=main.o select.o device.o player.o link.o socket.o 


CC=gcc
#CFLAGS=-I/home/json_arm_install/include $(DEBUG)
#LDFLAGS=-L/home/json_arm_install/lib -ljson

$(Target):$(Object)
	$(CC) $(Object) -o $@ -Wall $(DEBUG) $(CFLAGS) $(LDFLAGS) -L/usr/local/lib -ljson -Wl,-rpath=/usr/local/lib  -static

clean:
	rm *.o main
