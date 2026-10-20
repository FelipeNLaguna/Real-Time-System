TARGET=main
SRCS=$(TARGET).c 

FLAGS= -O2 -Wall -MMD
INCLUDE= -I. -I../include
LIBDIR= -L../lib
LIBS=-lrobo -lm

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