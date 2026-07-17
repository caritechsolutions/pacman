#ifdef LINUX_OS
#include <sys/un.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "app_utility.h"
#include "common/gxmalloc.h"

struct wpa_ctrl {
    int s;
    struct sockaddr_un local;
    struct sockaddr_un dest;
};

#define CONFIG_CTRL_IFACE_DIR "/var/run/wpa_supplicant"
static const char *ctrl_iface_dir = CONFIG_CTRL_IFACE_DIR;
static struct wpa_ctrl *ctrl_conn;
static char *ctrl_ifname = NULL;
static char reply_buffer[2048];

static int wpa_ctrl_request(struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len,
             char *reply, size_t *reply_len,
             void (*msg_cb)(char *msg, size_t len))
{
    struct timeval tv;
    int res;
    fd_set rfds;
    const char *_cmd;
    size_t _cmd_len;

    _cmd = cmd;
    _cmd_len = cmd_len;

    if (send(ctrl->s, _cmd, _cmd_len, 0) < 0) {
        return -1;
    }

    for (;;) {
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        FD_ZERO(&rfds);
        FD_SET(ctrl->s, &rfds);
        res = select(ctrl->s + 1, &rfds, NULL, NULL, &tv);
        if (res < 0)
            return res;
        if (FD_ISSET(ctrl->s, &rfds)) {
            res = recv(ctrl->s, reply, *reply_len, 0);
            if (res < 0)
                return res;
            if (res > 0 && reply[0] == '<') {
                /* This is an unsolicited message from
                 * wpa_supplicant, not the reply to the
                 * request. Use msg_cb to report this to the
                 * caller. */
                if (msg_cb) {
                    /* Make sure the message is nul
                     * terminated. */
                    if ((size_t) res == *reply_len)
                        res = (*reply_len) - 1;
                    reply[res] = '\0';
                    msg_cb(reply, res);
                }
                continue;
            }
            *reply_len = res;
            break;
        } else {
            return -2;
        }
    }
    return 0;
}

static void wpa_cli_msg_cb(char *msg, size_t len)
{
    app_log_debug("%s\n", msg);
}

static int _wpa_ctrl_command(struct wpa_ctrl *ctrl, char *cmd, int print)
{
    size_t len;
    int ret;

    if (ctrl_conn == NULL) {
        app_log_error("Not connected to wpa_supplicant - command dropped.\n");
        return -1;
    }
    memset(reply_buffer, 0, sizeof(reply_buffer));
    len = sizeof(reply_buffer) - 1;
    ret = wpa_ctrl_request(ctrl, cmd, strlen(cmd), reply_buffer, &len, wpa_cli_msg_cb);
    if (ret == -2) {
        app_log_error("'%s' command timed out.\n", cmd);
        return -2;
    } else if (ret < 0) {
        app_log_error("'%s' command failed.\n", cmd);
        return -1;
    }
    if (print) {
        reply_buffer[len] = '\0';
        //app_log_debug("%s", reply_buffer);
    }
    return 0;
}

static int wpa_ctrl_command(struct wpa_ctrl *ctrl, char *cmd)
{
    return _wpa_ctrl_command(ctrl, cmd, 1);
}

static int wpa_cli_cmd_status(struct wpa_ctrl *ctrl, int argc, char *argv[])
{
    int verbose = argc > 0 && strcmp(argv[0], "verbose") == 0;
    return wpa_ctrl_command(ctrl, verbose ? "STATUS-VERBOSE" : "STATUS");
}

enum wpa_cli_cmd_flags {
    cli_cmd_flag_none       = 0x00,
    cli_cmd_flag_sensitive  = 0x01
};

struct wpa_cli_cmd {
    const char *cmd;
    int (*handler)(struct wpa_ctrl *ctrl, int argc, char *argv[]);
    enum wpa_cli_cmd_flags flags;
    const char *usage;
};

static struct wpa_cli_cmd wpa_cli_commands[] = {
    { "status", wpa_cli_cmd_status,
      cli_cmd_flag_none,
      "[verbose] = get current WPA/EAPOL/EAP status" }
};

static int wpa_request(struct wpa_ctrl *ctrl, int argc, char *argv[])
{
    struct wpa_cli_cmd *cmd, *match = NULL;
    int count;
    int ret = 0;

    count = 0;
    cmd = wpa_cli_commands;
    while (cmd->cmd) {
        if (strncasecmp(cmd->cmd, argv[0], strlen(argv[0])) == 0)
        {
            match = cmd;
            if (strcasecmp(cmd->cmd, argv[0]) == 0) {
                /* we have an exact match */
                count = 1;
                break;
            }
            count++;
        }
        cmd++;
    }
    if (count > 1) {
        app_log_debug("Ambiguous command '%s'; possible commands:", argv[0]);
        cmd = wpa_cli_commands;
        while (cmd->cmd) {
            if (strncasecmp(cmd->cmd, argv[0], strlen(argv[0])) == 0) {
                app_log_debug(" %s", cmd->cmd);
            }
            cmd++;
        }
        app_log_debug("\n");
        ret = 1;
    } else if (count == 0) {
        app_log_debug("Unknown command '%s'\n", argv[0]);
        ret = 1;
    } else {
        ret = match->handler(ctrl, argc - 1, &argv[1]);
    }

    return ret;
}

