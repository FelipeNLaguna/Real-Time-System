LIB_EST=librealtime.a
OBJS_LIB= simulator.o sampling.o gen_u.o logger.o display.o timespec_utils.o

FLAGS= -O2 -Wall -MMD -pthread
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