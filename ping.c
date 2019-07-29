#include <stdio.h>
#include <stdlib.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include<netinet/ip_icmp.h>
#include <string.h>
#include <arpa/inet.h>

int pingSweep();

struct iphdr *ip;
struct icmphdr *icmp;
struct ifreq ifr;

struct subnet{
    struct in_addr ip;
    struct in_addr netmask;
    /*struct in_addr wildcart;
    struct in_addr broadcast;
    struct in_addr netID;*/

    uint32_t ipDec;
    uint32_t netmaskDec;
    uint32_t wildcartDec;
    uint32_t broadcastDec;
    uint32_t netIDDec;
}subnet;

int main() {
    pingSweep();

}

int pingSweep() {

    int sock;

    char chnl[]="wlan0";

    sock=socket(AF_INET, SOCK_DGRAM, 0);
    if (sock<0){
        printf("ERROR");
        return 0;
    }

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, chnl, IFNAMSIZ - 1);
    ioctl(sock, SIOCGIFADDR, &ifr);
    subnet.ip=((struct sockaddr_in *)&ifr.ifr_addr )->sin_addr;
    ioctl(sock, SIOCGIFNETMASK, &ifr);
    subnet.netmask=((struct sockaddr_in *)&ifr.ifr_addr )->sin_addr;
    close(sock);

    subnet.ipDec = inet_addr(inet_ntoa(subnet.ip));
    subnet.netmaskDec=inet_addr(inet_ntoa(subnet.netmask));
    subnet.wildcartDec=~subnet.netmaskDec;
    subnet.netIDDec= subnet.ipDec & subnet.netmaskDec;
    subnet.broadcastDec= subnet.netIDDec | subnet.wildcartDec;

    uint16_t psize = sizeof(*icmp)+sizeof(*ip);
    static char *packet;
    packet=malloc(psize);
    ip= (struct iphdr*) packet;
    icmp= (struct icmphdr*)(ip+1);

    unsigned int id = (unsigned int)rand();
    unsigned int seq = (unsigned int)rand();

    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->tot_len = htons(psize);
    ip->id = id;
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_ICMP;
    ip->saddr = subnet.ipDec;
    icmp->type = 8;
    icmp->code = 0;
    icmp->un.echo.id=id;
    icmp->un.echo.sequence=seq;
    icmp->checksum=0;

    sock=socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    struct sockaddr_in destIP;
    destIP.sin_family=AF_INET;
    uint32_t destIPDec=0;
     unsigned char *buff=malloc(psize);
    ssize_t data;
    for (int i=1; destIPDec != subnet.broadcastDec; i++) {
        destIPDec = ntohl(htonl(subnet.netIDDec) +i);
        ip->daddr = destIPDec;
        destIP.sin_addr.s_addr=destIPDec;
        socklen_t destLen = sizeof(destIP);
        sendto(sock, packet, psize, 0, (struct sockaddr*) &destIP, destLen);
        printf("3244");
        data = recvfrom(sock, buff, 65536, 0, (struct sockaddr*) &destIP, &destLen);
        printf("324");
        if(data <0 )
        {
            printf("Failed to get packets\n");
            return 1;
        }

    }
    struct in_addr bc_addr;
    bc_addr.s_addr = subnet.broadcastDec;
    printf("\nbroadcast %s", inet_ntoa(bc_addr));
    bc_addr.s_addr = ntohl(htonl(subnet.broadcastDec) -1);
    printf("\nbroadcast %s", inet_ntoa(bc_addr));

    struct in_addr nid_addr;
    nid_addr.s_addr = subnet.netIDDec;
    printf("\nnetID %s", inet_ntoa(nid_addr));


    return 0;
}
