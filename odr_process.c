#include    "unp.h"
#include "hw_addrs.h"
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <time.h>

#define MY_ODRDG_PATH       "/tmp/myodr.dg"
#define MY_UNIXDG_PATH      "/tmp/myserv.dg"
#define PATH_LEN            15
#define MAC_LEN             6
#define MY_ETH_PROTOCOL     0x0916
#define ODR_DATA_LEN        1410

struct routing_table
{
    unsigned char       destination_ipaddress[10][INET_ADDRSTRLEN];
    unsigned char       next_hop[10][ETH_ALEN];
    int                 hop_number[10];
    time_t              stale_time[10];
};


int main(int argc, char **argv){

	int                 n, sockfd[5], sockfd_un, i, j, index, tmp, maxfdp1, isMyPacket, flag, isRREQ, isClient, isServer, reply_id; 
    int                 t_sec; // store the user setting staleness time
    const int           on = 1;
    char                *ptr;
    char                host_ipaddr[5][INET_ADDRSTRLEN+1];
    char                src_name[50];
    char                dest_name[50];
    unsigned char       host_mac[5][ETH_ALEN+1];
    unsigned char       dest_mac[ETH_ALEN+1];
    unsigned char       src_mac[ETH_ALEN+1];
    unsigned char       broadcast_mac[ETH_ALEN+1] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    struct sockaddr_un  odraddr, peeraddr;
    struct hwa_info     *hwa, *hwahead;
    struct sockaddr     *sa;
    struct sockaddr_ll  hostaddr[5];
    fd_set              rset;

    //initiate the frame header
    void                *buffer = (void *)malloc(ETH_FRAME_LEN);
    unsigned char       *etherhead = buffer;
    struct ethhdr       *eh = (struct ethhdr *)etherhead;

    //receive parameter setting
    uint16_t            ifindex;
    uint16_t            odr_type;
    uint16_t            rrep_already_sent;
    uint16_t            force_discovery;
    uint16_t            broadcast_id; 
    uint16_t            hop_cnt;
    unsigned char       src_ipaddress[INET_ADDRSTRLEN+1];
    unsigned char       src_path[PATH_LEN+1];
    unsigned char       dest_ipaddress[INET_ADDRSTRLEN+1]; 
    unsigned char       dest_path[PATH_LEN+1];
    unsigned char       tmp_ipaddress[INET_ADDRSTRLEN+1]; 
    unsigned char       tmp_path[PATH_LEN+1];
    unsigned char       msg[ODR_DATA_LEN];//use for odr packet socket
    char                data[MAXLINE];//use for unix domain socket

    //set up a routing table
    struct routing_table rt;

    unlink(MY_ODRDG_PATH);

    if (argc != 2){
        err_quit("[ERROR]: please enter correct format: ./odr_process <time>");
    }
    //store the staleness time--------------------------------------------------------
    t_sec = atoi(argv[1]);
    printf("\n[INFO]: The staleness time period is set to: %d sec\n\n", t_sec);

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

    //build the packet socket and bind it to each interface---------------------------
    for(i = 0; i < index; i++){
        if(i == 0 || i == 1){
            continue;
        }
        bzero(&hostaddr[i], sizeof(hostaddr[i]));
        //prepare sockaddr_ll
        hostaddr[i].sll_family = PF_PACKET;
        hostaddr[i].sll_protocol = htons(ETH_P_ALL);
        hostaddr[i].sll_ifindex = (i+1);
        hostaddr[i].sll_hatype = ARPHRD_ETHER;
        hostaddr[i].sll_pkttype = PACKET_OTHERHOST;
        hostaddr[i].sll_halen = ETH_ALEN;
        //mac addr
        for(j = 0; j < MAC_LEN; j++){
            hostaddr[i].sll_addr[j] = host_mac[i][j];
        }
        //bind each mac addr to PF_PACKET socket---------------------------------
        if((sockfd[i] = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0){
            err_sys("[ERROR]: PF_PACKET socket build error");
        }
        if (setsockopt(sockfd[i], SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
            err_sys("[ERROR]: setsockopt<SO_REUSEADDR> error");
        }
        if(bind(sockfd[i], (SA *) &hostaddr[i], sizeof(hostaddr[i])) < 0){
            err_sys("[ERROR]: bind error");
        }
    }

    //build the unix socket and bind it to tmp file-----------------------------------
    if((sockfd_un = socket(PF_LOCAL, SOCK_DGRAM, 0)) < 0){
	   err_sys("[ERROR]: ODR: socket error.");
	}
   	unlink(MY_ODRDG_PATH);
    bzero(&odraddr, sizeof(odraddr));
	odraddr.sun_family = AF_LOCAL;
	strcpy(odraddr.sun_path, MY_ODRDG_PATH);
 	if(bind(sockfd_un, (SA *) &odraddr, sizeof(odraddr)) < 0){
	   err_sys("[ERROR]: ODR: bind error.");
	}

    //wait for the packet from client or server
    printf("[INFO]: Waiting for the peer request... ...\n");

    broadcast_id = 0;
    reply_id = 0;
    isMyPacket = 0;

    //use select to monitor the socket----------------------------------------
    while(1){

        //set up the select parameters
        rrep_already_sent = 0;
        force_discovery = 0;   
        isRREQ = 0;     

        FD_ZERO(&rset);
        for(i = 0; i < index; i++){
            if(i == 0 || i == 1){
                continue;
            }
            tmp = 0;
            tmp = max(sockfd[i], tmp);
        }
        tmp = max(sockfd_un, tmp);
        maxfdp1 = tmp + 1;
        for(i = 0; i < index; i++){
            if(i == 0 || i == 1){
                continue;
            }
            FD_SET(sockfd[i], &rset);
        }
        FD_SET(sockfd_un, &rset);
        if(select(maxfdp1, &rset, NULL, NULL, NULL) < 0){
            if(errno == EINTR){
                continue;
            }else{
                err_sys("[ERROR]: select error...");
            }
        }

        //deal with the UNIX domain socket receive------------------------------------
        if(FD_ISSET(sockfd_un, &rset)){

            //initiate
            isClient = 0;
            isServer = 0;

            if(msg_recv(sockfd_un, (SA *) &peeraddr, sizeof(peeraddr), dest_ipaddress, dest_path, data, &flag) < 0){
                err_sys("[ERROR]: msg_recv error");
            }

            memcpy(src_path, sock_ntop((SA *) &peeraddr, sizeof(peeraddr)), ETH_ALEN);

            //judge the packet from client or server
            if(!strcmp(src_path, MY_UNIXDG_PATH)){
                //request from server  
                isServer = 1;
            }else{
                //request from client
                isClient = 1;
            }

            /*Generate a RREQ in response to a time client calling msg_send for a destination 
            for which ODR has no route (or for which a route exists, but msg_send has the flag 
            parameter set or the route has gone ‘stale’ – see below), and ‘flood’ the RREQ out 
            on all the node’s interfaces (except for the interface it came in on and, of course, 
            the interfaces eth0 and lo).
            */
            
            //sinario when request from client
            if(isClient){

                if(!isExistDest(rt, dest_ipaddress)){
                    isRREQ = 1;
                    printf("[INFO]: New destination request...\n");
                }else if(flag == 1){
                    isRREQ = 1;
                    force_discovery = 1;
                    printf("[INFO]: Client forces a **Route Rediscovery**\n");
                }else if(!checkStaleEntry(&rt, dest_ipaddress, t_sec)){
                    isRREQ = 1;
                    printf("[INFO]: This entry in routing table has been stale...\n");
                }

                if(isRREQ){
            
                    //flood the RREQ out
                    odr_type = 0; //RREQ
                    hop_cnt = 0;

                    for(i = 0; i < index; i++){
                        if(i == 0 || i == 1){
                            continue;
                        }
                        broadcast_id++;
                        ifindex = i+1;

                        //set frame header
                        memcpy((void *)buffer, (void *)broadcast_mac, ETH_ALEN);
                        memcpy((void *)(buffer + ETH_ALEN), (void *)host_mac[i], ETH_ALEN);
                        eh->h_proto = MY_ETH_PROTOCOL;

                        //send the packet
                        memcpy(msg, data, ODR_DATA_LEN);
                        printf("broadcast_id: %d\n", broadcast_id);

                        if((n = odr_send(sockfd[i], buffer, msg, host_ipaddr[1], src_path, dest_ipaddress, dest_path, 
                            ifindex, odr_type, rrep_already_sent, force_discovery, broadcast_id, hop_cnt)) < 0){
                            err_sys("[ERROR]: odr_send error");
                        }

                        prifInfo(host_ipaddr[1], host_ipaddr[1], dest_ipaddress, broadcast_mac, odr_type);
                    
                    }
                }else{
                    //directly transmit the data
                    odr_type = 2;
                    hop_cnt = 0;
                    
                    getNextHop(rt, dest_ipaddress, dest_mac);

                    for(i = 0; i < index; i++){
                        if(i == 0 || i == 1){
                            continue;
                        }
                
                        ifindex = i+1;

                        //set frame header
                        memcpy((void *)buffer, (void *)dest_mac, ETH_ALEN);
                        memcpy((void *)(buffer + ETH_ALEN), (void *)host_mac[i], ETH_ALEN);
                        eh->h_proto = MY_ETH_PROTOCOL;

                        //send the packet
                        memcpy(msg, data, ODR_DATA_LEN);
                        if((n = odr_send(sockfd[i], buffer, msg, host_ipaddr[1], src_path, dest_ipaddress, dest_path, 
                            ifindex, odr_type, rrep_already_sent, force_discovery, broadcast_id, hop_cnt)) < 0){
                            err_sys("[ERROR]: odr_send error");
                        }
                        prifInfo(host_ipaddr[1], host_ipaddr[1], dest_ipaddress, dest_mac, odr_type);
                    }
                }
            }
            //sinario when request from server
            if(isServer){
                odr_type = 2;
                hop_cnt = 0;

                getNextHop(rt, dest_ipaddress, dest_mac);

                for(i = 0; i < index; i++){
                    if(i == 0 || i == 1){
                        continue;
                    }

                    ifindex = i+1;

                    //set frame header
                    memcpy((void *)buffer, (void *)dest_mac, ETH_ALEN);
                    memcpy((void *)(buffer + ETH_ALEN), (void *)host_mac[i], ETH_ALEN);
                    eh->h_proto = MY_ETH_PROTOCOL;

                    //send the packet
                    memcpy(msg, data, ODR_DATA_LEN);
                    if((n = odr_send(sockfd[i], buffer, msg, host_ipaddr[1], src_path, dest_ipaddress, dest_path, 
                        ifindex, odr_type, rrep_already_sent, force_discovery, broadcast_id, hop_cnt)) < 0){
                        err_sys("[ERROR]: odr_send error");
                    }
                    prifInfo(host_ipaddr[1], host_ipaddr[1], dest_ipaddress, dest_mac, odr_type);
                }
            }
        }


        //deal with the PACKET socket receive
        for(i = 0; i < index; i++){
            if(i == 0 || i == 1){
                continue;
            }
            if(FD_ISSET(sockfd[i], &rset)){

                //receive the packet
               if((n = odr_recv(sockfd[i], buffer, msg, src_ipaddress, src_path, dest_ipaddress, dest_path,
                     &ifindex, &odr_type, &rrep_already_sent, &force_discovery, &broadcast_id, &hop_cnt)) < 0){
                    err_sys("[ERROR]: odr_recv error");
                }

                //protocol filter........
                if(eh->h_proto != MY_ETH_PROTOCOL){
                    continue;
                }

                //source mac addr
                j = 0;
                while(j < MAC_LEN){
                    dest_mac[j] = *etherhead++ & 0xff;
                    j++;
                }

                //destination mac addr
                j = 0;
                while(j < MAC_LEN){
                    src_mac[j] = *etherhead++ & 0xff;
                    j++;
                }

                //reset the parameter
                etherhead = buffer;

                //record into the routing table
                if(odr_type == 0 || odr_type == 1){
                    updateNewDest(&rt, src_ipaddress, src_mac);
                }
                
                //judge the packet whether is for this node
                
                if(!strcmp(dest_ipaddress, host_ipaddr[1])){
                    isMyPacket = 1;
                }
               
                //judge the packet for this node or not
                if(isMyPacket){
                    //if the packet is for me
                    if(addr2name(src_name, src_ipaddress) < 0){
                        err_sys("[ERROR]: addr2name error");
                    }
                    if(addr2name(dest_name, dest_ipaddress) < 0){
                        err_sys("[ERROR]: addr2name error");
                    }
                    printf("[INFO]: Receive a packet from %s to %s (this node) of interface[%d].\n", src_ipaddress, dest_ipaddress, i+1);

                    //relay the message to client/servers
                    if(msg[0] != '\0'){
                        bzero(&peeraddr, sizeof(peeraddr));
                        peeraddr.sun_family = AF_LOCAL;
                        strcpy(peeraddr.sun_path, dest_path);

                        if(msg_send(sockfd_un, (SA *) &peeraddr, sizeof(peeraddr), src_ipaddress, src_path, msg, 0) < 0){
                            err_sys("[ERROR]: msg_send error");
                        }
                    }

                    //reply the RREP
                    if(broadcast_id > reply_id){
                        reply_id = broadcast_id;
                        //reply the RREQ with 
                        odr_type = 1;
                        hop_cnt = 0;
                        ifindex = i+1;
                        rrep_already_sent = 1;
                        
                        //interchange the dest data and src data, and clear the payload
                        memcpy(tmp_ipaddress, dest_ipaddress, INET_ADDRSTRLEN);
                        memcpy(dest_ipaddress, src_ipaddress, INET_ADDRSTRLEN);
                        memcpy(src_ipaddress, tmp_ipaddress, INET_ADDRSTRLEN);
                        memcpy(tmp_path, dest_path, PATH_LEN);
                        memcpy(dest_path, src_path, PATH_LEN);
                        memcpy(src_path, tmp_path, PATH_LEN);
                        bzero(msg, ODR_DATA_LEN);

                        getNextHop(rt, dest_ipaddress, dest_mac);

                        //set frame header
                        memcpy((void *)buffer, (void *)dest_mac, ETH_ALEN);
                        memcpy((void *)(buffer + ETH_ALEN), (void *)host_mac[i], ETH_ALEN);
                        eh->h_proto = MY_ETH_PROTOCOL;

                        //send the packet
                        memcpy(msg, data, ODR_DATA_LEN);
                        if((n = odr_send(sockfd[i], buffer, msg, host_ipaddr[1], src_path, dest_ipaddress, dest_path, 
                            ifindex, odr_type, rrep_already_sent, force_discovery, broadcast_id, hop_cnt)) < 0){
                            err_sys("[ERROR]: odr_send error");
                        }
                        prifInfo(host_ipaddr[1], host_ipaddr[1], dest_ipaddress, dest_mac, odr_type);
                    }else{
                        //have been replied to this RREQ before, so ignore this time
                        continue;
                    }

                }else{

                    //if the packet is not for me
                    if(odr_type == 0){
                        if(broadcast_id > reply_id){
                            reply_id = broadcast_id;
                        }else{
                            continue;
                        }
                        //if is RREQ, flood it out on other interfaces
                        for(j = 0; j < index; j++){
                            if(j == 0 || j == 1){
                                continue;
                            }
                            if(j == i){
                                continue;
                            }

                            ifindex = j+1;

                            getNextHop(rt, dest_ipaddress, dest_mac);

                            //set frame header
                            memcpy((void *)buffer, (void *)broadcast_mac, ETH_ALEN);
                            memcpy((void *)(buffer + ETH_ALEN), (void *)host_mac[j], ETH_ALEN);
                            eh->h_proto = MY_ETH_PROTOCOL;

                            //add hop count
                            hop_cnt++;

                            //send the packet
                            if((n = odr_send(sockfd[j], buffer, msg, src_ipaddress, src_path, dest_ipaddress, dest_path, 
                                ifindex, odr_type, rrep_already_sent, force_discovery, broadcast_id, hop_cnt)) < 0){
                                err_sys("[ERROR]: odr_send error");
                            }
                            prifInfo(host_ipaddr[1], src_ipaddress, dest_ipaddress, dest_mac, odr_type);
                        }
                    }
                    if(odr_type == 1 || odr_type == 2){
                        for(j = 0; j < index; j++){
                            if(j == 0 || j == 1){
                                continue;
                            }
                            if(j == i){
                                continue;
                            }

                            ifindex = j+1;

                            //set frame header
                            memcpy((void *)buffer, (void *)broadcast_mac, ETH_ALEN);
                            memcpy((void *)(buffer + ETH_ALEN), (void *)host_mac[j], ETH_ALEN);
                            eh->h_proto = MY_ETH_PROTOCOL;

                            //add hop count
                            hop_cnt++;

                            //send the packet
                            if((n = odr_send(sockfd[j], buffer, msg, src_ipaddress, src_path, dest_ipaddress, dest_path, 
                                ifindex, odr_type, rrep_already_sent, force_discovery, broadcast_id, hop_cnt)) < 0){
                                err_sys("[ERROR]: odr_send error");
                            }
                            prifInfo(host_ipaddr[1], src_ipaddress, dest_ipaddress, dest_mac, odr_type);
                        }
                    }
                }
            }
        }
    }

    exit(0);
}

