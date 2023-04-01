#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netinet/ether.h>

#define PROMISC_MODE 1
#define MULTICAST_MODE 2

void processPayload(char *payload, int payload_len) {
    printf("Payload length: %d\n", payload_len);
    printf("Payload: ");
    for (int i = 0; i < payload_len; i++) {
        printf("%c", payload[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    int sockfd, ret;
    char buffer[ETH_FRAME_LEN];
    struct sockaddr_ll socket_address;
    socklen_t saddr_len = sizeof(socket_address);

    int opt = argv[1][0] - '0';

    // create the socket
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // set the socket address
    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_ifindex = if_nametoindex("eth0");
    socket_address.sll_protocol = htons(ETH_P_ALL);
    socket_address.sll_pkttype = PACKET_MULTICAST;
    socket_address.sll_addr[0] = 0xff;
    socket_address.sll_addr[1] = 0xff;
    socket_address.sll_addr[2] = 0xff;
    socket_address.sll_addr[3] = 0xff;
    socket_address.sll_addr[4] = 0xff;
    socket_address.sll_addr[5] = 0xff;
    socket_address.sll_halen = ETH_ALEN;
    socket_address.sll_hatype = 0;

    // bind the socket to the interface
    ret = bind(sockfd, (struct sockaddr *)&socket_address, saddr_len);
    if (ret == -1) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    switch (opt) {
        case PROMISC_MODE: {
            // set the socket to promiscuous mode
            struct packet_mreq mr;
            memset(&mr, 0, sizeof(mr));
            mr.mr_ifindex = if_nametoindex("eth0");
            mr.mr_type = PACKET_MR_PROMISC;

            if (setsockopt(sockfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0) {
                perror("setsockopt");
                return 1;
            }
            break;
        }
        case MULTICAST_MODE: {
            // set the socket to multicast mode
            struct packet_mreq mr;
            memset(&mr, 0, sizeof(mr));
            mr.mr_ifindex = if_nametoindex("eth0");
            mr.mr_type = PACKET_MR_MULTICAST;
            mr.mr_alen = ETH_ALEN;
            mr.mr_address[0] = 0xff;
            mr.mr_address[1] = 0xff;
            mr.mr_address[2] = 0xff;
            mr.mr_address[3] = 0xff;
            mr.mr_address[4] = 0xff;
            mr.mr_address[5] = 0xff;

            if (setsockopt(sockfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0) {
                perror("setsockopt");
                return 1;
            }
            break;
        }
    }

    // receive and print frames
    while (1) {
        ret = recvfrom(sockfd, buffer, ETH_FRAME_LEN, 0, (struct sockaddr *)&socket_address, &saddr_len);
        if (ret == -1) {
            perror("recvfrom");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        processPayload(buffer + sizeof(struct ether_header), ret - sizeof(struct ether_header));
    }

    close(sockfd);
    return 0;
}
