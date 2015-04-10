#include     "unp.h"

void time_cli(FILE *fp, int sockfd, const SA *pservaddr, socklen_t servlen) {
	
	int    		n;
	socklen_t	len;
	char    	msg[MAXLINE], cliIP[MAXLINE], servIP[MAXLINE];

	while (fgets(msg, MAXLINE, fp) != NULL) {

	if(sendto(sockfd, msg, strlen(msg), 0, pservaddr, servlen) < 0){
		err_sys("[ERROR]: time_cli: sendto error");
	}
	len = sizeof(pservaddr);	
	printf("[INFO]: send request to %s\n", sock_ntop(pservaddr, len));

	n = Recvfrom(sockfd, msg, MAXLINE, 0, NULL, NULL);

	msg[n] = 0;  // null terminate
	Fputs(msg, stdout);
	}
}
