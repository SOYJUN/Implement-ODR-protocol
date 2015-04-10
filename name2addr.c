#include "unp.h"

int name2addr(char * hostname, char * hostaddr)
{
	struct in_addr		inaddr;
	struct hostent		*hptr;
	char				addr[INET_ADDRSTRLEN];
	char				**pptr;				

	hptr = gethostbyname(hostname);
	if (hptr==NULL){
		printf("[ERROR]: Cannot find IP address of %s, client terminated.\n", hostname);
		return -1;
	}

    pptr = hptr->h_addr_list;
	
	inet_ntop(AF_INET, *pptr, addr, sizeof(addr));
	
	strcpy(hostaddr, addr);

	return 1;
}
