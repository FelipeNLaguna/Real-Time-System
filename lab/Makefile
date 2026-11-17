TARGET=main
SRCS=$(TARGET).c 

FLAGS= -O2 -Wall -MMD -pthread
INCLUDE= -I. -I../include
LIBDIR= -L../lib_calculus -L../lib_buffer -L../lib_rt_tasks
LIBS=-lrobo -lrealtime -lm  -lbuf -pthread

CC = gcc
CFLAGS= $(FLAGS) $(INCLUDE)
LDFLAGS= $(LIBDIR) $(LIBS)

all: $(TARGET)

$(TARGET) : $(SRCS:.c=.o)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

-include $(SRCS:.c=.d)

clean:
	rm -f *~ *.bak *.o *.d

distclean:
	rm -f $(TARGET)