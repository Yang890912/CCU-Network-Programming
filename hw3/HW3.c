#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <time.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#define MAC_ADDRSTRLEN 200
#define IP_ADDRSTRLEN 200

void print_pkthdr(char *time, int caplen, int len)
{
    printf("\tTime: %s\n", time);
    printf("\tCapture Length: %d bytes\n", caplen);
    printf("\tLength: %d bytes\n", len);
}

char *mac_ntoa(u_char *d)
{
    static char str[MAC_ADDRSTRLEN];

    snprintf(str, sizeof(str), "%02x:%02x:%02x:%02x:%02x:%02x", d[0], d[1], d[2], d[3], d[4], d[5]);

    return str;
} // end mac_ntoa

int main(int argc, char **argv)
{
    // 參數至少2個
    if (argc < 2)
    {
        printf("usage: ./HW3 -option <file>\n");
        exit(1);
    }

    char errbuf[PCAP_ERRBUF_SIZE];
    const char *filename = strdup(argv[argc - 1]); // 讀檔名
    int pkt_cnt = 0;

    pcap_t *handle = pcap_open_offline(filename, errbuf); // 打開.pcap檔

    if (!handle)
    {
        fprintf(stderr, "pcap_open_offline(): %s\n", errbuf);
        exit(1);
    }
    printf("Open: %s\n", filename);

    while (1)
    {
        // 抓封包
        struct pcap_pkthdr *header = NULL;
        const u_char *content = NULL;
        struct ether_header *eth = NULL;
        struct ip *ip_hdr = NULL;
        struct tcphdr *tcp_hdr = NULL;
        struct udphdr *udp_hdr = NULL;
        time_t local_tv_sec;
        struct tm *ltime;

        int ret = pcap_next_ex(handle, &header, (const u_char **)&content);

        // 檢查狀態
        if (ret == 0)
        {
            printf("Timeout\n");
            continue;
        }
        else if (ret == -1)
        {
            fprintf(stderr, "pcap_next_ex(): %s\n", pcap_geterr(handle));
            exit(1);
        }
        else if (ret == -2)
        {
            printf("No more packet from file\n");
            return 0;
        }

        // ret = 1 才會到這行
        local_tv_sec = header->ts.tv_sec;
        ltime = localtime(&local_tv_sec);
        char timestr[100];
        strftime(timestr, sizeof(timestr), "%c", ltime);

        pkt_cnt++;
        printf("[No. %d]\n", pkt_cnt);
        print_pkthdr(timestr, header->caplen, header->len); // 輸出hdr資訊, caplen->捕捉長度, len->實際長度

        eth = (struct ether_header *)content;
        char dst_mac_addr[MAC_ADDRSTRLEN]; // 200 bytes
        char src_mac_addr[MAC_ADDRSTRLEN];

        // copy header
        strncpy(dst_mac_addr, mac_ntoa(eth->ether_dhost), sizeof(dst_mac_addr));
        strncpy(src_mac_addr, mac_ntoa(eth->ether_shost), sizeof(src_mac_addr));
        u_int16_t type = ntohs(eth->ether_type);

        printf("[Ethernet Frame]\n");
        printf("+-------------------------+-------------------------+\n");
        printf("|   MAC Source address    | MAC Destination address |\n");
        printf("+-------------------------+-------------------------+\n");
        printf("| %23s | %23s |\n", src_mac_addr, dst_mac_addr);
        printf("+-------------------------+-------------------------+\n");

        if (type == ETHERTYPE_IP) // IPv4
        {
            ip_hdr = (struct ip *)(content + ETHER_HDR_LEN); //  packet_conent+14 可以得到IP的header的地址
            char ip_src[IP_ADDRSTRLEN];                      // 200 bytes
            char ip_dst[IP_ADDRSTRLEN];
            inet_ntop(AF_INET, &(ip_hdr->ip_src), ip_src, IP_ADDRSTRLEN); // inet_ntop()函数用于将网络字节序的二进制地址转换成文本字符串
            inet_ntop(AF_INET, &(ip_hdr->ip_dst), ip_dst, IP_ADDRSTRLEN);

            if (ip_hdr->ip_p == IPPROTO_TCP) // protocal TCP
            {
                unsigned int sport, dport;
                tcp_hdr = (struct tcphdr *)(content + ETHER_HDR_LEN + ip_hdr->ip_hl * 4); // packet_conent+14+ip_hdr_len*4 可以得到TCP的header的地址
                sport = ntohs(tcp_hdr->th_sport);
                dport = ntohs(tcp_hdr->th_dport);

                printf("+-------------------------+-------------------------+\n");
                printf("|     Ethernet type       |         Protocol        |\n");
                printf("+-------------------------+-------------------------+\n");
                printf("|          IP             |           TCP           |\n");
                printf("+-------------------------+-------------------------+\n");
                printf("+-------------------------+-------------------------+-------------------------+-------------------------+\n");
                printf("|   IP Source address     | IP Destination address  |      Source Port        |     Destination Port    |\n");
                printf("+-------------------------+-------------------------+-------------------------+-------------------------+\n");
                printf("| %23s | %23s | %23d | %23d |\n", ip_src, ip_dst, sport, dport);
                printf("+-------------------------+-------------------------+-------------------------+-------------------------+\n");
            }
            else if (ip_hdr->ip_p == IPPROTO_UDP) // protocal UDP
            {
                unsigned int sport, dport;
                udp_hdr = (struct udphdr *)(content + ETHER_HDR_LEN + ip_hdr->ip_hl * 4); // packet_conent+14+ip_hdr_len*4 可以得到UDP的header的地址
                sport = ntohs(udp_hdr->uh_sport);
                dport = ntohs(udp_hdr->uh_dport);

                printf("+-------------------------+-------------------------+\n");
                printf("|     Ethernet type       |         Protocol        |\n");
                printf("+-------------------------+-------------------------+\n");
                printf("|          IP             |           UDP           |\n");
                printf("+-------------------------+-------------------------+\n");
                printf("+-------------------------+-------------------------+-------------------------+-------------------------+\n");
                printf("|   IP Source address     | IP Destination address  |      Source Port        |     Destination Port    |\n");
                printf("+-------------------------+-------------------------+-------------------------+-------------------------+\n");
                printf("| %23s | %23s | %23d | %23d |\n", ip_src, ip_dst, sport, dport);
                printf("+-------------------------+-------------------------+-------------------------+-------------------------+\n");
            }
        }
        else if (type == ETHERTYPE_IPV6) // IPv6
        {
            printf("+------------------------+\n");
            printf("|     Ethernet type      |\n");
            printf("+------------------------+\n");
            printf("|          IPv6          |\n");
            printf("+------------------------+\n");
        }
        else if (type == ETHERTYPE_ARP) // ARP
        {
            printf("+------------------------+\n");
            printf("|     Ethernet type      |\n");
            printf("+------------------------+\n");
            printf("|          ARP           |\n");
            printf("+------------------------+\n");
        }
        else if (type == ETHERTYPE_PUP) // PUP
        {
            printf("+------------------------+\n");
            printf("|     Ethernet type      |\n");
            printf("+------------------------+\n");
            printf("|          PUP           |\n");
            printf("+------------------------+\n");
        }
        else if (type == ETHERTYPE_REVARP) // REVARP
        {
            printf("+------------------------+\n");
            printf("|     Ethernet type      |\n");
            printf("+------------------------+\n");
            printf("|         REVARP         |\n");
            printf("+------------------------+\n");
        }
        else
        {
            printf("[type] 0x%04X\n", type);
            printf("[Warning] No support such type\n");
        }

    } // end while(1)

    pcap_close(handle);
}