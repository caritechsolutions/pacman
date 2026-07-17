#ifdef LINUX_OS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/nl80211.h>
#include <linux/genetlink.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <wifi/gxwifi.h>

#define WIFI_DBG(...)
#define WIFI_ERR(...) printf(__VA_ARGS__)

#define MAX_INTERFACES 16

struct p2p_interface_info {
    int ifindex;
    char ifname[16];
    char addr[18];
    int wiphy_index;              // Add wiphy index
    int supports_p2p_client;
    int supports_p2p_go;
    int is_p2p_device;
};

struct scan_context {
    struct p2p_interface_info interfaces[MAX_INTERFACES];
    int count;
    int nl80211_id;
    struct nl_sock *sock;
};

// Callback function to parse interface information
static int interface_handler(struct nl_msg *msg, void *arg) {
    struct scan_context *ctx = (struct scan_context *)arg;
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

    if (ctx->count >= MAX_INTERFACES) {
        return NL_SKIP;
    }

    /* 初始化当前接口结构体 */
    memset(&ctx->interfaces[ctx->count], 0, sizeof(struct p2p_interface_info));

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), NULL);

    if (tb[NL80211_ATTR_IFINDEX]) {
        ctx->interfaces[ctx->count].ifindex = nla_get_u32(tb[NL80211_ATTR_IFINDEX]);
    }

    if (tb[NL80211_ATTR_IFNAME]) {
        strncpy(ctx->interfaces[ctx->count].ifname,
                nla_get_string(tb[NL80211_ATTR_IFNAME]), 15);
        ctx->interfaces[ctx->count].ifname[15] = '\0';
    }

    if (tb[NL80211_ATTR_MAC]) {
        unsigned char *mac = nla_data(tb[NL80211_ATTR_MAC]);
        snprintf(ctx->interfaces[ctx->count].addr, 18,
                "%02x:%02x:%02x:%02x:%02x:%02x",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    /* Get wiphy index */
    if (tb[NL80211_ATTR_WIPHY]) {
        ctx->interfaces[ctx->count].wiphy_index = nla_get_u32(tb[NL80211_ATTR_WIPHY]);
    }

    if (tb[NL80211_ATTR_IFTYPE]) {
        enum nl80211_iftype iftype = nla_get_u32(tb[NL80211_ATTR_IFTYPE]);
        if (iftype == NL80211_IFTYPE_P2P_DEVICE) {
            ctx->interfaces[ctx->count].is_p2p_device = 1;
        }
        /* 对于一般的station接口，我们也认为它可能支持P2P */
        if (iftype == NL80211_IFTYPE_STATION) {
            /* 大多数现代WiFi网卡的station接口都支持P2P */
            ctx->interfaces[ctx->count].supports_p2p_client = 1;
        }
    }

    ctx->count++;
    return NL_SKIP;
}

// Callback function to parse wiphy information
static int wiphy_handler(struct nl_msg *msg, void *arg) {
    struct scan_context *ctx = (struct scan_context *)arg;
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    int current_wiphy = -1;
    int i;

    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), NULL);

    /* Get current wiphy index */
    if (tb[NL80211_ATTR_WIPHY]) {
        current_wiphy = nla_get_u32(tb[NL80211_ATTR_WIPHY]);
        WIFI_DBG("Processing wiphy %d\n", current_wiphy);
    }

    /* Check supported interface types */
    if (tb[NL80211_ATTR_SUPPORTED_IFTYPES] && current_wiphy >= 0) {
        struct nlattr *nl_mode;
        int rem_mode;
        int supports_p2p_client = 0;
        int supports_p2p_go = 0;

        WIFI_DBG("Checking supported interface types for wiphy %d:\n", current_wiphy);

        nla_for_each_nested(nl_mode, tb[NL80211_ATTR_SUPPORTED_IFTYPES], rem_mode) {
            enum nl80211_iftype iftype = nla_type(nl_mode);

            switch(iftype) {
                case NL80211_IFTYPE_STATION:
                    WIFI_DBG("  - STATION\n");
                    break;
                case NL80211_IFTYPE_AP:
                    WIFI_DBG("  - AP\n");
                    break;
                case NL80211_IFTYPE_P2P_CLIENT:
                    WIFI_DBG("  - P2P_CLIENT\n");
                    supports_p2p_client = 1;
                    break;
                case NL80211_IFTYPE_P2P_GO:
                    WIFI_DBG("  - P2P_GO\n");
                    supports_p2p_go = 1;
                    break;
                case NL80211_IFTYPE_P2P_DEVICE:
                    WIFI_DBG("  - P2P_DEVICE\n");
                    break;
                default:
                    WIFI_DBG("  - Other type: %d\n", iftype);
                    break;
            }
        }

        WIFI_DBG("P2P support for wiphy %d: client=%d, go=%d\n",
            current_wiphy, supports_p2p_client, supports_p2p_go);

        /* Mark only interfaces belonging to current wiphy */
        for (i = 0; i < ctx->count; i++) {
            if (ctx->interfaces[i].wiphy_index == current_wiphy) {
                ctx->interfaces[i].supports_p2p_client = supports_p2p_client;
                ctx->interfaces[i].supports_p2p_go = supports_p2p_go;
                WIFI_DBG("Updated interface %s: P2P_client=%d, P2P_GO=%d\n",
                    ctx->interfaces[i].ifname, supports_p2p_client, supports_p2p_go);
            }
        }
    } else {
        WIFI_DBG("No supported interface types info for wiphy %d\n", current_wiphy);
    }

    return NL_SKIP;
}

