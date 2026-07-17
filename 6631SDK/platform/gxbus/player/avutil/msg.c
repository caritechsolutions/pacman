
//#define MSG_USE_COLORS

//#include "config.h"

#include "gx_msg.h"
#include "gx_common.h"

#ifdef GXDBG
#define MSGSIZE_MAX 3072

int mp_msg_levels[MSGT_MAX];	// verbose level of this module. inited to -2
int mp_msg_level_all = MSGL_STATUS;
int verbose = 0;

static int msg_init_flag = 0;

void msg_init(void)
{
	int i;
	char* env = getenv("PLAYER_VERBOSE");
	if (env)
		verbose = atoi(env);
	for (i = 0; i < MSGT_MAX; i++)
		mp_msg_levels[i] = -2;
	mp_msg_levels[MSGT_IDENTIFY] = -1;	// no -identify output by default
}

static inline int msg_test(int mod, int lev)
{
	return lev <= (mp_msg_levels[mod] == -2 ? mp_msg_level_all + verbose : mp_msg_levels[mod]);
}

void gx_msg(int mod, int lev, const char* format, ...)
{
    va_list va;
    char tmp[MSGSIZE_MAX];

	if(msg_init_flag ==0){
		msg_init();
		msg_init_flag =1;
	}

    va_start(va, format);
    vsnprintf(tmp, MSGSIZE_MAX, format, va);
    va_end(va);
    tmp[MSGSIZE_MAX-2] = '\n';
    tmp[MSGSIZE_MAX-1] = 0;

#ifdef MSG_USE_COLORS
/* that's only a silly color test*/
    {   unsigned char v_colors[10]={9,1,3,15,7,2,2,8,8,8};
        static const char* mod_text[MSGT_MAX]= {
                                "GLOBAL",
                                "CPLAYER",
                                "GPLAYER",
                                "VIDEOOUT",
                                "AUDIOOUT",
                                "DEMUXER",
                                "DS",
                                "DEMUX",
                                "HEADER",
                                "AVSYNC",
                                "AUTOQ",
                                "CFGPARSER",
                                "DECAUDIO",
                                "DECVIDEO",
                                "SEEK",
                                "WIN32",
                                "OPEN",
                                "DVD",
                                "PARSEES",
                                "LIRC",
                                "STREAM",
                                "CACHE",
                                "MENCODER",
                                "XACODEC",
                                "TV",
                                "OSDEP",
                                "SPUDEC",
                                "PLAYTREE",
                                "INPUT",
                                "VFILTER",
                                "OSD",
                                "NETWORK",
                                "CPUDETECT",
                                "CODECCFG",
                                "SWS",
                                "VOBSUB",
                                "SUBREADER",
                                "AFILTER",
                                "NETST",
                                "MUXER",
                                "OSDMENU",
                                "IDENTIFY",
                                "RADIO",
                                "ASS",
                                "LOADER",
                                "STATUSLINE",
        };

        int c=v_colors[lev];
        int c2=(mod+1)%15+1;
        static int header=1;
        FILE* stream= (lev) <= MSGL_WARN ? stderr : stdout;
        if(header){
            fprintf(stream, "\033[%d;3%dm%9s\033[0;37m: ",c2>>3,c2&7, mod_text[mod]);
        }
        fprintf(stream, "\033[%d;3%dm",c>>3,c&7);
        header=    tmp[strlen(tmp)-1] == '\n'
                 ||tmp[strlen(tmp)-1] == '\r';
    }
#endif
    if (lev <= MSGL_WARN){
        gxlogd ( "%s", tmp);fflush(stderr);
    } else {
        gxlogd("%s", tmp);fflush(stdout);
    }
}

#endif
