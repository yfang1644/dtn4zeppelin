SRCS 	= bpreceiver.c bpsender.c
CC 		= gcc

EXECS   = ${SRCS:%.c=%}

all: $(EXECS)

%: %.c
	$(CC) -o $@ $< -lici -lbp

clean:
	$(RM) -f $(EXECS)

.PHONY:  clean