// Get network interface information
int get_interfaces(struct scan_context *ctx) {
    struct nl_msg *msg;
    int ret;

    msg = nlmsg_alloc();
    if (!msg) {
        return -1;
    }

    genlmsg_put(msg, 0, 0, ctx->nl80211_id, 0, NLM_F_DUMP,
                NL80211_CMD_GET_INTERFACE, 0);

    ret = nl_send_auto_complete(ctx->sock, msg);
    nlmsg_free(msg);

    if (ret < 0) {
        return ret;
    }

    nl_socket_modify_cb(ctx->sock, NL_CB_VALID, NL_CB_CUSTOM,
                        interface_handler, ctx);

    ret = nl_recvmsgs_default(ctx->sock);

    return ret;
}

// Get wiphy information
int get_wiphy_info(struct scan_context *ctx) {
    struct nl_msg *msg;
    int ret;

    msg = nlmsg_alloc();
    if (!msg) {
        return -1;
    }

    genlmsg_put(msg, 0, 0, ctx->nl80211_id, 0, NLM_F_DUMP,
                NL80211_CMD_GET_WIPHY, 0);

    ret = nl_send_auto_complete(ctx->sock, msg);
    nlmsg_free(msg);

    if (ret < 0) {
        return ret;
    }

    nl_socket_modify_cb(ctx->sock, NL_CB_VALID, NL_CB_CUSTOM,
                        wiphy_handler, ctx);

    ret = nl_recvmsgs_default(ctx->sock);

    return ret;
}

// Comparison function for sorting interfaces (by wiphy first, then by ifindex)
static int compare_interfaces(const void *a, const void *b) {
    const struct p2p_interface_info *iface_a = (const struct p2p_interface_info *)a;
    const struct p2p_interface_info *iface_b = (const struct p2p_interface_info *)b;

    // Sort by wiphy_index first
    if (iface_a->wiphy_index != iface_b->wiphy_index) {
        return iface_a->wiphy_index - iface_b->wiphy_index;
    }

    // Within same wiphy, sort by ifindex
    return iface_a->ifindex - iface_b->ifindex;
}

// Simple fallback detection method (by parsing /proc/net/wireless)
void fallback_detect_wireless_interfaces() {
    FILE *fp;
    char line[256];

    WIFI_DBG("\n=== Fallback Detection Method (via /proc/net/wireless) ===\n");

    fp = fopen("/proc/net/wireless", "r");
    if (!fp) {
        WIFI_ERR("Unable to open /proc/net/wireless\n");
        return;
    }

    // Skip first two header lines
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    WIFI_DBG("Detected wireless interfaces:\n");
    while (fgets(line, sizeof(line), fp)) {
        char ifname[16];
        if (sscanf(line, "%15s", ifname) == 1) {
            // Remove colon
            char *colon = strchr(ifname, ':');
            if (colon) *colon = '\0';

            WIFI_DBG("  - %s (may support P2P, requires further detection)\n", ifname);
        }
    }

    fclose(fp);
}

