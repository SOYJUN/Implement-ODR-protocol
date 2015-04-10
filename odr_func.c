#include "unp.h"
#include "hw_addrs.h"
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#define MY_ETH_PROTOCOL 0x0916
#define ODR_DATA_LEN		1410
#define PATH_LEN			15

ssize_t odr_send(int sockfd, void *buffer, char *msg, char *src_ipaddr, char *src_path, 
	char *dest_ipaddr, char *dest_path, int ifindex, int type, int ra_sent, int f_discovery, 
	int b_id, int h_cnt){

	int 				n;
	struct sockaddr_ll	destaddr;
	//odr protocol message encapsulated
	uint16_t			*odr_type = buffer + 14; //16 network order bytes odr type
	uint16_t			*rrep_already_sent = buffer + 16; // two byte rrep already sent flag
	uint16_t		 	*force_discovery = buffer + 18;	//two bye force discovery flag
	uint16_t	 		*broadcast_id = buffer + 20; // two byte broadcast ID 
	uint16_t 	 		*hop_cnt = buffer + 22; // two byte hop count
	unsigned char		*src_ipaddress = buffer + 24; //16 byte source ip address
	unsigned char		*src_sun_path = buffer + 41;//15 byte tmp file name
	unsigned char 		*dest_ipaddress = buffer + 57; // 16 byte destination ip address
	unsigned char		*dest_sun_path =  buffer + 74;//15 byte tmp file name
	unsigned char 		*data = buffer + 90;
	/*flooding out mac address*/
	//unsigned char 		dest_mac[ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; 

	//initiate destination sockaddr_ll
	bzero(&destaddr, sizeof(destaddr));
	destaddr.sll_family = PF_PACKET;
	destaddr.sll_ifindex = ifindex;
	destaddr.sll_halen = ETH_ALEN;

	//set odr mesg header
	*odr_type = htons(type);
	*rrep_already_sent = htons(ra_sent);
	*force_discovery = htons(f_discovery);
	*broadcast_id = htons(b_id);
	*hop_cnt = htons(h_cnt);
	memcpy((void *)src_ipaddress, (void *)src_ipaddr, INET_ADDRSTRLEN+1);
	memcpy((void *)dest_ipaddress, (void *)dest_ipaddr, INET_ADDRSTRLEN+1);
	memcpy((void *)src_sun_path, (void *)src_path, PATH_LEN+1);
	memcpy((void *)dest_sun_path, (void *)dest_path, PATH_LEN+1);
	memcpy((void *)data, (void *)msg, ODR_DATA_LEN);

	//send the packet
	if((n = sendto(sockfd, buffer, ETH_FRAME_LEN, 0, (SA *) &destaddr, sizeof(destaddr))) < 0){
		perror("[ERROR]: odr_func: sendto error");
		return (-1);
	}
	return n;
}

ssize_t odr_recv(int sockfd, void *buffer, char *msg, char *src_ipaddr, char *src_path, 
	char *dest_ipaddr, char *dest_path, int *type, int *ra_sent, int *f_discovery, 
	int *b_id, int *h_cnt){

	int 				n;
	//odr protocol message encapsulated
	uint16_t			*odr_type = buffer + 14; //16 network order bytes odr type
	uint16_t			*rrep_already_sent = buffer + 16; // two byte rrep already sent flag
	uint16_t		 	*force_discovery = buffer + 18;	//two bye force discovery flag
	uint16_t	 		*broadcast_id = buffer + 20; // two byte broadcast ID 
	uint16_t 	 		*hop_cnt = buffer + 22; // two byte hop count
	unsigned char		*src_ipaddress = buffer + 24; //16 byte source ip address
	unsigned char		*src_sun_path = buffer + 41;//15 byte tmp file name
	unsigned char 		*dest_ipaddress = buffer + 57; // 16 byte destination ip address
	unsigned char		*dest_sun_path =  buffer + 74;//15 byte tmp file name
	unsigned char 		*data = buffer + 90;
	/*flooding out mac address*/
	//unsigned char 		dest_mac[ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; 

	//receive the packet
	if((n = recvfrom(sockfd, buffer, ETH_FRAME_LEN, 0, NULL, NULL)) < 0){
		perror("[ERROR]: odr_func: recvfrom error");
		return (-1);
	}

	//set odr mesg header
	*type = ntohs(*odr_type);
	*ra_sent = ntohs(*rrep_already_sent);
	*f_discovery = ntohs(*force_discovery);
	*b_id = ntohs(*broadcast_id);
	*h_cnt = ntohs(*hop_cnt);
	memcpy((void *)src_ipaddr, (void *)src_ipaddress, INET_ADDRSTRLEN+1);
	memcpy((void *)dest_ipaddr, (void *)dest_ipaddress, INET_ADDRSTRLEN+1);
	memcpy((void *)src_path, (void *)src_sun_path, PATH_LEN+1);
	memcpy((void *)dest_path, (void *)dest_sun_path, PATH_LEN+1);
	memcpy((void *)msg, (void *)data, ODR_DATA_LEN);

	return n;
}

void prifInfo(char *host_ipaddr, unsigned char *src_ipaddr, unsigned char *dest_ipaddr, 
	     unsigned char *dest_mac, int type){
	int 		i;
	char		src_name[50];
	char 		host_name[50];
    char        dest_name[50];
    char		dest_mac_show[50];

    if(addr2name(src_name, src_ipaddr) < 0){
        err_sys("[ERROR]: prifInfo: addr2name1 error");
    }

    if(addr2name(host_name, host_ipaddr) < 0){
        err_sys("[ERROR]: prifInfo: addr2name2 error");
    }
    if(addr2name(dest_name, dest_ipaddr) < 0){
        err_sys("[ERROR]: prifInfo: addr2name3 error");
    }
    
    printf("[INFO]: ODR at node %s: sending frame hdr at node %s, dest addr:", host_name, host_name);
    i = 0;

    while(i < 6){
        //write the mac addr in buffer of presentation
        printf("%.2x%s", dest_mac[i], (i == 5) ? " " : ":");
        i++;
    }

    printf("\nODR msg type %d from src %s to dest %s\n", type, src_name, dest_name);
}


