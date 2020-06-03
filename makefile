OBJS	= diseaseAggregator.o worker.o list.o
OUT	= diseaseAggregator worker

OBJS0	= diseaseAggregator.o
SOURCE0	= diseaseAggregator.c
HEADER0	= 
OUT0	= diseaseAggregator

OBJS1	= worker.o list.o
SOURCE1	= worker.c list.c
HEADER1	= 
OUT1	= worker

CC	 = gcc
FLAGS	 = -g -c -Wall
LFLAGS	 = 

all: diseaseAggregator worker

diseaseAggregator: $(OBJS0) $(LFLAGS)
	$(CC) -g $(OBJS0) -o $(OUT0)

worker: $(OBJS1) $(LFLAGS)
	$(CC) -g $(OBJS1) -o $(OUT1)

diseaseAggregator.o: diseaseAggregator.c
	$(CC) $(FLAGS) diseaseAggregator.c

worker.o: worker.c
	$(CC) $(FLAGS) worker.c 

list.o: list.c
	$(CC) $(FLAGS) list.c 


clean:
	rm -f $(OBJS) $(OUT)
