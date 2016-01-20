#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "sock.h"

#define DEF_DISCOVER_PORT	2000

#define MAX_IF_ADDRS        10

typedef int XbeeAddrList;

int discover(XbeeAddrList &addrs, int timeout);
int discover1(IFADDR *ifaddr, XbeeAddrList &addrs, int timeout);

int main(int argc, char *argv[])
{
    XbeeAddrList addrs;
    int ret;

    if ((ret = discover(addrs, 2000)) < 0) {
        printf("error: discover failed: %d\n", ret);
        return 1;
    }
    
    return 0;
}

int discover(XbeeAddrList &addrs, int timeout)
{
    IFADDR ifaddrs[MAX_IF_ADDRS];
    int cnt, i;
    
    if ((cnt = GetInterfaceAddresses(ifaddrs, MAX_IF_ADDRS)) < 0)
        return -1;
    
    for (i = 0; i < cnt; ++i) {
        int ret;
        if ((ret = discover1(&ifaddrs[i], addrs, timeout)) < 0)
            return ret;
    }
    
    return 0;
}

int discover1(IFADDR *ifaddr, XbeeAddrList &addrs, int timeout)
{
    uint8_t txBuf[1024]; // BUG: get rid of this magic number!
    uint8_t rxBuf[1024]; // BUG: get rid of this magic number!
    SOCKADDR_IN bcastaddr;
    SOCKET sock;
    int cnt;
    
    /* create a broadcast socket */
    if (OpenBroadcastSocket(DEF_DISCOVER_PORT, &sock) != 0) {
        printf("error: OpenBroadcastSocket failed\n");
        return -2;
    }
        
    /* build a broadcast address */
    bcastaddr = ifaddr->bcast;
    bcastaddr.sin_port = htons(DEF_DISCOVER_PORT);
    
    /* send the broadcast packet */
    sprintf((char *)txBuf, "Me here! Ignore this message.\n");
    if ((cnt = SendSocketDataTo(sock, txBuf, sizeof(txBuf), &bcastaddr)) != sizeof(txBuf)) {
        perror("error: SendSocketDataTo failed");
        CloseSocket(sock);
        return -1;
    }

    /* receive Xbee responses */
    while (SocketDataAvailableP(sock, timeout)) {

        /* get the next response */
        memset(rxBuf, 0, sizeof(rxBuf));
        if ((cnt = ReceiveSocketData(sock, rxBuf, sizeof(rxBuf))) < 0) {
            printf("error: ReceiveSocketData failed\n");
            CloseSocket(sock);
            return -3;
        }
        
        printf("got: %s", rxBuf);
    }
    
    /* close the socket */
    CloseSocket(sock);
    
    /* return successfully */
    return 0;
}

