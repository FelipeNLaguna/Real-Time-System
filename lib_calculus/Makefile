LIB_EST=librobo.a
OBJS_LIB= derivate.o next_x.o result_y.o

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
