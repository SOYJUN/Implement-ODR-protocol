#include "unp.h"
#include "hw_addrs.h"
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#define MY_ETH_PROTOCOL 0x0916
#define ODR_DATA_LEN 		1410
#define PATH_LEN			15

int main(int argc, char **argv){

	int 				i, index, n, sockfd;
	//get hw addr initiate
	struct hwa_info 	*hwa, *hwahead;
	//the pf_packet initiate
	struct sockaddr_ll	destaddr;
	void				*buffer = (void *)malloc(ETH_FRAME_LEN);
	unsigned char		*etherhead = buffer;
	unsigned char		*data = buffer + 14;
	unsigned char 		src_mac[5][ETH_ALEN]; 
	unsigned char 		dest_mac[ETH_ALEN] = {0x00, 0x0C, 0x29, 0xDA, 0x5F, 0xBD};
	char 				*ptr;
	char 				msg[ODR_DATA_LEN] = "sigh, fucking badr!";
	char				src_ipaddr[INET_ADDRSTRLEN+1] = "192.168.9.16";
	char				src_path[PATH_LEN+1] = "/tmp/un.1n2n3m";
	char				dest_ipaddr[INET_ADDRSTRLEN+1] = "192.168.0.1";
	char				dest_path[PATH_LEN+1] = "/tmp/myserv.dg";
	struct ethhdr		*eh = (struct ethhdr *)etherhead;

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
				src_mac[index][i] = *ptr++ & 0xff;
				printf("%.2x%s", src_mac[index][i], (i == 5) ? " " : ":");
				i++;
			} 	
			printf("\n");	
			index++;
		}
	}

	printf("\n");

	//buid the send socket
	if((sockfd = socket(PF_PACKET, SOCK_RAW, htons(MY_ETH_PROTOCOL))) < 0){
		err_sys("[ERROR]: PF_PACKET socket build error");
	}

	//set frame header
	memcpy((void *)buffer, (void *)dest_mac, ETH_ALEN);
	memcpy((void *)(buffer + ETH_ALEN), (void *)src_mac[index-1], ETH_ALEN);
	eh->h_proto = MY_ETH_PROTOCOL;

	//send the packet
	if((n = odr_send(sockfd, buffer, msg, src_ipaddr, src_path, dest_ipaddr, dest_path, 2, 0, 1, 1, 54, 11)) < 0){
		err_sys("[ERROR]: sendto error");
	}

}