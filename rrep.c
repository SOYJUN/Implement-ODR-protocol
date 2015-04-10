#include "unp.h"
#include "hw_addrs.h"
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#define MY_ETH_PROTOCOL 0x0916
#define RRE_DATA_LEN 		1445

ssize_t rrep(int sockfd, void *buffer, SA *destaddr, socklen_t len, char *msg, char *src_ipaddr, char *dest_ipaddr){

	int 				n;
	//odr protocol message encapsulated
	uint16_t			*odr_type = buffer + 14; //16 network order bytes odr type
	uint16_t		 	*force_discovery = buffer + 16;	//two bye force discovery flag
	uint16_t	 		*broadcast_id = buffer + 18; // two byte broadcast ID 
	unsigned char		*src_ipaddress = buffer + 20; //16 byte source ip address
	unsigned char 		*dest_ipaddress = buffer + 37; // 16 byte destination ip address
	unsigned char 		*data = buffer + 54;
	/*flooding out mac address*/
	//unsigned char 		dest_mac[ETH_ALEN] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; 

	//set odr mesg header
	*odr_type = htons(1);
	*force_discovery = htons(0);
	*broadcast_id = htons(0);
	memcpy((void *)src_ipaddress, (void *)src_ipaddr, INET_ADDRSTRLEN+1);
	memcpy((void *)dest_ipaddress, (void *)dest_ipaddr, INET_ADDRSTRLEN+1);
	memcpy((void *)data, (void *)msg,RRE_DATA_LEN);

	//send the packet
	if((n = sendto(sockfd, buffer, ETH_FRAME_LEN, 0, destaddr, len)) < 0){
		perror("[ERROR]: RREP: sendto error");
		return (-1);
	}
	return n;
}