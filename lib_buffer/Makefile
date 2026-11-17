LIB_EST=libbuf.a
OBJS_LIB= buffer.o

FLAGS= -O2 -Wall -MMD
INCLUDE= -I. -I../include

CC = gcc
CFLAGS= $(FLAGS) $(INCLUDE)

all: $(LIB_EST)

$(LIB_EST) : $(OBJS_LIB)
	ar rcs $@ $^ 

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $< 

clean: 
	rm -f *~ *.bak *.o *.d

distclean:
	rm -f $(LIB_EST)