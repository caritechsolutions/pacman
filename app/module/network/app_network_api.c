#include "app.h"
#include "app_module.h"
#include "app_str_id.h"

#if NETWORK_SUPPORT
static char *s_network_error_tip_str[ERROR_TYPE_TOTAL] = {
            " ",     //no tip
    STR_ID_INVALID_IP,
    STR_ID_SUB_ERROE,
    STR_ID_GATE_WAY_ERROR,
    STR_ID_DNS1_ERROR,
    STR_ID_DNS2_ERROR,
    STR_ID_DNS_COMMON,
    STR_ID_NET_ADDR_ERROR,// ip&mask!= gateway&mask
    STR_ID_INVALID, //other error
};


int app_network_ip_valid(unsigned int ip)
{
    if(((ip&0x000000ff)==0)||((ip&0x000000ff)>=224))
        return 0;//error
    else
        return 1;
}

char *app_get_network_para_err_tip(NetWorkParamsError error_type)
{
    return s_network_error_tip_str[error_type];
}

void app_network_ip_simplify(char *buf, char *ip)
{
	char *p;
	char tmp[16];
	int num[4] = {0, 0, 0, 0};

	strcpy(tmp, ip);
	num[0] = atoi(tmp);

	p = strchr(tmp, '.');
	p++;
	num[1] = atoi(p);

	p = strchr(p, '.');
	p++;
	num[2] = atoi(p);

	p = strchr(p, '.');
	p++;
	num[3] = atoi(p);

	sprintf(buf, "%03d.%03d.%03d.%03d", num[0], num[1], num[2], num[3]);
}
void app_network_ip_completion(char *ip)
{
	char *p;
	char buf[15];
	int num[4] = {0, 0, 0, 0};

	strcpy(buf, ip);

	num[0] = atoi(buf);

	p = strchr(buf, '.');
	if(p== NULL)
		goto END;
	p++;
	num[1] = atoi(p);

	p = strchr(p, '.');
	if(p== NULL)
		goto END;
	p++;
	num[2] = atoi(p);

	p = strchr(p, '.');
	if(p== NULL)
		goto END;
	p++;
	num[3] = atoi(p);

END:
	memset(ip, 0, 16);
	sprintf(ip, "%03d.%03d.%03d.%03d", num[0], num[1], num[2], num[3]);
}
status_t app_if_name_convert(const char *if_name, char *display_name)
{
    uint8_t num_offset = 0;
    int dev_num = 0;
    char *name_head = NULL;

    if((if_name == NULL) || (display_name == NULL))
    {
        return GXCORE_ERROR;
    }

    if(strncmp(if_name, "eth", 3) == 0) //wired
    {
#if (MOD_3G_SUPPORT > 0)
#ifdef LINUX_OS
        IfType dev_type = app_if_dev_check_type(if_name);
        if(dev_type == IF_TYPE_ETH3G)
        {
            name_head = STR_ID_3G;
            num_offset = 5;
        }
        else
#endif
#endif
        {
            name_head = STR_ID_WIRED;
            num_offset = 3;
        }
    }
    else if (strncmp(if_name, "ra", 2) == 0)
    {
        name_head = STR_ID_WIFI;
        num_offset = 2;
    }
    else if (strncmp(if_name, "wlan", 4) == 0)
    {
        name_head = STR_ID_WIFI;
        num_offset = 4;
    }
    else if (strncmp(if_name, "USB3G", 5) == 0)
    {
        name_head = STR_ID_3G;
        num_offset = 5;
    }
    else if (strncmp(if_name, "usb0", 4) == 0)
    {
        name_head = STR_ID_RNDIS;
        num_offset = 4;
    }
    else
    {
        strcpy(display_name, if_name);
    }

    if(name_head != NULL)
    {
        dev_num = atoi(if_name + num_offset);
        if(dev_num > 0)
        {
            sprintf(display_name, "%s-%d", name_head, dev_num);
        }
        else
        {
            sprintf(display_name, "%s", name_head);
        }
    }

    return GXCORE_SUCCESS;
}


