SRCS 	= bpreceiver.c bpsender.c bpchat.c
CC 		= gcc

EXECS   = ${SRCS:%.c=%}

all: $(EXECS)

%: %.c
	$(CC) -o $@ $< -lici -lbp -pthread

clean:
	$(RM) -f $(EXECS)

.PHONY:  clean
