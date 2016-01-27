#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "sock.h"

#define DEF_DISCOVER_PORT	2000
#define DEF_RESET_PIN       2
#define DEF_CHUNK_SIZE      8192
#define MAX_CHUNK_SIZE      8192

#define MAX_IF_ADDRS        10

typedef int XbeeAddrList;

int chunkSize = DEF_CHUNK_SIZE;

int load(const char *ipAddr, char *fileName, int resetPin);
int sendRequest(SOCKADDR_IN *addr, uint8_t *req, int reqSize, uint8_t *res, int resMax);
void dumpHdr(const uint8_t *buf, int size);
int discover(XbeeAddrList &addrs, int timeout);
int discover1(IFADDR *ifaddr, XbeeAddrList &addrs, int timeout);
void Usage();

int main(int argc, char *argv[])
{
    XbeeAddrList addrs;
    char *infile = NULL;
    char *ipaddr = NULL;
    int resetPin = DEF_RESET_PIN;
    int ret, i;

    /* get the arguments */
    for (i = 1; i < argc; ++i) {

        /* handle switches */
        if (argv[i][0] == '-') {
            switch(argv[i][1]) {
            case 'c':
                if (argv[i][2])
                    chunkSize = atoi(&argv[i][2]);
                else if (++i < argc)
                    chunkSize = atoi(argv[i]);
                else
                    Usage();
                if (chunkSize < 1 || chunkSize > MAX_CHUNK_SIZE) {
                    printf("error: chunk size must be between 1 and %d\n", MAX_CHUNK_SIZE);
                    return 1;
                }
                break;
            case 'i':
                if (argv[i][2])
                    ipaddr = &argv[i][2];
                else if (++i < argc)
                    ipaddr = argv[i];
                else
                    Usage();
                break;
            case 'r':
                if (argv[i][2])
                    resetPin = atoi(&argv[i][2]);
                else if (++i < argc)
                    resetPin = atoi(argv[i]);
                else
                    Usage();
                break;
            case '?':
                /* fall through */
            default:
                Usage();
                break;
            }
        }

        /* handle the input filename */
        else {
            if (infile)
                Usage();
            infile = argv[i];
        }
    }
    
    if (infile) {
        if (!ipaddr) {
            printf("error: must specify IP address or host name with -i\n");
            return 1;
        }
        if (load(ipaddr, infile, resetPin) < 0)
            return 1;
    }
    
    else {
        if ((ret = discover(addrs, 2000)) < 0) {
            printf("error: discover failed: %d\n", ret);
            return 1;
        }
    }
    
    return 0;
}

void Usage()
{
    printf("\
usage: espload\n\
         [ -c <size> ]     chunk size (default is %d)\n\
         [ -i <addr> ]     IP address or host name of module to load\n\
         [ -r <pin> ]      pin to use for resetting the Propeller (default is %d)\n\
         [ <name> ]        file to load (discover modules if not given)\n", DEF_CHUNK_SIZE, DEF_RESET_PIN);
    exit(1);
}

int load(const char *hostName, char *fileName, int resetPin)
{
    uint8_t buffer[MAX_CHUNK_SIZE], *p;
    int imageSize, remaining, cnt;
    SOCKADDR_IN addr;
    uint8_t *image;
    FILE *fp;
    
    if (GetInternetAddress(hostName, 80, &addr) != 0) {
        printf("error: invalid host name or IP address '%s'\n", hostName);
        return -1;
    }
    
    /* open the image file */
    if (!(fp = fopen(fileName, "rb"))) {
        printf("error: can't open '%s'\n", fileName);
        return -1;
    }
    
    /* get the size of the binary file */
    fseek(fp, 0, SEEK_END);
    imageSize = (int)ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* allocate space for the file */
    if (!(image = (uint8_t *)malloc(imageSize)))
        return -1;

    /* read the entire image into memory */
    if ((int)fread(image, 1, imageSize, fp) != imageSize) {
        free(image);
        return -1;
    }
    
    /* close the file */
    fclose(fp);

    cnt = snprintf((char *)buffer, sizeof(buffer), "\
POST /load-begin?size=%d&reset-pin=%d HTTP/1.1\r\n\
\r\n", imageSize, resetPin);
    
    if ((cnt = sendRequest(&addr, buffer, cnt, buffer, sizeof(buffer))) == -1) {
        printf("error: load-begin request failed\n");
        return -1;
    }
    
    p = image;
    remaining = imageSize;
    while (remaining > 0) {
        int hdrCnt = snprintf((char *)buffer, sizeof(buffer), "\
POST /load-data HTTP/1.1\r\n\
\r\n");
        if ((cnt = remaining) > chunkSize - hdrCnt)
            cnt = chunkSize - hdrCnt;
        memcpy(&buffer[hdrCnt], p, cnt);
        if (sendRequest(&addr, buffer, hdrCnt + cnt, buffer, sizeof(buffer)) == -1) {
            printf("error: load-data request failed\n");
            return -1;
        }
        p += cnt;
        remaining -= cnt;
    }
        
    cnt = snprintf((char *)buffer, sizeof(buffer), "\
POST /load-end?command=run HTTP/1.1\r\n\
\r\n");
    
    if ((cnt = sendRequest(&addr, buffer, cnt, buffer, sizeof(buffer))) == -1) {
        printf("error: load-end request failed\n");
        return -1;
    }
    
    return 0;
}

// should try:
// Connection: keep-alive

int sendRequest(SOCKADDR_IN *addr, uint8_t *req, int reqSize, uint8_t *res, int resMax)
{
    SOCKET sock;
    int cnt;
    
    if (ConnectSocket(addr, &sock) != 0) {
        printf("error: connect failed\n");
        return -1;
    }
    
    printf("REQ:\n");
    dumpHdr(req, reqSize);
    
    if (SendSocketData(sock, req, reqSize) != reqSize) {
        printf("error: send request failed\n");
        return -1;
    }
    
    if ((cnt = ReceiveSocketDataTimeout(sock, res, resMax, 10000)) == -1) {
        printf("error: receive response failed\n");
        return -1;
    }
    
    printf("RES:\n");
    dumpHdr(res, cnt);
        
    CloseSocket(sock);
    
    return cnt;
}
    
void dumpHdr(const uint8_t *buf, int size)
{
    bool startOfLine = true;
    const uint8_t *p = buf;
    while (p < buf + size) {
        if (*p == '\r') {
            if (startOfLine)
                break;
            startOfLine = true;
            putchar('\n');
        }
        else if (*p != '\n') {
            startOfLine = false;
            putchar(*p);
        }
        ++p;
    }
    putchar('\n');
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
    SOCKADDR_IN addr;
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
        if ((cnt = ReceiveSocketDataAndAddress(sock, rxBuf, sizeof(rxBuf), &addr)) < 0) {
            printf("error: ReceiveSocketData failed\n");
            CloseSocket(sock);
            return -3;
        }
        
        printf("from %s got: %s", AddressToString(&addr), rxBuf);
    }
    
    /* close the socket */
    CloseSocket(sock);
    
    /* return successfully */
    return 0;
}