char *app_inet_ntoa(struct in_addr in)
{
    static char s_add_buffer[18] = {0};
    unsigned char *bytes = (unsigned char *) &in;
    memset(s_add_buffer, 0 , 18);
    sprintf (s_add_buffer, "%03d.%03d.%03d.%03d",
            bytes[0], bytes[1], bytes[2], bytes[3]);
    return s_add_buffer;
}

status_t app_get_ip_addr(const char *if_name, char *ipaddr)
{
    int skfd;
    struct ifreq ifr;
    struct sockaddr_in *sa = NULL;

    if((if_name == NULL) || (ipaddr == NULL))
    {
        return GXCORE_ERROR;
    }

    if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return GXCORE_ERROR;
    }

    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
    if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0)
    {
        close(skfd);
        return GXCORE_ERROR;
    }

	sa = (struct sockaddr_in *)(&ifr.ifr_addr);
    strcpy(ipaddr, app_inet_ntoa(sa->sin_addr));

	close(skfd);
    return GXCORE_SUCCESS;
}

status_t app_set_ip_addr(const char *if_name, const char *ipaddr)
{
    int skfd;
    struct ifreq ifr;
    struct sockaddr_in *sa = NULL;

    if((if_name == NULL) || (ipaddr == NULL))
    {
        return GXCORE_ERROR;
    }

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return GXCORE_ERROR;
    }

    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
	sa = (struct sockaddr_in*)&ifr.ifr_addr;
	sa->sin_family = AF_INET;
	inet_aton(ipaddr, &sa->sin_addr);
    if (ioctl(skfd, SIOCSIFADDR, &ifr) < 0)
    {
        close(skfd);
        return GXCORE_ERROR;
    }

	close(skfd);
    return GXCORE_SUCCESS;
}

status_t app_get_bcast_addr(const char *if_name, char *bcast)
{
    int skfd;
    struct ifreq ifr;
    struct sockaddr_in *sa = NULL;

    if((if_name == NULL) || (bcast == NULL))
    {
        return GXCORE_ERROR;
    }

    if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return GXCORE_ERROR;
    }

    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
    if (ioctl(skfd, SIOCGIFBRDADDR, &ifr) < 0)
    {
        close(skfd);
        return GXCORE_ERROR;
    }
	sa = (struct sockaddr_in *)(&ifr.ifr_broadaddr);
    strcpy(bcast, app_inet_ntoa(sa->sin_addr));

	close(skfd);
    return GXCORE_SUCCESS;
}

status_t app_set_bcast_addr(const char *if_name, const char *bcast)
{
    int skfd;
    struct ifreq ifr;
    struct sockaddr_in *sa = NULL;

    if((if_name == NULL) || (bcast == NULL))
    {
        return GXCORE_ERROR;
    }

    if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return GXCORE_ERROR;
    }

    strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
	sa = (struct sockaddr_in*)&ifr.ifr_broadaddr;
	sa->sin_family = AF_INET;
	inet_aton(bcast, &sa->sin_addr);
    if (ioctl(skfd, SIOCGIFBRDADDR, &ifr) < 0)
    {
        close(skfd);
        return GXCORE_ERROR;
    }

	close(skfd);
    return GXCORE_SUCCESS;
}

status_t app_get_mac_addr(const char *if_name, char *mac)
{
    int skfd;
    struct ifreq ifr;

    if((if_name == NULL) || (mac == NULL)) {
        return GXCORE_ERROR;
    }

    if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return GXCORE_ERROR;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, if_name, IFNAMSIZ);
    if (ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0) {
        close(skfd);
        return GXCORE_ERROR;
    }
	sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
            (unsigned char)ifr.ifr_hwaddr.sa_data[0],
            (unsigned char)ifr.ifr_hwaddr.sa_data[1],
            (unsigned char)ifr.ifr_hwaddr.sa_data[2],
            (unsigned char)ifr.ifr_hwaddr.sa_data[3],
            (unsigned char)ifr.ifr_hwaddr.sa_data[4],
            (unsigned char)ifr.ifr_hwaddr.sa_data[5]);

	close(skfd);
    return GXCORE_SUCCESS;
}

