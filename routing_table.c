#include "unp.h"
#include "hw_addrs.h"
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <time.h>

struct routing_table
{
    unsigned char       destination_ipaddress[10][INET_ADDRSTRLEN];
    unsigned char       next_hop[10][ETH_ALEN];
    int                 hop_number[10];
    time_t				stale_time[10];
};

int isExistDest(struct routing_table rt, unsigned char *dest_ipaddr){
	int i;

	for(i = 0; i < 10; i++){
		if(!strcmp(dest_ipaddr, rt.destination_ipaddress[i])){
			return 1;
		}
	}
	return 0;
}

/*use when  a client at a node calls msg_send, and if an entry 
  for the destination node already exists in the routing table
*/
int checkStaleEntry(struct routing_table *rt, unsigned char *dest_ipaddr, int t_sec){
	int i, index, isStaleEntry;
	time_t current = time(NULL);

	isStaleEntry = 0;
	for(i = 0; i < 10; i++){
		if(!strcmp(dest_ipaddr, rt -> destination_ipaddress[i])){
			index = i;
			if(current > (rt -> stale_time[i] + t_sec)){
				isStaleEntry = 1;
				break;
			}
		}
	}
	
	//delete this stale entry(copy the next entry forward)
	for(i = index; i < 9; i++){
		memcpy(rt -> destination_ipaddress[i], rt -> destination_ipaddress[i+1], INET_ADDRSTRLEN);
		memcpy(rt -> next_hop[i], rt -> next_hop[i+1], ETH_ALEN);
		rt -> hop_number[i] = rt -> hop_number[i+1];
		rt -> stale_time[i] = rt -> stale_time[i+1];
	}

	//zero the last entry
	bzero(rt -> destination_ipaddress[9], INET_ADDRSTRLEN);
	bzero(rt -> next_hop[9], ETH_ALEN);
	rt -> hop_number[9] = 0;
	rt -> stale_time[9] = 0;

	if(isStaleEntry){
		return 1; // has been stale
	}
	return 0;
}

void getNextHop(struct routing_table rt, unsigned char* dest_ipaddr, unsigned char* dest_mac){
	int i;

	for(i = 0; i < 10; i++){
		if(!strcmp(dest_ipaddr, rt.destination_ipaddress[i])){
			break;
		}
	}	
	memcpy(dest_mac, rt.next_hop[i], ETH_ALEN);
}

//use when new odr message(including rrep) receive
void updateNewDest(struct routing_table *rt, unsigned char *dest_ipaddr, unsigned char *next_h, int h_num){
	int i, index, isExistDest;

	//two senario: the destination exists or not exists
	isExistDest = 0;
	for(i = 0; i < 10; i++){
		if(rt -> destination_ipaddress[i][0] == '\0'){
			break;
		}
		if(!strcmp(dest_ipaddr, rt -> destination_ipaddress[i])){
			index = i;
			isExistDest = 1;
		}
	}

	if(isExistDest){
		if(h_num < rt -> hop_number[index]){
			memcpy(rt -> next_hop[index], next_h, ETH_ALEN);
			rt -> hop_number[index] = h_num;
			rt -> stale_time[index] = time(NULL);
		}
	}else{
		memcpy(rt -> destination_ipaddress[i], dest_ipaddr, INET_ADDRSTRLEN);
		memcpy(rt -> next_hop[i], next_h, ETH_ALEN);
		rt -> hop_number[i] = h_num;
		rt -> stale_time[i] = time(NULL);
	}
}


