#include    "unp.h"
#include "hw_addrs.h"
#include <linux/if_ether.h>
#include <time.h>

#define MY_UNIXDG_PATH      "/tmp/myserv.dg"
#define MY_ODRDG_PATH       "/tmp/myodr.dg"
#define PATH_LEN    15
#define ODR_DATA_LEN        1410

int main(int argc, char **argv){

	int                 i, sockfd, index, flag;
    char                src_ipaddr[INET_ADDRSTRLEN+1];
    char                src_path[PATH_LEN+1];
    char                msg[ODR_DATA_LEN];
    char                *ptr;
    char                host_ipaddr[5][INET_ADDRSTRLEN+1];
    char                dest_name[1024];
    char                src_name[1024];
    char                recv_name[1024];
    unsigned char       host_mac[5][ETH_ALEN+1];
    struct sockaddr_un  servaddr, odraddr;
    struct hwa_info     *hwa, *hwahead;
    struct sockaddr     *sa;
    time_t              ticks;
    

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

    //set up the socket----------------------------------------------------------------
    if((sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0)) < 0){
		err_sys("[ERROR]: Server: socket error.");
	}

   	unlink(MY_UNIXDG_PATH);
    	bzero(&servaddr, sizeof(servaddr));
    	servaddr.sun_family = AF_LOCAL;
    	strcpy(servaddr.sun_path, MY_UNIXDG_PATH);

    if(bind(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
	   err_sys("[ERROR]: Server: bind error.");
	}

    //deal with the process-----------------------------------------------------------
    while(1){
    	if(msg_recv(sockfd, (SA *) &odraddr, sizeof(odraddr), src_ipaddr, src_path, msg, &flag) < 0){
            err_sys("[ERROR]: msg_recv error");
        }

        if(addr2name(recv_name, src_ipaddr) < 0){
            err_sys("[ERROR]: addr2name error");
        }
        
        if(addr2name(src_name, host_ipaddr[1]) < 0){
            err_sys("[ERROR]: addr2name error");
        }

        printf("\n[INFO]: Server at node %s receiving a request from %s\n\n", src_name, recv_name);

        ticks = time (NULL);
        snprintf(msg, sizeof(msg), "%.24s\r\n", ctime(&ticks) ) ;
        if(msg_send(sockfd, (SA *) &odraddr, sizeof(odraddr), src_ipaddr, src_path, msg, 0) < 0){
            err_sys("[ERROR]: msg_send error");
        }

        //server at node  vm i1  responding to request from  vm i2
        printf("\n[INFO]: Server at node %s responding to request from %s\n\n", src_name, recv_name);
        
    }
    
    exit(0);
}