status_t app_set_mac_addr(const char *if_name, const char *mac)
{
    int s;
    struct ifreq ifr;
    int ret;
    unsigned int tmp[6] = {0};

    if((if_name == NULL) || (mac == NULL)) {
        return GXCORE_ERROR;
    }

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if(s < 0) {
        return GXCORE_ERROR;
    }

    sscanf(mac, "%x:%x:%x:%x:%x:%x", &tmp[0], &tmp[1], &tmp[2], &tmp[3], &tmp[4], &tmp[5]);

    ifr.ifr_hwaddr.sa_data[0] = (unsigned char)tmp[0];
    ifr.ifr_hwaddr.sa_data[1] = (unsigned char)tmp[1];
    ifr.ifr_hwaddr.sa_data[2] = (unsigned char)tmp[2];
    ifr.ifr_hwaddr.sa_data[3] = (unsigned char)tmp[3];
    ifr.ifr_hwaddr.sa_data[4] = (unsigned char)tmp[4];
    ifr.ifr_hwaddr.sa_data[5] = (unsigned char)tmp[5];

    strncpy(ifr.ifr_name, if_name, IFNAMSIZ);
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;

    ret = ioctl(s, SIOCSIFHWADDR, &ifr);
    if(ret < 0) {
        close(s);
        return GXCORE_ERROR;
    }

    close(s);
    return GXCORE_SUCCESS;
}

static int network_ioctl(int fd, int request, void* data, const char *errmsg)
{
    int r = ioctl(fd, request, data);
    if (r < 0 && errmsg)
        app_log_error("%s failed\n", errmsg);
    return r;
}

