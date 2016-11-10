#include <mavkit/MavlinkUDP.h>
#include <stdexcept>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>

#define BUFFER_LENGTH 512

//----------------------------------------------------------------------------//
uint64_t microsSinceEpoch()
{
    struct timeval tv;
    uint64_t micros = 0;

    gettimeofday(&tv, NULL);
    micros = ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;

    return micros;
}
//----------------------------------------------------------------------------//
MavlinkUDP::MavlinkUDP(std::string target_ip, int target_port, int local_port)
{
    struct sockaddr_in locAddr;
    memset(&locAddr, 0, sizeof(struct sockaddr_in));
    locAddr.sin_family = AF_INET;
    locAddr.sin_port = htons(local_port);
    locAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind socket to local_port
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(bind(sock,(struct sockaddr *)&locAddr, sizeof(struct sockaddr)) == -1)
    {
        close(sock);
        throw std::logic_error(std::string("error bind failed: ") + strerror(errno));
    }

    //make socket non blocking
    if (fcntl(sock, F_SETFL, O_NONBLOCK | FASYNC) < 0)
    {
        close(sock);
        throw std::logic_error(std::string("error setting nonblocking: ") + strerror(errno));
    }

    memset(&gcAddr, 0, sizeof(struct sockaddr_in));
    gcAddr.sin_family = AF_INET;
    gcAddr.sin_port = htons(target_port);
    gcAddr.sin_addr.s_addr = inet_addr(target_ip.c_str());

    std::cout << "to   " << target_ip << ":" << target_port << std::endl;
    std::cout << "from " << inet_ntoa(locAddr.sin_addr) << ":" << local_port << std::endl;
}
//----------------------------------------------------------------------------//
MavlinkUDP::~MavlinkUDP()
{
    close(sock);
}
//----------------------------------------------------------------------------//
bool MavlinkUDP::is_valid_ip(const char* ip)
{
    char* dst[INET_ADDRSTRLEN];
    return inet_pton(AF_INET, ip, dst) == 1;
}
//----------------------------------------------------------------------------//
bool MavlinkUDP::send_message(mavlink_message_t &msg)
{
    uint8_t buf[BUFFER_LENGTH];
    uint16_t len, bytes_sent;

    len = mavlink_msg_to_send_buffer(buf, &msg);
    bytes_sent = sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));

    return len == bytes_sent;
}
//----------------------------------------------------------------------------//
bool MavlinkUDP::receive_message(mavlink_message_t &msg)
{
    uint8_t buf[BUFFER_LENGTH];
    socklen_t fromlen = sizeof(struct sockaddr);

    ssize_t recsize = recvfrom(sock, (void *)buf, BUFFER_LENGTH, 0, (struct sockaddr *)&gcAddr, &fromlen);

    for(int i = 0 ; i < recsize ; i++)
    {
        if(mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status))
        {
            if(i < recsize-1)
                std::cout << "WARNING : udp packet bytes are wasted." << std::endl;
            return true;
        }
    }

    return false;
}
//----------------------------------------------------------------------------//
