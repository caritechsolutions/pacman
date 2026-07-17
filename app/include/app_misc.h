#ifndef __APP_MISC_H__
#define __APP_MISC_H__

#define APP_NETWORK (1<<0)
#define APP_SUBT    (1<<1)
#define APP_MISC    (1<<2)
#define APP_NETAPPS (1<<3)
extern unsigned int app_debug;
extern unsigned int app_error;

#define APP_NONE     "\e[0m\n"
#define APP_BLACK    "\e[0;30m"
#define APP_L_BLACK  "\e[1;30m"
#define APP_RED      "\e[0;31m"
#define APP_L_RED    "\e[1;31m"
#define APP_GREEN    "\e[0;32m"
#define APP_L_GREEN  "\e[1;32m"
#define APP_BROWN    "\e[0;33m"
#define APP_YELLOW   "\e[1;33m"
#define APP_BLUE     "\e[0;34m"
#define APP_L_BLUE   "\e[1;34m"
#define APP_PURPLE   "\e[0;35m"
#define APP_L_PURPLE "\e[1;35m"
#define APP_CYAN     "\e[0;36m"
#define APP_L_CYAN   "\e[1;36m"
#define APP_GRAY     "\e[0;37m"
#define APP_WHITE    "\e[1;37m"

#define APP_app_log_error(color, fmt, x...) do {                                    \
    if(app_debug) {                                                          \
        app_log_debug("%s", color);                                                 \
        app_log_debug("%s():%d "fmt, __func__, __LINE__, ##x);                      \
        app_log_debug("\e[0m\n");                                                   \
    }                                                                        \
} while(0)

#define APP_DBG(module, fmt, x...) do {                                      \
    if(app_debug & module) {                                                 \
        int index = 1, temp = module;                                        \
        for(;temp != 1 && ((temp >> 1) != 0); temp >>= 1) {                  \
            index++;                                                         \
        }                                                                    \
        app_log_debug("\e[%dm", 30 + index);                                        \
        app_log_debug("[%s] %s():%d "fmt, #module, __func__, __LINE__, ##x);        \
        app_log_debug("\e[0m\n");                                                   \
    }                                                                        \
} while(0)

#define APP_DUMP(ptr, size, fmt, x...) do {                                  \
    if(app_debug) {                                                          \
        app_log_debug(fmt" ", ##x);                                                 \
        if (size != 0) {                                                     \
            unsigned int i = 0;                                              \
            for (i = 0; i < (size); i++) {                                   \
                app_log_debug("0x%02x,", (ptr)[i]);                                 \
            }                                                                \
            app_log_debug("\n");                                                    \
        }                                                                    \
    }                                                                        \
} while (0)

#define APP_ERR(module, fmt, x...) do {                                      \
    if(app_error) {                                                          \
        app_log_error("\e[1;31m");                                                  \
        app_log_error("[%s] %s():%d "fmt, #module, __func__, __LINE__, ##x);        \
        app_log_error("\e[0m\n");                                                   \
    }                                                                        \
} while(0)


#define DOT_OR_DOTDOT(s) ((s)[0] == '.' && (!(s)[1] || ((s)[1] == '.' && !(s)[2])))


extern int app_debug_set(unsigned int modules);
extern char *trim(char *str);
extern int SHA1(const unsigned char *data, unsigned long len, unsigned char *buf);
extern unsigned char char2hex(unsigned char c);
extern int hex2str(unsigned char* hex,unsigned char* str, int hexlen, int strlen);
extern int str2hex(unsigned char* str,unsigned char* hex, int strlen, int hexlen);
extern char *get_size_str(char *size_str, int size);


#endif
