all=kydevmonit
CC=gcc

INTERLAYER_DIR=./src/interlayer/
DBUSLAYER_DIR=./src/dbuslayer/

INCLUDE=-I$(INTERLAYER_DIR) -I$(DBUSLAYER_DIR) -I/usr/include/libxml2/ `pkg-config --cflags glib-2.0 gobject-2.0 dbus-glib-1 dbus-1` 
LIBS=`pkg-config --libs glib-2.0 gobject-2.0 dbus-glib-1 dbus-1`  -lxml2 -ldl

SRCS=./src/main.c $(DBUSLAYER_DIR)monitor_dbus.c $(DBUSLAYER_DIR)marshal.c $(INTERLAYER_DIR)interlayer.c
OBJS=$(SRCS:.c=.o)

kydevmonit:$(OBJS)
	$(CC) -g -O0 -o $@ $^ $(INCLUDE) $(LIBS)
install:
	install kydevmonit /usr/bin/
	cp kydevmonit.service /lib/systemd/system/
	cp com.jd.test.conf /etc/dbus-1/system.d/
	systemctl daemon-reload
clean:
	rm -rf $(OBJS) kydevmonit 

%.o:%.c
	$(CC) $(INCLUDE) -o $@ -c $<