int get_p2p_device(struct p2p_device_list *device_list) {
    struct scan_context ctx;
    struct p2p_interface_info *iface;
    int ret;
    int i;
    int p2p_capable_count = 0;
    int wiphy_count = 0;
    int last_wiphy = -1;

    /* 手动初始化结构体 */
    memset(&ctx, 0, sizeof(ctx));

    if (!device_list) {
        return -1;
    }

    /* 初始化结构体 */
    device_list->dev_names = NULL;
    device_list->count = 0;

    /* Initialize netlink socket */
    ctx.sock = nl_socket_alloc();
    if (!ctx.sock) {
        WIFI_ERR("Failed to allocate netlink socket\n");
        return -2;
    }

    ret = genl_connect(ctx.sock);
    if (ret < 0) {
        WIFI_ERR("Failed to connect to generic netlink: %s\n", nl_geterror(ret));
        nl_socket_free(ctx.sock);
        return -3;
    }

    ctx.nl80211_id = genl_ctrl_resolve(ctx.sock, "nl80211");
    if (ctx.nl80211_id < 0) {
        WIFI_ERR("Failed to resolve nl80211: %s\n", nl_geterror(ctx.nl80211_id));
        nl_socket_free(ctx.sock);
        return -4;
    }

    /* Get interface information */
    ret = get_interfaces(&ctx);
    if (ret < 0) {
        WIFI_ERR("Failed to get interface information: %s\n", nl_geterror(ret));
        nl_socket_free(ctx.sock);
        return -5;
    }

    WIFI_DBG("Found %d total interfaces\n", ctx.count);

    /* Get wiphy information */
    ret = get_wiphy_info(&ctx);
    if (ret < 0) {
        WIFI_ERR("Failed to get wiphy information: %s\n", nl_geterror(ret));
    }

    /* Sort interfaces (by wiphy and ifindex) */
    qsort(ctx.interfaces, ctx.count, sizeof(struct p2p_interface_info), compare_interfaces);

    /* 显示详细的接口信息用于调试 */
    WIFI_DBG("Detected %d network interfaces:\n\n", ctx.count);

    for (i = 0; i < ctx.count; i++) {
        iface = &ctx.interfaces[i];

        /* Check if this is a new wiphy */
        if (iface->wiphy_index != last_wiphy) {
            if (last_wiphy != -1) WIFI_DBG("\n");  /* Add separator between wiphys */
            WIFI_DBG("=== Wiphy %d (phy%d) ===\n", wiphy_count, iface->wiphy_index);
            wiphy_count++;
            last_wiphy = iface->wiphy_index;
        }

        WIFI_DBG("Interface %d:\n", i + 1);
        WIFI_DBG("  Name: %s\n", iface->ifname);
        WIFI_DBG("  ifindex: %d\n", iface->ifindex);
        WIFI_DBG("  MAC Address: %s\n", iface->addr);
        WIFI_DBG("  Wiphy: phy%d\n", iface->wiphy_index);
        WIFI_DBG("  P2P Client Support: %s\n", iface->supports_p2p_client ? "Yes" : "No");
        WIFI_DBG("  P2P GO Support: %s\n", iface->supports_p2p_go ? "Yes" : "No");
        WIFI_DBG("  P2P Device: %s\n", iface->is_p2p_device ? "Yes" : "No");

        if (iface->supports_p2p_client || iface->supports_p2p_go || iface->is_p2p_device) {
            WIFI_DBG("  *** This interface supports P2P functionality ***\n");
            p2p_capable_count++;
        }
        WIFI_DBG("\n");
    }

    WIFI_DBG("Summary:\n");
    WIFI_DBG("- Detected %d wiphy devices\n", wiphy_count);
    WIFI_DBG("- Found %d P2P-capable network cards\n", p2p_capable_count);

    /* 如果没有找到P2P设备，直接返回 */
    if (p2p_capable_count == 0) {
        nl_socket_free(ctx.sock);
        return 0;
    }

    /* 实现推荐接口选择逻辑：为每个wiphy选择最高ifindex的P2P接口 */
    /* 首先找到所有唯一的wiphy */
    int unique_wiphys[MAX_INTERFACES];
    int wiphy_count_actual = 0;
    int w, j;

    for (i = 0; i < ctx.count; i++) {
        int found = 0;
        for (j = 0; j < wiphy_count_actual; j++) {
            if (unique_wiphys[j] == ctx.interfaces[i].wiphy_index) {
                found = 1;
                break;
            }
        }
        if (!found) {
            unique_wiphys[wiphy_count_actual++] = ctx.interfaces[i].wiphy_index;
        }
    }

    /* 为每个wiphy找到最高ifindex的P2P接口作为推荐接口 */
    device_list->dev_names = (char **)malloc(wiphy_count_actual * sizeof(char *));
    if (!device_list->dev_names) {
        nl_socket_free(ctx.sock);
        return -6;
    }

    device_list->count = 0;
    for (w = 0; w < wiphy_count_actual; w++) {
        int target_wiphy = unique_wiphys[w];
        int max_ifindex = -1;
        int recommended_interface = -1;

        /* 在这个wiphy中找到支持P2P且ifindex最高的接口 */
        for (i = 0; i < ctx.count; i++) {
            if (ctx.interfaces[i].wiphy_index == target_wiphy) {
                /* 检查是否支持P2P功能 */
                if (ctx.interfaces[i].supports_p2p_client ||
                    ctx.interfaces[i].supports_p2p_go ||
                    ctx.interfaces[i].is_p2p_device) {
                    if (ctx.interfaces[i].ifindex > max_ifindex) {
                        max_ifindex = ctx.interfaces[i].ifindex;
                        recommended_interface = i;
                    }
                }
            }
        }

        if (recommended_interface >= 0) {
            device_list->dev_names[device_list->count] = (char *)malloc(16);
            if (!device_list->dev_names[device_list->count]) {
                /* 释放之前分配的内存 */
                while (--device_list->count >= 0) {
                    free(device_list->dev_names[device_list->count]);
                }
                free(device_list->dev_names);
                device_list->dev_names = NULL;
                device_list->count = 0;
                nl_socket_free(ctx.sock);
                return -6;
            }
            strcpy(device_list->dev_names[device_list->count],
                   ctx.interfaces[recommended_interface].ifname);

            WIFI_DBG("- phy%d recommended P2P interface: %s (ifindex: %d) ← Highest ifindex in this wiphy\n",
                target_wiphy,
                ctx.interfaces[recommended_interface].ifname,
                ctx.interfaces[recommended_interface].ifindex);

            /* 显示这个wiphy中的所有接口用于参考 */
            WIFI_DBG("  All interfaces in this wiphy: ");
            int first = 1;
            for (i = 0; i < ctx.count; i++) {
                if (ctx.interfaces[i].wiphy_index == target_wiphy) {
                    if (!first) WIFI_DBG(", ");
                    WIFI_DBG("%s(ifindex:%d)", ctx.interfaces[i].ifname, ctx.interfaces[i].ifindex);
                    first = 0;
                }
            }
            WIFI_DBG("\n");

            device_list->count++;
        }
    }

    nl_socket_free(ctx.sock);
    return 0;
}

void free_p2p_device(struct p2p_device_list *device_list) {
    int i;
    if (device_list && device_list->dev_names) {
        for (i = 0; i < device_list->count; i++) {
            if (device_list->dev_names[i]) {
                free(device_list->dev_names[i]);
            }
        }
        free(device_list->dev_names);
        device_list->dev_names = NULL;
        device_list->count = 0;
    }
}

#if 0
int main() {
    struct p2p_device_list device_list = {0};
    int ret;
    int i;

    ret = get_p2p_device(&device_list);
    if (ret < 0) {
        WIFI_ERR("Failed to get P2P devices, error code: %d\n", ret);
    } else if (device_list.count > 0) {
        printf("Found %d recommended P2P interfaces (one per wiphy):\n", device_list.count);
        for (i = 0; i < device_list.count; i++) {
            printf("  %d: %s (recommended for P2P operations)\n", i + 1, device_list.dev_names[i]);
        }
    } else {
        printf("No P2P-capable interfaces found via nl80211.\n");
    }

    /* 释放内存 */
    free_p2p_device(&device_list);

    return ret < 0 ? 1 : 0;
}
#endif
#endif
