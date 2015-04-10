#include    "unp.h"
#include "hw_addrs.h"
#include <linux/if_ether.h>

#define MY_UNIXDG_PATH 		"/tmp/myserv.dg"
#define MY_ODRDG_PATH       "/tmp/myodr.dg"
#define PATH_LEN	15
#define ODR_DATA_LEN        1410

int main(int argc, char **argv){

	int 		    	sockfd, i, index, flag, n;
	int 				fd = -1;
	void				*buffer = (void *)malloc(MAXLINE);
    char    			tmpFileName[PATH_LEN] = "";
    char 				dest_ipaddr[INET_ADDRSTRLEN+1];
    char				recv_ipaddr[INET_ADDRSTRLEN+1];
    char    			dest_path[PATH_LEN+1];
    char    			msg[ODR_DATA_LEN];
    char				dest_name[1024];
    char				src_name[1024];
    char				recv_name[1024];
    char 				*ptr;
    FILE				*fp;
    char                host_ipaddr[5][INET_ADDRSTRLEN+1];
    unsigned char       host_mac[5][ETH_ALEN+1];
	struct sockaddr_un 	cliaddr, odraddr;
	struct hwa_info     *hwa, *hwahead;
	struct sockaddr     *sa;
	fd_set 				rset;
	struct timeval		tm;

	//get the host ip addr and mac addr-----------------------------------------------
    index = 0;
    for(hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next){
        if(hwa->if_index > 0){
            if ( (sa = hwa->ip_addr) != NULL){
                strncpy(host_ipaddr[index], Sock_ntop_host(sa, sizeof(*sa)), INET_ADDRSTRLEN);
            }
            ptr = hwa->if_haddr;
            i = 0;
            while(i < 6){
                //output and store the mac addr
                host_mac[index][i] = *ptr++ & 0xff;
                i++;
            }   
            printf("\n\n");   
            index++;
        }
    }

    //set up the socket------------------------------------------------------------
    if((sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0)) < 0){
		err_sys("[ERROR]: Client: socket error.");
	}

    bzero(&cliaddr, sizeof(cliaddr));   /* bind an address for us */
   	cliaddr.sun_family = AF_LOCAL;
	memset(tmpFileName, 0, sizeof(tmpFileName));
	strncpy(tmpFileName, "/tmp/un.XXXXXX", sizeof(tmpFileName));
	if((fd = mkstemp(tmpFileName)) == -1){
		printf("%s: %s\n", tmpFileName, strerror(errno));
		err_sys("[ERROR]: mkstemp error.");

	}else{
		unlink(tmpFileName);
		close(fd);
	}
    strcpy(cliaddr.sun_path, tmpFileName);

    if(bind(sockfd, (SA *) &cliaddr, sizeof(cliaddr)) < 0){
		err_sys("[ERROR]: Client: bind error.");
	}

    //deal with the process-----------------------------------------------------------
    while(1){   

    	//initiate
		flag = 0;

	    printf("[INFO]: >>Please choose one of vm1, ... ..., vm10 as a server node.\n\t>>Or type 'quit' to exit the program\n"); 

	    if(fgets(dest_name, 1024, stdin) == NULL){
			if(errno == EINTR){
				perror("[ERROR]: fgets error");
				printf("[INFO]: Restart... ...");
				continue;
			}else{
				err_sys("[ERROR]: fgets error");
			}
		}
		dest_name[strlen(dest_name)-1] = '\0';

		//judge the input format
		if(strcmp(dest_name, "vm1") && strcmp(dest_name, "vm2") && strcmp(dest_name, "vm3") && strcmp(dest_name, "vm4") && strcmp(dest_name, "vm5") &&
			strcmp(dest_name, "vm6") && strcmp(dest_name, "vm7") && strcmp(dest_name, "vm8") && strcmp(dest_name, "vm9") && strcmp(dest_name, "vm10") &&
			strcmp(dest_name, "quit")){ 
			printf("[WARNING]: Invalid choice format: vm<i> or quit\n");
			continue;	
		}

		if(addr2name(src_name, host_ipaddr[1]) < 0){
			err_sys("[ERROR]: addr2name error");
		}
		if(!strcmp(dest_name, src_name)){
			printf("[WARNING]: Now locate in %s, Please choose another node.\n", src_name);
			continue;
		}

		if(!strcmp(dest_name, "quit")){
			printf("[INFO]: Exiting the program... ...\n");
			break;
		}

		if(name2addr(dest_name, dest_ipaddr) < 0){
			err_sys("[ERROR]: name2addr error");
		}

		//client at node  vm i1  sending request to server at  vm i2
		printf("\n[INFO]: Client at node %s sending request to server at %s\n\n", src_name, dest_name);

    	bzero(&odraddr, sizeof(odraddr)); /* fill in server's address */
     	odraddr.sun_family = AF_LOCAL;
     	strcpy(odraddr.sun_path, MY_ODRDG_PATH);

     	memcpy(dest_path, MY_UNIXDG_PATH, PATH_LEN);
     	memcpy(msg, "Hi, server", ODR_DATA_LEN);

     	while(1){

	     	if(msg_send(sockfd, (SA *) &odraddr, sizeof(odraddr), dest_ipaddr, dest_path, msg, flag) < 0){
	     		err_sys("[ERROR]: msg_send error");
	     	}

	     	tm.tv_sec = 10;
			tm.tv_usec = 0;
			FD_ZERO(&rset);
			FD_SET(sockfd, &rset);
			if((n = select(sockfd+1, &rset, NULL, NULL, &tm)) < 0){
				if(errno == EINTR){
					printf("[WARNING]: Ignored EINTR signal");
				} else {
					err_sys("[ERROR]: select error");
				}
			}
			if(FD_ISSET(sockfd, &rset)){
				
				if(msg_recv(sockfd, (SA *)&odraddr, sizeof(odraddr), recv_ipaddr, NULL, msg, NULL) < 0){
                	err_sys("[ERROR]: msg_recv error");
            	}

            	if(addr2name(recv_name, recv_ipaddr) < 0){
					err_sys("[ERROR]: addr2name error");
				}
				//client at node  vm i1 : received from   vm i2  <timestamp>
				printf("\n[INFO]: Client at node %s: received form %s <%s>\n\n", src_name, recv_name, msg);
				
				break;

			}else if (n == 0){
				printf("[WARNING]: Client at node %s: timeout on response from %s\n", src_name, dest_name);
				if(flag == 0){
					flag = 1;
					continue;
				}else{
					printf("[ERROR]: Time out twice! Cancel this request...\n");
					break;
				}
			}
		}
    }
	exit(0);
}
