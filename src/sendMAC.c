/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include<unistd.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define DEFAULT_IF	"eth0"
#define BUF_SIZ		1514

#define CORRUPT_W_ARP 1
#define ADD_ARP_N_IP 2
#define ADD_IP 		3
#define ADD_N_CORRUPT_IP 4
#define ADD_ARP 5
#define SMALLER_PKT 6
#define SET_AS_IP 7

void writePayload(char *sendbuf, int *tx_len, char* payload, int payload_len) {
	for (int i = 0; i < 10 * payload_len; i++) {
		sendbuf[(*tx_len)++] = payload[i % 10];
	}
	sendbuf[(*tx_len)++] = '\0';
}

void createEthernetHeader(char *sendbuf, int* tx_len, char* src, char* dest) {
	/* Construct the Ethernet header */
	memset(sendbuf, 0, BUF_SIZ);

	struct ether_header *eh = (struct ether_header *) sendbuf;

	/* Ethernet header */
	memcpy(eh->ether_shost, (uint8_t *)src, ETH_ALEN);
	memcpy(eh->ether_dhost, (uint8_t *)dest, ETH_ALEN);

	/* Ethertype field */
	eh->ether_type = htons(0x880B);
	*tx_len += sizeof(struct ether_header);
};

void setAsARP(char *sendbuf) {
	struct ether_header *eh = (struct ether_header *) sendbuf;
	eh->ether_type = htons(0x0806);
}

void setAsIP(char *sendbuf) {
	struct ether_header *eh = (struct ether_header *) sendbuf;
	eh->ether_type = htons(0x0800);
}

void createARPHeader(char *sendbuf, int *tx_len, char* src, char* dest) {
	struct ether_header *eh = (struct ether_header *) sendbuf;
	eh->ether_type = htons(0x0806);

	/* ARP header */
	struct ether_arp *ea = (struct ether_arp *) (sendbuf + sizeof(struct ether_header));
	ea->arp_hrd = htons(ARPHRD_ETHER);
	ea->arp_pro = htons(ETH_P_IP);
	ea->arp_hln = ETH_ALEN;
	ea->arp_pln = 4;
	ea->arp_op = htons(ARPOP_REQUEST);
	memcpy(ea->arp_sha, (uint8_t *)src, ETH_ALEN);
	memcpy(ea->arp_tha, (uint8_t *)dest, ETH_ALEN);
	*tx_len += sizeof(struct ether_arp);
}

void createIPHeader(char *sendbuf, int *tx_len, char* src, char* dest) {
	struct ether_header *eh = (struct ether_header *) sendbuf;
	eh->ether_type = htons(0x0800);

	/* IP header */
	struct iphdr *iph = (struct iphdr *) (sendbuf + sizeof(struct ether_header));
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = htons(sizeof(struct iphdr));
	iph->id = htons(54321);
	iph->frag_off = 0;
	iph->ttl = 64;
	iph->protocol = 0x88;
	iph->check = 0;
	iph->saddr = inet_addr(src);
	iph->daddr = inet_addr(dest);
	*tx_len += sizeof(struct iphdr);
}

void corruptIPHeaderLength(char *sendbuf) {
	struct iphdr *iph = (struct iphdr *) (sendbuf + sizeof(struct ether_header));
	iph->tot_len = htons(0x1234);
}

int main(int argc, char *argv[])
{
	char MAC_addr[6] = { 0x52, 0x54, 0x00, 0x12, 0x34, 0x58 };
	int sockfd;
	struct ifreq if_idx;
	struct ifreq if_mac;
	int tx_len = 0;
	char sendbuf[BUF_SIZ];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct sockaddr_ll socket_address;
	char ifName[IFNAMSIZ];
	
	/* Get interface name */
	strcpy(ifName, DEFAULT_IF);

	/* Open RAW socket to send on */
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
	    perror("socket");
	}

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
	    perror("SIOCGIFINDEX");
	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
	    perror("SIOCGIFHWADDR");

	int option = argv[1] ? atoi(argv[1]) : 0;

	createEthernetHeader(sendbuf, &tx_len, if_mac.ifr_hwaddr.sa_data, MAC_addr);

	switch (option) {
		case CORRUPT_W_ARP:
			setAsARP(sendbuf);
			break;
		case ADD_ARP_N_IP:
			createIPHeader(sendbuf, &tx_len, "0.0.0.0", "231.0.0.1");
			createARPHeader(sendbuf, &tx_len, if_mac.ifr_hwaddr.sa_data, MAC_addr);
			break;
		case ADD_IP:
			createIPHeader(sendbuf, &tx_len, "0.0.0.0", "231.0.0.1");
			break;
		case ADD_N_CORRUPT_IP:
			createIPHeader(sendbuf, &tx_len, "0.0.0.0", "231.0.0.1");
			corruptIPHeaderLength(sendbuf);
			break;
		case ADD_ARP:
			createARPHeader(sendbuf, &tx_len, if_mac.ifr_hwaddr.sa_data, MAC_addr);
			break;
		case SET_AS_IP:
			setAsIP(sendbuf);
			break;
		default:
			break;	
	}

	if (option != SMALLER_PKT) 
		writePayload(sendbuf, &tx_len, "Hello World", strlen("Hello World"));

	/* Index of the network device */
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	memcpy(socket_address.sll_addr, MAC_addr, ETH_ALEN);
	socket_address.sll_protocol = htons(ETH_P_ALL);
	socket_address.sll_family = AF_PACKET;
	socket_address.sll_pkttype = 0;
	socket_address.sll_hatype = 0;

	/* Send packet */
	while (1) {
		if (sendto(sockfd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
		    printf("Send failed\n");
		sleep(2);
	}
	return 0;
}