#ifndef CONFIG_CTRL_IFACE_CLIENT_DIR
#define CONFIG_CTRL_IFACE_CLIENT_DIR "/tmp"
#endif
#ifndef CONFIG_CTRL_IFACE_CLIENT_PREFIX
#define CONFIG_CTRL_IFACE_CLIENT_PREFIX "wpa_ctrl_"
#endif
static struct wpa_ctrl * wpa_ctrl_open(const char *ctrl_path)
{
    struct wpa_ctrl *ctrl = NULL;
    static int counter = 0;
    int ret;
    size_t res;
    int tries = 0;

    ctrl = GxCore_Mallocz(sizeof(*ctrl));
    if (ctrl == NULL)
        return NULL;
    memset(ctrl, 0, sizeof(*ctrl));

    ctrl->s = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (ctrl->s < 0) {
        GXCORE_FREE(ctrl);
        return NULL;
    }
    ctrl->local.sun_family = AF_UNIX;
    counter++;
try_again:
    ret = snprintf(ctrl->local.sun_path, sizeof(ctrl->local.sun_path),
            CONFIG_CTRL_IFACE_CLIENT_DIR "/"
            CONFIG_CTRL_IFACE_CLIENT_PREFIX "%d-%d",
            (int) getpid(), counter);
    if (ret < 0 || (size_t) ret >= sizeof(ctrl->local.sun_path)) {
        close(ctrl->s);
        GXCORE_FREE(ctrl);
        return NULL;
    }
    tries++;
    if (bind(ctrl->s, (struct sockaddr *) &ctrl->local,
                sizeof(ctrl->local)) < 0) {
        if (errno == EADDRINUSE && tries < 2) {
            /*
             * getpid() returns unique identifier for this instance
             * of wpa_ctrl, so the existing socket file must have
             * been left by unclean termination of an earlier run.
             * Remove the file and try again.
             */
            unlink(ctrl->local.sun_path);
            goto try_again;
        }
        close(ctrl->s);
        GXCORE_FREE(ctrl);
        return NULL;
    }
    ctrl->dest.sun_family = AF_UNIX;
    res = strlcpy(ctrl->dest.sun_path, ctrl_path,
            sizeof(ctrl->dest.sun_path));
    if (res >= sizeof(ctrl->dest.sun_path)) {
        close(ctrl->s);
        GXCORE_FREE(ctrl);
        return NULL;
    }
    if (connect(ctrl->s, (struct sockaddr *) &ctrl->dest,
                sizeof(ctrl->dest)) < 0) {
        close(ctrl->s);
        unlink(ctrl->local.sun_path);
        GXCORE_FREE(ctrl);
        return NULL;
    }

    return ctrl;
}

static int wpa_cli_open_connection(const char *ifname, int attach)
{
    char *cfile = NULL;
    int flen, res;

    if (ifname == NULL)
        return -1;
    if (cfile == NULL) {
        flen = strlen(ctrl_iface_dir) + strlen(ifname) + 2;
        cfile = GxCore_Mallocz(flen);
        if (cfile == NULL)
            return -1;
        res = snprintf(cfile, flen, "%s/%s", ctrl_iface_dir,
                ifname);
        if (res < 0 || res >= flen) {
            GXCORE_FREE(cfile);
            return -1;
        }
    }
    ctrl_conn = wpa_ctrl_open(cfile);
    if (ctrl_conn == NULL) {
        GXCORE_FREE(cfile);
        return -1;
    }

    GXCORE_FREE(cfile);

    return 0;

}

static void wpa_ctrl_close(struct wpa_ctrl *ctrl)
{
    if (ctrl == NULL)
        return;
    unlink(ctrl->local.sun_path);
    if (ctrl->s >= 0)
        close(ctrl->s);
    GXCORE_FREE(ctrl);
    ctrl = NULL;
}

static void wpa_cli_close_connection(void)
{
    if (ctrl_conn == NULL)
        return;

    wpa_ctrl_close(ctrl_conn);
    ctrl_conn = NULL;
}

static void wpa_cli_cleanup(void)
{
    wpa_cli_close_connection();
}

static int wpa_cli(const char *ifname, char *cmd)
{
    int ret = 0;
    char *argv[1] = {};
    argv[0] = cmd;

    GXCORE_FREE(ctrl_ifname);
    ctrl_ifname = GxCore_Strdup(ifname);

    if (wpa_cli_open_connection(ctrl_ifname, 0) < 0) {
        GXCORE_FREE(ctrl_ifname);
        return -1;
    }

    ret = wpa_request(ctrl_conn, 1, &argv[0]);
    GXCORE_FREE(ctrl_ifname);
    wpa_cli_cleanup();

    return ret;
}

int app_check_ap_state(const char *ifname)
{
    int ret = 0 ;
    ret = wpa_cli(ifname, "status");
    if(0 == ret) {
        if(strstr(reply_buffer, "wpa_state=SCANNING")) {
            char *s = strstr(reply_buffer, "ip_address=");
            if(s && strncasecmp(s+strlen("ip_address="), "127.0.0.1", 9)) {
                return 0;
            }
        }
    }
    if ( -2 == ret )
		return ret ;
    return -1;
}
#endif
