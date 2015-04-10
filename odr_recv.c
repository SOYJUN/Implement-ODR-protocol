#include "unp.h"
#include "hw_addrs.h"
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#define MAC_LEN 			6
#define MY_ETH_PROTOCOL		0x0916
#define ODR_DATA_LEN		1410
#define PATH_LEN			15

int main(int argc, char **argv){

	int 				i, j, index, prflag, n, sockfd[5], tmp, maxfdp1;
	//get hw addr initiate
	struct hwa_info 	*hwa, *hwahead;
	//the pf_packet initiate
	struct sockaddr_ll	hostaddr[5];
	void				*buffer = (void *)malloc(ETH_FRAME_LEN);
	unsigned char		*etherhead = buffer;
	uint16_t			odr_type;
	uint16_t			rrep_already_sent;
	uint16_t	 		force_discovery;
	uint16_t	 		broadcast_id; 
	uint16_t 	 		hop_cnt;
	unsigned char		src_ipaddress[INET_ADDRSTRLEN+1];
	unsigned char 		src_path[PATH_LEN+1];
	unsigned char 		dest_ipaddress[INET_ADDRSTRLEN+1]; 
	unsigned char 		dest_path[PATH_LEN+1];
	unsigned char 		msg[ODR_DATA_LEN];
	unsigned char		host_mac[5][ETH_ALEN];
	char				*ptr;
	struct ethhdr		*eh = (struct ethhdr *)etherhead;
	fd_set 				rset;
	const int 			on = 1;
	socklen_t			len;

	//get the host mac addr-----------------------------------------------
	index = 0;
	for(hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next){
		if(hwa->if_index > 0){
			printf("Interface:      %s\n", hwa->if_name);
			printf("MAC-address[%d]: ", index);
			ptr = hwa->if_haddr;
			i = 0;
			while(i < 6){
				//output and store the mac addr
				host_mac[index][i] = *ptr++ & 0xff;
				printf("%.2x%s", host_mac[index][i], (i == 5) ? " " : ":");
				i++;
			}
			printf("\n");	
			index++;
		}
	}

	printf("\n");

	for(i = 0; i < index; i++){
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

	//use select to monitor the socket----------------------------------------
	while(1){
		//set up the select parameters
		FD_ZERO(&rset);
		for(i = 0; i < index; i++){
			tmp = 0;
			tmp = max(sockfd[i], tmp);
		}
		maxfdp1 = tmp + 1;
		for(i = 0; i < index; i++){
			FD_SET(sockfd[i], &rset);
		}
		if(select(maxfdp1, &rset, NULL, NULL, NULL) < 0){
			if(errno == EINTR)
				continue;
			else
				err_sys("[ERROR]: select error...");
		}

		//deal with the socket receive-----------------------------------------
		for(i = 0; i < index; i++){
			if(FD_ISSET(sockfd[i], &rset)){

				//receive the packet
				if((n = odr_recv(sockfd[i], buffer, msg, src_ipaddress, src_path, dest_ipaddress, dest_path,
					 &odr_type, &rrep_already_sent, &force_discovery, &broadcast_id, &hop_cnt)) < 0){
					err_sys("[ERROR]: sendto error");
				}
				//protocol filter........
				if(eh->h_proto != MY_ETH_PROTOCOL){
					continue;
				}
				//source mac addr
				printf("Message for: ");
				i = 0;
				while(i < MAC_LEN){
					printf("%.2x", *etherhead++ & 0xff);
					if(i != MAC_LEN-1){
						printf(":");
					}
					i++;
				}

				//destination mac addr
				i = 0;
				printf("\nMessage from:   ");
				while(i < MAC_LEN){
					printf("%.2x", *etherhead++ & 0xff);
					if(i != MAC_LEN-1){
						printf(":");
					}
					i++;
				}

				printf("\n\nODR message type: %d\n", odr_type);

				printf("\nRREP already sent: %d\n", rrep_already_sent);

				printf("\nForce discovery: %d\n", force_discovery);

				printf("\nBroadcast ID: %d\n", broadcast_id);

				printf("\nHop count: %d\n", hop_cnt);

				printf("\nSource IP address: %s\n", src_ipaddress);

				printf("\nSource IP tmp file path: %s\n", src_path);

				printf("\nDestination IP address: %s\n", dest_ipaddress);

				printf("\nDestination IP tmp file path: %s\n", dest_path);

				printf("\nRecive message: %s\n\n", msg);
			}
			etherhead = buffer;
		}
	}
	exit(0);
}