status_t app_if_up(const char *if_name)
{
    int ioctl_fd = -1;
    struct ifreq ifrequest;

    if(if_name == NULL)
        return GXCORE_ERROR;

    if((ioctl_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return GXCORE_ERROR;

    memset(&ifrequest, 0, sizeof(struct ifreq));
    strlcpy(ifrequest.ifr_name, if_name, sizeof(ifrequest.ifr_name));

    if(network_ioctl(ioctl_fd, SIOCGIFFLAGS, &ifrequest, "getting interface flags") < 0)
    {
        close(ioctl_fd);
        return GXCORE_ERROR;
    }

    if (!(ifrequest.ifr_flags & IFF_UP))
    {
        ifrequest.ifr_flags |= IFF_UP;
        /* Let user know we mess up with interface */
        app_log_debug("upping interface\n");
        if(network_ioctl(ioctl_fd, SIOCSIFFLAGS, &ifrequest, "setting interface flags") < 0)
        {
            close(ioctl_fd);
            return GXCORE_ERROR;
        }
    }

    close(ioctl_fd);

    return GXCORE_SUCCESS;
}

status_t app_if_down(const char *if_name)
{
    int ioctl_fd = -1;
    struct ifreq ifrequest;

    if(if_name == NULL)
        return GXCORE_ERROR;

    if((ioctl_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return GXCORE_ERROR;

    memset(&ifrequest, 0, sizeof(struct ifreq));
    strlcpy(ifrequest.ifr_name, if_name, sizeof(ifrequest.ifr_name));

    if(network_ioctl(ioctl_fd, SIOCGIFFLAGS, &ifrequest, "getting interface flags") < 0)
    {
        close(ioctl_fd);
        return GXCORE_ERROR;
    }

    if(ifrequest.ifr_flags & IFF_UP)
    {
        ifrequest.ifr_flags &= ~IFF_UP;
        /* Let user know we mess up with interface */
        app_log_debug("downing interface\n");
        if(network_ioctl(ioctl_fd, SIOCSIFFLAGS, &ifrequest, "setting interface flags") < 0)
        {
            close(ioctl_fd);
            return GXCORE_ERROR;
        }
    }

    close(ioctl_fd);

    return GXCORE_SUCCESS;
}

#ifdef LINUX_OS
status_t app_get_gateway(const char *if_name, char *gateway)
{
    FILE *read_fp;
    char cmdline[60] = {0};
    char temp_buf[18] = {0};
    char temp_buf2[18] = {0};
    char *p1 = NULL;
    char *p2 = NULL;
    char len = 0;

    if((if_name == NULL) || (gateway == NULL))
    {
        return GXCORE_ERROR;
    }

    strcpy(cmdline, "route -n | grep ");
    strcat(cmdline, if_name);
    strcat(cmdline, " | grep UG | awk '{print $2}'");

    read_fp = popen(cmdline, "r");
    if (read_fp != NULL)
    {
        if(fread(temp_buf, 1, 17, read_fp) > 0)
        {
            p1 = temp_buf;
            p2 = p1;
            while(p1 != NULL)
            {
               if((*p1 == '.') || (*p1 == '\0') || (*p1 == '\n'))
               {
                   len = p1 - p2;
                   if(len == 3)
                   {
                   }
                   else if(len == 2)
                   {
                       strcat(temp_buf2, "0");
                   }
                   else if(len == 1)
                   {
                       strcat(temp_buf2, "00");
                   }
                   else
                   {
                       return GXCORE_ERROR;
                   }
                   strncat(temp_buf2, p2, len);

                   if(*p1 == '.')
                   {
                       strcat(temp_buf2, ".");
                       p2 = p1 + 1;
                   }
                   else
                   {
                       break;
                   }
               }
               p1++;
            }
		    pclose(read_fp);
            strcpy(gateway, temp_buf2);
            gateway[strlen(temp_buf2)] = '\0';
			return GXCORE_SUCCESS;
        }
        pclose(read_fp);
    }
	return GXCORE_ERROR;
}

bool app_check_netlink(const char *if_name) //if_name: eth0,ra0... etc.
{
    bool ret = false;
    int skfd = -1;
    struct ifreq ifr;
    struct ethtool_value edata;
    char buffer[512] ={0};
    FILE *read_fp = NULL;

    if((if_name == NULL) || (strlen(if_name) >= 10) || (strlen(if_name) == 0))
    {
        return false;
    }

#if 0 // 使用popen在csky-4.9linux上CPU占用高，造成wifi连接超时
    char cmd_buff[100] = {0};
    sprintf(cmd_buff, "ifconfig %s 2>/dev/null | grep Ethernet 2>/dev/null", if_name);
    read_fp = popen(cmd_buff, "r");
    if (read_fp != NULL)
    {
        if (fgets(buffer, sizeof(buffer), read_fp))
            ret = true;
        pclose(read_fp);
    }
#else
#define PROC_NET_DEV       "/proc/net/dev"
    read_fp = fopen(PROC_NET_DEV, "r");

    if(read_fp != NULL)
    {
        fgets(buffer, sizeof(buffer), read_fp);
        fgets(buffer, sizeof(buffer), read_fp);

        /* Read each device line */
        while(fgets(buffer, sizeof(buffer), read_fp))
        {
            if (strstr(buffer, if_name) != NULL)
            {
                ret = true;
                break;
            }
        }
        fclose(read_fp);
    }
#endif

    if(GXCORE_ERROR == app_if_up(if_name))
        return false;

    if((ret == true) && (strncmp((char *)if_name, "eth", 3) == 0)) //wired
    {
        edata.cmd = ETHTOOL_GLINK;
        edata.data = 0;
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);
        ifr.ifr_data = (char *) &edata;

        if (( skfd = socket( AF_INET, SOCK_DGRAM, 0 )) < 0)
        {
            ret = false;
        }
        else
        {
            if(ioctl( skfd, SIOCETHTOOL, &ifr ) < 0)
            {
                ret = false;
            }

            if(edata.data != 1)
            {
                ret = false;
            }
            close(skfd);
        }
    }

    return ret;
}
#else



status_t app_get_gateway(const char *if_name, char *gateway)
{
    //TODO
    return GXCORE_ERROR;
}

bool app_check_netlink(const char *if_name) //if_name: eth0,ra0... etc.
{
    //TODO
    return true;
}
#endif
#endif

