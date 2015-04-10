CC = gcc

LIBS = -lpthread\
	/home/jun/Documents/unpv13e/libunp.a\

FLAGS = -g -O2

CFLAGS = ${FLAGS} -I/home/jun/Documents/unpv13e/lib\


OBJ = routing_table.o odr_func.o rrep.o rreq.o msg_send.o msg_recv.o time_cli.o time_serv.o name2addr.o addr2name.o get_hw_addrs.o

all: odr_process odr_send odr_recv client server ${OBJ} 

odr_process: odr_process.o ${OBJ}
	${CC} ${FLAGS} -o odr_process odr_process.o ${OBJ} ${LIBS}

odr_send: odr_send.o ${OBJ}
	${CC} ${FLAGS} -o odr_send odr_send.o ${OBJ} ${LIBS}

odr_recv: odr_recv.o ${OBJ}
	${CC} ${FLAGS} -o odr_recv odr_recv.o ${OBJ} ${LIBS}

server: server.o ${OBJ}
	${CC} ${FLAGS} -o server server.o ${OBJ} ${LIBS}

client: client.o ${OBJ}
	${CC} ${FLAGS} -o client client.o ${OBJ} ${LIBS}

%.o: %.c
	${CC} ${CFLAGS} -c $^

clean:
	rm odr_process odr_send odr_recv client server *.o *~
