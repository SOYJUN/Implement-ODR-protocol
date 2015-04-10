#include "unp.h"
#define PATH_LEN	15

/*the parameter dest_path needs to change into int pnum < port value , sun_path name>*/
ssize_t msg_send(int sockfd, SA* peer_addr, socklen_t plen, char* dest_ipaddr, char *dest_path, char* msg, int flag){
	
	int    				n;
	socklen_t 			len;
	char 				c_flag[2];
	unsigned char 		buffer[MAXLINE];

	if(sprintf(c_flag, "%d", flag) < 0){
		err_sys("[ERROR]: sprintf error");
	}
	bzero(buffer, MAXLINE);
	strcat(buffer, c_flag);
	strcat(buffer, "|");
	strcat(buffer, dest_ipaddr);
	strcat(buffer, "|");
	strcat(buffer, dest_path);
	strcat(buffer, "|");
	strcat(buffer, msg);

	printf("in msg_send:\n");
    printf("%d\n", flag);
    printf("%s\n", dest_ipaddr);
    printf("%s\n", dest_path);
    printf("%s\n", msg);

	len = plen;
	if((n = sendto(sockfd, buffer, strlen(buffer), 0, peer_addr, len)) < 0){
		return (-1);                                                                                                   
	}

	return n;
}
