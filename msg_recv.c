#include "unp.h"
#define PATH_LEN	15
#define ODR_DATA_LEN        1410

/*the parameter src_path needs to change into int pnum < port value , sun_path name>*/
ssize_t msg_recv(int sockfd, SA* peer_addr, socklen_t plen, char* dest_ipaddr, char* dest_path, char* msg, int *flag){


	int 				i, n;
	socklen_t 			len;
	unsigned char 		buffer[MAXLINE];
    unsigned char       c_flag[2];
	char 				delims[] = "|";
	char				*result = NULL;

	len = plen;
	if((n = recvfrom(sockfd, buffer, MAXLINE, 0, peer_addr, &len)) < 0){
		return (-1);
	}

    result = strtok(buffer, delims);
    i = 0;
    while(result != NULL){
    	if(i == 0){
            memcpy(c_flag, result, 2);
            result = strtok(NULL, delims);
            *flag = atoi(c_flag);
        }else if(i == 1){
    		memcpy(dest_ipaddr, result, INET_ADDRSTRLEN);
    		result = strtok(NULL, delims);
    	}else if(i == 2){
    		memcpy(dest_path, result, PATH_LEN);
    		result = strtok(NULL, delims);
    	}else if(i == 3){
    		memcpy(msg, result, ODR_DATA_LEN);
    		result = strtok(NULL, delims);
    	}
    	i++;
    }       
    printf("in msg_recv:\n");
    printf("%d\n", flag);
    printf("%s\n", dest_ipaddr);
    printf("%s\n", dest_path);
    printf("%s\n", msg);
	return n;
}
