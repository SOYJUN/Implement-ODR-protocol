#include "unp.h"

void time_serv(int sockfd, SA* pcliaddr, socklen_t clilen){

	int		n;
	time_t		t;
	socklen_t	len;
	char		msg[MAXLINE];
	
	while(1){
		len = clilen;
		if((n = recvfrom(sockfd, msg, MAXLINE, 0, pcliaddr, &len)) < 0){
			err_sys("[ERROR]: recvfrom error");
		}

		t = time (NULL);
        	snprintf(msg, sizeof(msg), "%.24s\r\n", ctime(&t));
		if((n = sendto(sockfd, msg, strlen(msg), 0, pcliaddr, len)) < 0){
			err_sys("[ERROR]: sendto error");
		}
	}
}
