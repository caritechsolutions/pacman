#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "app_pop.h"
#include "gui_core.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "app_wnd_file_list.h"
#if NETWORK_SUPPORT && VPN_SUPPORT
#include <stdlib.h>
#ifdef ECOS_OS
#include <network.h>
#include <wolfssl/wolfcrypt/wc_port.h>
#endif
#include <arpa/inet.h>

#if 1//ECOS_OS
#define VPN_MERGE_STR(a, b)			(a b)
#define OPENVPN_DEFAULT_DIR			WORK_PATH"theme/openvpn/"
#define OPENVPN_USERDB_DIR			"/home/gx/openvpn/"

#define OPENVPN_CONFIG_CHECK_FLAG	"gx_check\n"
#define OPENVPN_CLIENT_CONFIG		"client.ovpn"
#define OPENVPN_CA_CRT				"ca.crt"
#define OPENVPN_CLIENT_CRT			"client.crt"
#define OPENVPN_CLIENT_KEY			"client.key"
#define OPENVPN_TA_KEY				"ta.key"
#define OPENVPN_LOGIN_CONF			"login.conf" // default client conf match login
#define OPENVPN_USER_LOGIN_CONF			"userlogin.conf" // import client conf match login
#define OPENVPN_IMPORT_FILE_POP_MSG		"Please import file:"
#define OPENVPN_INPUT_PASSWD_POP_MSG	"Please input username and password "
#define OPENVPN_IMPORT_OVPN_FILE		".ovpn suffix file"

#define OPENVPN_OP_CA				"ca"
#define OPENVPN_OP_CERT				"cert"
#define OPENVPN_OP_KEY				"key"
#define OPENVPN_OP_AUTH				"tls-auth"
#define OPENVPN_OP_LOGIN			"auth-user-pass"

#define IMPORT_CONF_FILE_SUFFFIX    "ovpn;OVPN;"
#define IMPORT_KEY_FILE_SUFFIX		"conf;CONF;key;KEY;crt;CRT"
#define FILTER_FILE_SUFFIX			"crt;CRT;key;KEY"

#define OPENVPN_CONFIG_SOURCE_DEFAULT	"default"
#define OPENVPN_CONFIG_SOURCE_IMPORT		"import"

#define OPENVPN_OP_FILE_NAME_LEN_MAX		(30)
#define OPENVPN_OP_PATH_NAME_LEN_MAX		(100)
#define OPENVPN_OP_LINE_BUF_MAX_SIZE		(128)

static int ssl_time_regist_start = 0;
#endif

#define _vpn_fclose_fp(fp) do {if (fp) fclose(fp); fp = NULL; } while(0)
#define _vpn_free_ptr(ptr) do {if (ptr) GxCore_Free(ptr); ptr = NULL; } while(0)


#define BUF_LEN 32
#define TXT_VPN_BLUE "text_system_setting_blue"
#define VPN_PASS_DISPLAY_STR "******"

#define TEXT_SYS_SET_STATUS_SHOW 	"text_system_setting_timezone"
#define IMG_SYS_SET_STATUS_BAK 	"img_system_setting_timezone_back"
#define TEXT_SYS_SET_YELLOW_KEY	"text_system_setting_yellow"
#define IMG_SYS_SET_YELLOW_KEY	"img_system_setting_yellow"
#define SYSTEM_SETTING_OPT_ITEM	"btn_system_setting_opt"
enum
{
    _VPN_TYPE = 0,
    _VPN_PSK,
    _VPN_SERVER,
    _VPN_USER,
    _VPN_PASS,
    _VPN_AUTO,
    _VPN_SAVE,
    _VPN_MAX,
};

typedef enum
{
	VPN_CONFIG_INSIDE = 0,
	VPN_CONFIG_IMPORT,
	VPN_CONFIG_OTHER,
}OpenVPN_ConfigMode;

typedef enum
{
	CLIENT_CONF = 0,
	LOGIN_CONF,
	CLIENT_KEY,
	CLIENT_CRT,
	CA_CRT,
	TA_KEY,
	MAX_CONF_ID,
}OpenVPN_Options_ID;

typedef enum
{
	IMPORT_IDLE = 0,
	IMPORT_START,
	IMPORT_CONF_OVPN,
	IMPORT_KEY_CRT_FILES_AUTO_MODE,
	IMPORT_KEY_CRT_FILES_MANUAL_MODE,
	IMPORT_CHECK,
	IMPORT_END,
}OpenVpn_Import_Status;

typedef enum
{
	CLEAN_ALL = 0,
	CLEAN_ONLY_KEY_CRT,
	CLEAN_MATCH_FILE,
	CLEAN_OTHER
}OpenVPN_Clean_Import;

typedef struct opnvpn_info_s
{
	OpenVPN_Options_ID id;
	char* option;
	char* default_path;
	char* user_path;
	char user_optionfile[OPENVPN_OP_FILE_NAME_LEN_MAX];
	char active_def;
	char active_user_option; /* if import options file success set 1 */
	char tag_user_option; /*if *.ovpn file mark option file set 1 */
}OpenVpn_Conf_Info_t;

typedef struct openvpn_import_conf_s
{
	int init_flag;
	int conf_options_num;
	int import_options_count;
	OpenVpn_Conf_Info_t* pconf_info; 
	OpenVpn_Import_Status import_status;
}OpenVpn_ImportConf_t;

static SystemSettingOpt  s_VpnOpt;
static SystemSettingItem s_VpnItem[_VPN_MAX];
static AppMsgProperty_VpnInfo sg_Vpn_info;
static OpenVpn_ImportConf_t s_OpenVpn_ImportConf = {0};
static char openvpn_err_info[OPENVPN_OP_LINE_BUF_MAX_SIZE];

static char *s_OpenVpn_ConfSource[2] = 
{
	OPENVPN_CONFIG_SOURCE_DEFAULT,
	OPENVPN_CONFIG_SOURCE_IMPORT,
};

static OpenVpn_Conf_Info_t s_OpenVpn_ConfInfo[MAX_CONF_ID] =
{
	/*1. client.ovpn*/
	{CLIENT_CONF, OPENVPN_CLIENT_CONFIG, 
		VPN_MERGE_STR(OPENVPN_DEFAULT_DIR, OPENVPN_CLIENT_CONFIG),
		VPN_MERGE_STR(OPENVPN_USERDB_DIR, OPENVPN_CLIENT_CONFIG), OPENVPN_CLIENT_CONFIG, 0, 0},
	/*2. login.conf :username password*/
	{LOGIN_CONF, OPENVPN_OP_LOGIN, 
		VPN_MERGE_STR(OPENVPN_USERDB_DIR, OPENVPN_LOGIN_CONF),
		VPN_MERGE_STR(OPENVPN_USERDB_DIR, OPENVPN_USER_LOGIN_CONF), OPENVPN_LOGIN_CONF, 0, 0},
	/*3. client.key*/
	{CLIENT_KEY, OPENVPN_OP_KEY, 
		VPN_MERGE_STR(OPENVPN_DEFAULT_DIR, OPENVPN_CLIENT_KEY),
		VPN_MERGE_STR(OPENVPN_USERDB_DIR, OPENVPN_CLIENT_KEY), OPENVPN_CLIENT_KEY, 0, 0},	
	/*4. client.crt*/
	{CLIENT_CRT, OPENVPN_OP_CERT, 
		VPN_MERGE_STR(OPENVPN_DEFAULT_DIR, OPENVPN_CLIENT_CRT),
		VPN_MERGE_STR(OPENVPN_USERDB_DIR, OPENVPN_CLIENT_CRT), OPENVPN_CLIENT_CRT, 0, 0},
	/*5. ca.crt*/
	{CA_CRT, OPENVPN_OP_CA, 
		VPN_MERGE_STR(OPENVPN_DEFAULT_DIR, OPENVPN_CA_CRT),
		VPN_MERGE_STR(OPENVPN_USERDB_DIR, OPENVPN_CA_CRT), OPENVPN_CA_CRT, 0, 0},
	/*6. ta.key*/
	{TA_KEY, OPENVPN_OP_AUTH, 
		VPN_MERGE_STR(OPENVPN_DEFAULT_DIR, OPENVPN_TA_KEY),
		VPN_MERGE_STR(OPENVPN_USERDB_DIR, OPENVPN_TA_KEY), OPENVPN_TA_KEY, 0, 0},
};

enum
{
    _VPN_PARAM_INVILED,
    _VPN_PARAM_NOCHANGE,
    _VPN_PARAM_CHANGE,
};
enum vpn_key_opt_mode
{
	VPN_OPT_NULL,
	VPN_OPT_SETTING,
	VPN_OPT_KYEBORAD_DLG,
	VPN_OPT_KYEBORAD_STR,
	VPN_OPT_IMPORT_FILE_DLG,
	VPN_OPT_IMPORT_FILE_DLG_END,
	VPN_OPT_CONNECT,
};
static int s_sel = _VPN_TYPE;
static event_list *s_vpn_statemsg_timer = NULL;
static ubyte32 s_cur_psk = {0};
static ubyte32 s_cur_pass = {0};
static enum vpn_key_opt_mode s_key_opt_mode = VPN_OPT_NULL;
static AppMsgProperty_VpnState vpn_conn_state = VPN_STATE_IDLE;
static bool s_VpnMenu_Open = false;
static char *s_file_path = NULL;
static char *s_file_path_bak = NULL;
static int s_send_vpn_msg = 0; // backup vpn msg

static int app_vpn_setting_save_param(void);
static void app_vpn_setting_server_item_init(void);
static int app_vpn_setting_server_edit_press_callback(unsigned short key);
static int app_vpn_setting_yellow_key(void);
static int app_vpn_setting_key_icon_show(void);
static int app_vpn_openvpn_setting_import_conf_file(void);
static void _vpn_pop_info_show(char *info_str);


static int app_vpn_setting_status_set(char *show_str)
{
	if (show_str)
	{
		GUI_SetProperty(IMG_SYS_SET_STATUS_BAK, "state", "show");
		GUI_SetProperty(TEXT_SYS_SET_STATUS_SHOW, "state", "show");
		GUI_SetProperty(TEXT_SYS_SET_STATUS_SHOW, "string", show_str);
	}
	else
	{
		GUI_SetProperty(TEXT_SYS_SET_STATUS_SHOW, "state", "hide");
		GUI_SetProperty(IMG_SYS_SET_STATUS_BAK, "state", "hide");
	}

	/*classic purple no need display this img bar*/
	if (APP_THEME_CLASSIC_PURPLE)
	{
		GUI_SetProperty(IMG_SYS_SET_STATUS_BAK, "state", "hide");
	}
	return EVENT_TRANSFER_KEEPON;
}

static int _vpn_openvpn_init_importinfo(void)
{
	if(s_OpenVpn_ImportConf.init_flag != 1)
	{
		s_OpenVpn_ImportConf.init_flag = 1;
		s_OpenVpn_ImportConf.conf_options_num = 0;
		s_OpenVpn_ImportConf.import_status = IMPORT_START;
		s_OpenVpn_ImportConf.pconf_info = &s_OpenVpn_ConfInfo[CLIENT_CONF];
	}
	return 0;
}

static int _vpn_openvpn_save_login(char* username, char* passwd)
{
	int ret = 0;
	int str_len = 0;
	int size_w = 0;
	int sel = 0;
	int cur_type = 0;
	handle_t handle_file = 0;
	char* login_file = NULL;
	char* openvpn_userdir = NULL;
	char buffer[OPENVPN_OP_PATH_NAME_LEN_MAX] = {0};

	openvpn_userdir = OPENVPN_USERDB_DIR;
	GUI_GetProperty(app_system_get_combobox(_VPN_TYPE), "select", &cur_type);
	if (cur_type != VPN_TYPE_OPENVPN)
	{
		return 1;
	}

	login_file = s_OpenVpn_ConfInfo[LOGIN_CONF].default_path;
	GUI_GetProperty(app_system_get_combobox(_VPN_SERVER), "select", &sel);
	if(sel == 1) // import
	{
		login_file = s_OpenVpn_ConfInfo[LOGIN_CONF].user_path;
	}

	if(username == NULL || passwd == NULL || login_file == NULL 
		|| 0 == strlen(username) || 0 == strlen(passwd))
	{
		app_log_error(" point null   %s()%d\n",__FUNCTION__,__LINE__);
		return 1;
	}

	if(GxCore_FileExists(openvpn_userdir) == GXCORE_FILE_UNEXIST)
	{
		if(GxCore_Mkdir(openvpn_userdir) != GXCORE_SUCCESS)
		{
			return 1;
		}
	}

	handle_file  = GxCore_Open(login_file, "w+");
	if(handle_file <= 0)
	{
		ret = 1;
	}
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "%s\n%s\n",username,passwd);
	str_len = strlen(buffer);

	size_w = GxCore_Write(handle_file, (void *) buffer, 1, str_len);
	if(size_w != str_len)
	{
		ret = 1;
	}

	GxCore_Close(handle_file);
	app_log_debug("%s() ret: %s \n",__FUNCTION__, (0 == ret) ? "success" : "failure");

	return ret;
}

static int _vpn_openvpn_clean_import_file(OpenVPN_Clean_Import mode, char* file) // 0: all  1: crt/key
{
	int i = 0, j = 0;
	int nents = 0;
	int strlen_path = 0;
	int del_flag = 0;
	int key_suffix_count = 0;
	char* file_name = NULL;
	char* openvpn_dir = NULL;
	GxDirent* ents = NULL;
	char* key_suffix[] = {".key", ".KEY",".crt", ".CRT"};

	openvpn_dir = OPENVPN_USERDB_DIR;

	if(GxCore_FileExists(openvpn_dir) != GXCORE_FILE_EXIST)
	{
		return 1;
	}

	strlen_path = strlen(openvpn_dir);
	strlen_path+=(OPENVPN_OP_FILE_NAME_LEN_MAX+1);
	file_name = GxCore_Malloc(strlen_path);
	if(file_name == NULL)
	{
		app_log_error("malloc failure \n");
		return 1;
	}

	key_suffix_count = sizeof(key_suffix)/sizeof(char*);
	nents = GxCore_GetDir(openvpn_dir, &ents, FILTER_FILE_SUFFIX);
	if(nents > 0)
	{
		for(i=0; i<nents; i++)
		{
			del_flag = 0;
			memset(file_name, 0, strlen_path);
			sprintf(file_name, "%s%s", openvpn_dir, ents[i].fname);
			if(ents[i].ftype != GX_FILE_DIRECTORY)
			{
				if(mode == CLEAN_ONLY_KEY_CRT) // clean key /crt file
				{
					for(j=0; j<key_suffix_count; j++)
					{
						if(strstr(file_name, key_suffix[j]))
						{
							del_flag= 1;
							break;
						}
					}
				}
				else if(mode == CLEAN_MATCH_FILE && NULL != file)//clean match file
				{
					if(strstr(file_name, file))
					{
						del_flag = 1;
					}
				}
				else if(mode == CLEAN_ALL)//clean all file
				{
					del_flag = 1;
				}

				if(1 == del_flag)
				{
					app_log_debug("del openvpn config file: %s \n", file_name);
					GxCore_FileDelete(file_name);
				}
			}
		}
	}

	GxCore_FreeDir(ents, nents);
	_vpn_free_ptr(file_name);
	return 0;
}

static int _vpn_openvpn_check_option_file_exist(OpenVPN_ConfigMode mode)
{
	int ret = 0;
	int size = 0;
	int i, j = 0;
	int parse_line = 0;
	int line_len = 0;
	int optionfile_len = 0;
	FILE *rfp = NULL;
	char *config_file = NULL;
	char *data = NULL;
	char *phead = NULL;
	char *option_tag = NULL;
	char *option_filepath = NULL;
	char *option_endpos = NULL;
	char *user_optionfile = NULL;
	char line_buf[OPENVPN_OP_LINE_BUF_MAX_SIZE] = {0};
	OpenVPN_Options_ID option_id = MAX_CONF_ID;

	if(mode == VPN_CONFIG_INSIDE)
	{
		config_file = s_OpenVpn_ConfInfo[CLIENT_CONF].default_path;
	}
	else if(mode == VPN_CONFIG_IMPORT)
	{
		config_file = s_OpenVpn_ConfInfo[CLIENT_CONF].user_path;
	}

	if(NULL == config_file || GxCore_FileExists(config_file) != GXCORE_FILE_EXIST)
	{
		return 2;
	}

	if ((rfp = fopen(config_file, "r")) == NULL)
	{
		ret = 1;
		goto END;
	}

	fseek(rfp, 0 , SEEK_END);
	size = ftell(rfp);
	fseek(rfp, 0 , SEEK_SET);

	if (size <= 0 ||(data = GxCore_Malloc(size)) == NULL)
	{
		ret = 1;
		goto END;
	}

	if(fread(data, 1, size, rfp) != size)
	{
		ret = 1;
		goto END;
	}

	app_log_debug("%s: %s \n", config_file, data);
	j = 0;
	for(i = 0; i < size; i++)
	{
		parse_line = 0;
		if(i == 0)
		{
			parse_line = 1;
		}
		else if(data[i-1] == '\n' && data[i] != '\n')
		{
			parse_line = 1;
		}

		if(1 == parse_line)
		{
			line_len = 0;
			for(j=i; j<size; j++)
			{
				if(data[j] == '\n' && data[j+1] != '\n') //one line end
				{
					line_len = j - i + 1;
					//app_log_debug("line_len: %d i:%d j:%d \n", line_len, i, j);
					break;
				}
			}

			if(data[i] != ';' &&  data[i] != '#' && line_len > 0) //active data; not start from ';' or '#'
			{
				phead = data+i;
				option_tag = NULL;
				option_id = MAX_CONF_ID;
				memset(line_buf, 0, sizeof(line_buf));
				if(line_len > OPENVPN_OP_LINE_BUF_MAX_SIZE)
				{
					memcpy(line_buf, phead, OPENVPN_OP_LINE_BUF_MAX_SIZE);
					app_log_error("one line len:%d out of buf size:%d \n", line_len, OPENVPN_OP_LINE_BUF_MAX_SIZE);
				}
				else{
					memcpy(line_buf, phead, (line_len-1)); // (line_len-1) not copy endlog char '\n'
					app_log_debug("line_buf: %s \n", line_buf);
				}

				if(strncmp(line_buf, OPENVPN_OP_CA, strlen(OPENVPN_OP_CA)) == 0)
				{
					option_id = CA_CRT;
					option_tag = OPENVPN_OP_CA;
				}
				else if(strncmp(line_buf, OPENVPN_OP_CERT, strlen(OPENVPN_OP_CERT)) == 0)
				{
					option_id = CLIENT_CRT;
					option_tag = OPENVPN_OP_CERT;
				}
				else if(strncmp(line_buf, OPENVPN_OP_KEY, strlen(OPENVPN_OP_KEY)) == 0)
				{
					option_id = CLIENT_KEY;
					option_tag = OPENVPN_OP_KEY;
				}
				else if(strncmp(line_buf, OPENVPN_OP_AUTH, strlen(OPENVPN_OP_AUTH)) == 0)
				{
					option_id = TA_KEY;
					option_tag = OPENVPN_OP_AUTH;
				}
				else if(strncmp(line_buf, OPENVPN_OP_LOGIN, strlen(OPENVPN_OP_LOGIN)) == 0)
				{
					option_id = LOGIN_CONF;
					option_tag = OPENVPN_OP_LOGIN;
				}

				if(NULL != option_tag)
				{
					option_filepath = NULL;
					if(option_id < MAX_CONF_ID)
					{
						if(mode == VPN_CONFIG_INSIDE)
						{
							option_filepath = s_OpenVpn_ConfInfo[option_id].default_path;
						}
						else if(mode == VPN_CONFIG_IMPORT)
						{
							option_filepath = strstr(line_buf, OPENVPN_USERDB_DIR); //get current client.ovpn config option info
							user_optionfile = strrchr(option_filepath, '/')+1;
							optionfile_len = strlen(user_optionfile);

							if(TA_KEY == option_id) //ta_key eg. /*tls-auth ta_key 1*/
							{
								option_endpos = strrchr(user_optionfile, ' ');
								memset(option_endpos, 0, strlen(option_endpos)); // clean char " 1"
								optionfile_len = option_endpos - user_optionfile;
							}
							if(optionfile_len > OPENVPN_OP_FILE_NAME_LEN_MAX){
								optionfile_len = OPENVPN_OP_FILE_NAME_LEN_MAX-1;
							}

							/*update current client.ovpn option file name*/
							if(strcmp(s_OpenVpn_ConfInfo[option_id].user_optionfile, user_optionfile))
							{
								memset(s_OpenVpn_ConfInfo[option_id].user_optionfile, 0, OPENVPN_OP_FILE_NAME_LEN_MAX);
								strncpy(s_OpenVpn_ConfInfo[option_id].user_optionfile, user_optionfile, optionfile_len);
							}
						}

						if(strstr(line_buf, option_filepath))
						{
							/*check this file exist or not*/
							if(GxCore_FileExists(option_filepath) != GXCORE_FILE_EXIST)
							{
								ret = 2;
								memset(line_buf, 0, sizeof(line_buf));
								if(LOGIN_CONF == option_id)
								{
									sprintf(line_buf, "%s \n %s  ", OPENVPN_INPUT_PASSWD_POP_MSG, " and save ");
								}
								else
								{
									sprintf(line_buf, "%s %s ",OPENVPN_IMPORT_FILE_POP_MSG, s_OpenVpn_ConfInfo[option_id].user_optionfile);
								}
								goto END;
							}
						}
						else
						{
							/* client.ovpn */
							if(mode == VPN_CONFIG_INSIDE) /*default client.ovpn setting err*/
							{
								if(GxCore_FileExists(option_filepath) != GXCORE_FILE_EXIST)
								{
									ret = 2;
									memset(line_buf, 0, sizeof(line_buf));
									sprintf(line_buf, "%s  \n", "Default Server file client.ovpn err. Please switch Server to User Import mode \n");
									app_log_error("this default config: %s have err setting \n",config_file);
									goto END;
								}
							}
						}
					}
				}
			}

			/* point to next line start */
			if(line_len > 0){
				i += (line_len-1);
			}
		}

		/*get each row data.  end row tag '\n' */
	}

END:
	if(2 == ret)
	{
		memset(openvpn_err_info, 0, OPENVPN_OP_LINE_BUF_MAX_SIZE);
		strcpy(openvpn_err_info, line_buf);
	}
	_vpn_fclose_fp(rfp);
	_vpn_free_ptr(data);
	return ret;
}
static int _vpn_openvpn_check_option_file(OpenVpn_ImportConf_t* import_conf)
{
	int ret = 0;
	int i, j = 0;
	int count = 0;
	GxDirent *ents = NULL;
	int nents = 0;
	char* openvpn_dir = OPENVPN_USERDB_DIR;
	char str_buf[OPENVPN_OP_LINE_BUF_MAX_SIZE] = {0};
	char file_name[OPENVPN_OP_FILE_NAME_LEN_MAX] = {0};

	if(NULL == import_conf || NULL == import_conf->pconf_info)
	{
		return 2;
	}

	ret = 0;
	count = 0;
	memset(str_buf, 0, OPENVPN_OP_LINE_BUF_MAX_SIZE);
	if(import_conf->conf_options_num > 0)
	{
		nents = GxCore_GetDir((char*)openvpn_dir, &ents, NULL);
		for(i=CLIENT_KEY; i<MAX_CONF_ID; i++)
		{
			if(import_conf->pconf_info[i].tag_user_option == 1)
			{
				for(j=0; j<nents; j++)
				{
					if(strncmp(import_conf->pconf_info[i].user_optionfile, ents[j].fname, strlen(ents[j].fname)) == 0)
					{
						break;
					}
				}
				if(j >= nents)
				{
					if(count == 0)
					{
						count++;
						sprintf(str_buf ,"%s \n %s\n",OPENVPN_IMPORT_FILE_POP_MSG,import_conf->pconf_info[i].user_optionfile);
					}
					else
					{
						memset(file_name, 0, OPENVPN_OP_FILE_NAME_LEN_MAX);
						sprintf(file_name, " %s\n", import_conf->pconf_info[i].user_optionfile);
						strcat(str_buf, file_name);
						count++;
					}
				}
			}
		}

		if(count == 0)
		{
			if((import_conf->pconf_info[LOGIN_CONF].tag_user_option == 1))
			{
				if(GxCore_FileExists(import_conf->pconf_info[LOGIN_CONF].user_path) != GXCORE_FILE_EXIST)
				{
					count++;
					sprintf(str_buf, "%s ", OPENVPN_INPUT_PASSWD_POP_MSG);
				}
			}
		}
	}
	else
	{
		if(GxCore_FileExists(import_conf->pconf_info[CLIENT_CONF].user_path) != GXCORE_FILE_EXIST)
		{
			count++;
			sprintf(str_buf, "%s \n %s \n", OPENVPN_IMPORT_FILE_POP_MSG, OPENVPN_IMPORT_OVPN_FILE);
		}
	}

	if(count > 0)
	{
		ret = 2;
		memset(openvpn_err_info, 0, OPENVPN_OP_LINE_BUF_MAX_SIZE);
		strcpy(openvpn_err_info, str_buf);
	}

	GxCore_FreeDir(ents, nents);

	return ret;
}

static int _vpn_openvpn_check_config_file(char* import_file, OpenVpn_ImportConf_t* import_conf, OpenVPN_ConfigMode mode)
{
	int ret = 0;
	int size = 0;
	int wbuf_len = 0;
	int wbuf_size = 0;
	int strlen_dir = 0;
	int str_len = 0;
	int i, j, k, m, n = 0;
	int parse_line = 0;
	int line_len = 0;
	int char_point = 0;
	FILE *rfp = NULL, *wfp = NULL;
	char *config_file = NULL;
	char *login_file = NULL;
	char *default_login_file = VPN_MERGE_STR(OPENVPN_DEFAULT_DIR, OPENVPN_LOGIN_CONF);
	char *openvpn_dir = NULL;
	char *data = NULL;
	char *phead = NULL;
	char *wbuffer = NULL;
	char *option_tag = NULL;
	char *option_filename = NULL;
	char line_buf[OPENVPN_OP_LINE_BUF_MAX_SIZE] = {0};
	char option_file_check = 0;
	OpenVPN_Options_ID option_id = MAX_CONF_ID;

	ret = 0;
	if(mode == VPN_CONFIG_INSIDE)
	{
		openvpn_dir = OPENVPN_USERDB_DIR;
		config_file = s_OpenVpn_ConfInfo[CLIENT_CONF].default_path;
		login_file = s_OpenVpn_ConfInfo[LOGIN_CONF].default_path;
		if(config_file == NULL)
		{
			return 1;
		}
		if(GxCore_FileExists(config_file) != GXCORE_FILE_EXIST)
		{
			return 1;
		}
		if(GxCore_FileExists(login_file) != GXCORE_FILE_EXIST)
		{
			if(GxCore_FileExists(openvpn_dir) == GXCORE_FILE_UNEXIST)
			{
				if(GxCore_Mkdir(openvpn_dir) != GXCORE_SUCCESS)
				{
					return 1;
				}
			}

			if(GxCore_FileExists(default_login_file) == GXCORE_FILE_EXIST)
			{
				if ((rfp = fopen(default_login_file, "r")) == NULL)
					return 1;

				fseek(rfp, 0 , SEEK_END);
				size = ftell(rfp);
				fseek(rfp, 0 , SEEK_SET);

				if(size == 0)
				{
					goto END;
				}
				if (size < 0 ||(data = GxCore_Malloc(size)) == NULL)
				{
					ret = 1;
					goto END;
				}
				if(fread(data, 1, size, rfp) != size)
				{
					ret = 1;
					goto END;
				}
			}
			else 
			{
				/*if there is no default_login_file file = /dvb/theme/openvpn/login.conf */		
			}
			if(size > 0)
			{
				if ((wfp = fopen(login_file, "w+")) != NULL)
				{
					if (size == fwrite(data, 1, size, wfp))
					{
						ret = 0;
					}
					else
					{
						ret = 2;
						app_log_error("write config err \n");
					}
				}
			}
		}
	}
	else if(mode == VPN_CONFIG_IMPORT)
	{
		openvpn_dir = OPENVPN_USERDB_DIR;
		config_file = s_OpenVpn_ConfInfo[CLIENT_CONF].user_path;

		if(NULL == import_conf || NULL == import_conf->pconf_info || NULL == config_file)
		{
			ret = 2;
			goto END;
		}

		strlen_dir = strlen(openvpn_dir);
		if ((rfp = fopen(import_file, "r")) == NULL)
		{
			ret = 2;
			goto END;
		}

		fseek(rfp, 0 , SEEK_END);
		size = ftell(rfp);
		fseek(rfp, 0 , SEEK_SET);

		if (size <= 0 ||(data = GxCore_Malloc(size)) == NULL)
		{
			ret = 2;
			goto END;
		}

		if(fread(data, 1, size, rfp) != size)
		{
			ret = 2;
			goto END;
		}

		wbuf_size = size + strlen_dir*MAX_CONF_ID;
		if ((wbuffer = GxCore_Mallocz(wbuf_size)) == NULL)
		{
			ret = 2;
			goto END;
		}

		for(i=0; i<MAX_CONF_ID; i++)
		{
			import_conf->pconf_info[i].tag_user_option = 0;
			import_conf->pconf_info[i].active_user_option = 0;
		}

		j = 0;
		wbuf_len = 0;
		for(i = 0; i < size; i++)
		{
			parse_line = 0;
			if(i == 0)
			{
				parse_line = 1;
			}
			else if(data[i-1] == '\n' && data[i] != '\n')
			{
				parse_line = 1;
			}

			if(1 == parse_line)
			{
				line_len = 0;
				for(j=i; j<size; j++)
				{
					if(data[j] == '\n' && data[j+1] != '\n') //one line end
					{
						line_len = j - i + 1;
						//app_log_debug("line_len: %d i:%d j:%d \n", line_len, i, j);
						break;
					}
				}

				if(data[i] != ';' &&  data[i] != '#' && line_len > 0) //active data; not start from ';' or '#'
				{
					phead = data+i;
					option_tag = NULL;
					option_file_check = 0;
					option_id = MAX_CONF_ID;
					memset(line_buf, 0, sizeof(line_buf));
					if(line_len > OPENVPN_OP_LINE_BUF_MAX_SIZE)
					{
						ret = 2;
						memcpy(line_buf, phead, OPENVPN_OP_LINE_BUF_MAX_SIZE);
						app_log_error("one line len:%d out of buf size:%d \n", line_len, OPENVPN_OP_LINE_BUF_MAX_SIZE);
						//goto END;
					}
					else{
						memcpy(line_buf, phead, line_len);
						app_log_debug("line_buf: %s \n", line_buf);
					}

					if(strncmp(line_buf, OPENVPN_OP_CA, strlen(OPENVPN_OP_CA)) == 0)
					{
						option_file_check = 1;
						option_id = CA_CRT;
						option_tag = OPENVPN_OP_CA;
					}
					else if(strncmp(line_buf, OPENVPN_OP_CERT, strlen(OPENVPN_OP_CERT)) == 0)
					{
						option_file_check = 1;
						option_id = CLIENT_CRT;
						option_tag = OPENVPN_OP_CERT;
					}
					else if(strncmp(line_buf, OPENVPN_OP_KEY, strlen(OPENVPN_OP_KEY)) == 0)
					{
						option_file_check = 1;
						option_id = CLIENT_KEY;
						option_tag = OPENVPN_OP_KEY;
					}
					else if(strncmp(line_buf, OPENVPN_OP_AUTH, strlen(OPENVPN_OP_AUTH)) == 0)
					{
						option_file_check = 1;
						option_id = TA_KEY;
						option_tag = OPENVPN_OP_AUTH;
					}
					else if(strncmp(line_buf, OPENVPN_OP_LOGIN, strlen(OPENVPN_OP_LOGIN)) == 0)
					{
						option_file_check = 1;
						option_id = LOGIN_CONF;
						option_tag = OPENVPN_OP_LOGIN;
					}

					if(NULL != option_tag)
					{
						k = line_len;
						option_filename = NULL;
						if(option_id < MAX_CONF_ID)
						{
							import_conf->conf_options_num++;
							import_conf->pconf_info[option_id].tag_user_option = 1;
							import_conf->pconf_info[option_id].active_user_option = 0;
							option_filename = import_conf->pconf_info[option_id].user_optionfile;
							memset(option_filename, 0 , OPENVPN_OP_FILE_NAME_LEN_MAX);
							if(option_id == LOGIN_CONF)
							{
								strcpy(option_filename, OPENVPN_USER_LOGIN_CONF);
							}
							else
							{
								n = 0;
								m = 0;
								char_point = 0;
								for(k=line_len; k>0; k--)
								{
									if(line_buf[k] == '\n')
									{
										m++;
										continue;
									}
									if(line_buf[k] == '.')
									{
										char_point = line_len - k;
									}

									if(line_buf[k-1] == '/' ||( line_buf[k-1] == ' ' && char_point > 0))
									{
										for(n=0; n<line_len-k-m; n++)
										{
											if(line_buf[k+n] == ' ' || n > OPENVPN_OP_FILE_NAME_LEN_MAX)
												break;
											option_filename[n] = line_buf[k+n];
										}
										break;
									}
								}
							}
						}
						if(option_filename != NULL && option_file_check == 1)
						{
							memset(line_buf, 0, sizeof(line_buf));
							if(strcmp(option_tag, OPENVPN_OP_AUTH) == 0)
							{
								sprintf(line_buf,"%s %s%s 1\n",option_tag,openvpn_dir,option_filename);
							}
							else
							{
								sprintf(line_buf,"%s %s%s\n",option_tag,openvpn_dir,option_filename);
							}
							str_len = strlen(line_buf);
							memcpy(wbuffer+wbuf_len, line_buf, str_len);
							wbuf_len +=str_len;
						}
					}
					else if(line_len > 0)
					{
						memcpy(wbuffer+wbuf_len, line_buf, line_len);
						wbuf_len += line_len;
					}
				}

				/* point to next line start */
				if(line_len > 0){
					i += (line_len-1);
				}
			}

			/*get each row data.  end row tag '\n' */	
		}	

		app_log_min("wbuffer: %s \n", wbuffer);

		if(wbuf_len > 0)
		{
			if ((wfp = fopen(config_file, "w+")) != NULL)
			{
				if (wbuf_len == fwrite(wbuffer, 1, wbuf_len, wfp))
				{
					import_conf->pconf_info[CLIENT_CONF].active_user_option = 1;
					import_conf->pconf_info[CLIENT_CONF].tag_user_option = 1;
				}
				else
				{
					ret = 2;
					app_log_error("write config err \n");
				}
			}
		}
	}
	
END:
	_vpn_fclose_fp(rfp);
	_vpn_fclose_fp(wfp);
	_vpn_free_ptr(data);
	_vpn_free_ptr(wbuffer);
	return ret;
}

static int _vpn_openvpn_import_option_file(char* import_file, OpenVpn_ImportConf_t* import_conf)
{
	int ret= 0;
	int i = 0;
	int size = 0;
	int strlen_path = 0;
	int char_point = 0;
	int get_filename = 0;
	FILE *rfp = NULL, *wfp = NULL;
	char *data = NULL;
	char *openvpn_dir = NULL;
	char file_path[OPENVPN_OP_PATH_NAME_LEN_MAX] = {0};
	char file_name[OPENVPN_OP_FILE_NAME_LEN_MAX] = {0};

	ret = 0;
	openvpn_dir = OPENVPN_USERDB_DIR;
	if ((rfp = fopen(import_file, "r")) == NULL)
		return 2;

	fseek(rfp, 0 , SEEK_END);
	size = ftell(rfp);
	fseek(rfp, 0 , SEEK_SET);

	if (size <= 0 ||(data = GxCore_Malloc(size)) == NULL)
	{
		ret = 2;
		goto END;
	}

	if(fread(data, 1, size, rfp) != size)
	{
		ret = 2;
		goto END;
	}

	char_point = 0;
	get_filename = 0;
	strlen_path = strlen(import_file);
	memset(file_name, 0, OPENVPN_OP_FILE_NAME_LEN_MAX);
	for(i=strlen_path; i>0; i--)
	{
		if(import_file[i] == '.')
		{
			char_point = strlen_path - i;
		}
		if(import_file[i-1] == '/' && char_point > 0)
		{
			get_filename = 1;
			memcpy(file_name, import_file+i, strlen_path-i);
			break;
		}
	}	

	if(1 == get_filename)
	{
		memset(file_path, 0, OPENVPN_OP_PATH_NAME_LEN_MAX);
		sprintf(file_path, "%s%s", openvpn_dir, file_name);
	
		if ((wfp = fopen(file_path, "w+")) != NULL)
		{
			if (size == fwrite(data, 1, size, wfp))
			{
				for(i=0; i<MAX_CONF_ID; i++)
				{
					if(strncmp(import_conf->pconf_info[i].user_optionfile, file_name, strlen(file_name)) == 0)
					{
						break;
					}
				}
				if(i < MAX_CONF_ID)
				{
					ret = 0;
					import_conf->import_options_count++;
					import_conf->pconf_info[i].active_user_option = 1;
					app_log_debug("impore file:%s to pos: %s  success \n ", file_name, file_path);
				}
			}
			else
			{
				ret = 2;
				app_log_error("write file:%s err \n", file_path);
			}
		}
	}


END:
	_vpn_fclose_fp(rfp);
	_vpn_fclose_fp(wfp);
	_vpn_free_ptr(data);

	return ret;
}

static int _vpn_openvpn_import_conf_file(char* file_path, OpenVpn_ImportConf_t* import_conf)
{
	int ret= 1;
	int i = 0;
	int strlen_path = 0;
	char* str_ext = NULL;
	char* openvpn_dir = NULL;
	char* file_dir = NULL;
	char* import_file = NULL;

	if(NULL == file_path || NULL == import_conf)
	{
		app_log_error("Null point param \n");
		return 1;
	}

	openvpn_dir = OPENVPN_USERDB_DIR;

	if(GxCore_FileExists(openvpn_dir) != GXCORE_FILE_EXIST)
	{
		if(GxCore_Mkdir(openvpn_dir) != GXCORE_SUCCESS)
		{
			app_log_error("mkdir %s failure \n", openvpn_dir);
			ret = 1;
			goto END;
		}
	}

	str_ext = strrchr(file_path, '.') + 1;
	if(import_conf->import_status == IMPORT_CONF_OVPN)
	{
		if(str_ext != NULL && (0 == strcasecmp(str_ext, "ovpn") ||0 == strcasecmp(str_ext, "OVPN")))
		{
			ret = _vpn_openvpn_check_config_file(file_path, import_conf, VPN_CONFIG_IMPORT);
			if(ret == 0 )
			{
				//get dir name
				strlen_path = strlen(file_path) + 1;
				file_dir = GxCore_Malloc(strlen_path);
				strlen_path += OPENVPN_OP_FILE_NAME_LEN_MAX;
				import_file = GxCore_Malloc(strlen_path);
				if(import_file != NULL && file_dir != NULL)
				{
					strcpy(file_dir, file_path);
					str_ext = strrchr(file_dir, '/');
					if(str_ext != NULL && str_ext != file_dir)
					{
						/*get the lost char '/' postion*/
						str_ext++;
						*str_ext = '\0';
					}

					//clean old key/crt file
					_vpn_openvpn_clean_import_file(CLEAN_ONLY_KEY_CRT, NULL);
					import_conf->import_status = IMPORT_KEY_CRT_FILES_AUTO_MODE;
					for(i=CLIENT_KEY; i<MAX_CONF_ID; i++)
					{
						if(1 == import_conf->pconf_info[i].tag_user_option && 1 != import_conf->pconf_info[i].active_user_option)
						{
							memset(import_file, 0, strlen_path);
							//_vpn_openvpn_clean_import_file(CLEAN_MATCH_FILE, import_conf->pconf_info[i].user_optionfile);
							sprintf(import_file, "%s%s", file_dir, import_conf->pconf_info[i].user_optionfile);
							ret = _vpn_openvpn_import_option_file(import_file,import_conf);
							if(ret != 0)
							{
								import_conf->import_status = IMPORT_KEY_CRT_FILES_MANUAL_MODE;
							}
						}
					}
				}
			}
		}
	}
	else if(import_conf->import_status == IMPORT_KEY_CRT_FILES_MANUAL_MODE)
	{
		ret = _vpn_openvpn_import_option_file(file_path, import_conf);
	}

END:
	_vpn_free_ptr(file_dir);
	_vpn_free_ptr(import_file);
	return ret;
}

static char *_convert_state(AppMsgProperty_VpnState *vpn_state)
{
    char *str = NULL;
    switch(*vpn_state)
    {
        case VPN_STATE_IDLE:
            str = STR_ID_NO_CONNECT;
            break;
        case VPN_STATE_START:
            str = STR_ID_CONNECTTING;
            break;
        case VPN_STATE_OK:
            str = STR_ID_CONNECTED;
            break;
        case VPN_STATE_CLOSE:
            str = STR_ID_DISCONNECT;
            break;
        default:
            break;
    }

    return str;
}

static void _vpn_state_stop_timer(void)
{
    if(s_vpn_statemsg_timer)
    {
        remove_timer(s_vpn_statemsg_timer);
        s_vpn_statemsg_timer = NULL;
    }
}

static int _vpn_set_state_msg(void *data)
{
	int cur_type = 0;
	char *state_str = NULL;
	AppMsgProperty_VpnState *vpn_state = (AppMsgProperty_VpnState *)data;

	GUI_GetProperty(app_system_get_combobox(_VPN_TYPE), "select", &cur_type);
	if(cur_type == VPN_TYPE_OPENVPN && VPN_OPT_IMPORT_FILE_DLG == s_key_opt_mode)
	{
		/*THEME_classic_purple status info bar can hide import file dlg*/
		return 0;
	}

	if(VPN_STATE_IDLE == *vpn_state || VPN_STATE_CLOSE == *vpn_state)
	{
		GUI_SetProperty(TXT_VPN_BLUE, "string", STR_ID_START);
	}
	else
	{
		GUI_SetProperty(TXT_VPN_BLUE, "string", STR_ID_STOP);
	}

    state_str = _convert_state(vpn_state);
    if(cur_type == VPN_TYPE_OPENVPN)
    {
		if((APPMSG_VPN_STOP == s_send_vpn_msg ) && (VPN_STATE_CLOSE != *vpn_state))
		{
			state_str = "Disconnecting...";
		}
    }
    if(((GUI_CheckDialog(WND_SYSTEM_SETTING) == GXCORE_SUCCESS)&&(s_vpn_statemsg_timer))
       &&(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) != GXCORE_SUCCESS))
    {
        app_vpn_setting_status_set(state_str);
    }
    s_vpn_statemsg_timer = NULL;
    return 0;
}

static void _vpn_state_set_proc(AppMsgProperty_VpnState *vpn_state)
{
    if(s_VpnMenu_Open == false )
        return;

	// note: Sky_Blue theme the TEXT_SYS_SET_STATUS_SHOW and VPN_SAVE item rect  region are same place
	if ( APP_THEME_SKY_BLUE || APP_THEME_CLASSIC_PURPLE ){
	int sel = 0;
	int cur_type = 0;
	int hide_connect_info = 0;
	sel = app_system_get_sel();
	char state[6] = {0};		
	char item_widget[60] = {0};
	char* str = NULL;
	memset(item_widget, 0, sizeof(item_widget));
	sprintf(item_widget, "%s%d", SYSTEM_SETTING_OPT_ITEM,_VPN_SAVE+1); //btn_system_setting_opt7
	GUI_GetProperty(app_system_get_combobox(_VPN_TYPE), "select", &cur_type);
	if((VPN_OPT_KYEBORAD_STR == s_key_opt_mode && (_VPN_USER == sel || _VPN_PASS == sel)) ||
		(_VPN_SAVE == sel  && VPN_OPT_CONNECT != s_key_opt_mode)) // while sel VPN_SAVE item not show connection status
	{
		hide_connect_info = 1;
	}
	else if(cur_type != sg_Vpn_info.vpn_type)
	{
		hide_connect_info = 1;
	}
	
	if(1 == hide_connect_info)	
	{
		GUI_GetProperty(TEXT_SYS_SET_STATUS_SHOW, "state", &state);
		if(0 == strcmp(state, "show"))
		{
			app_vpn_setting_status_set(NULL); //hide the connection status info bar
		}
	
		GUI_GetProperty(item_widget, "string", &str);
		if(str != NULL && 0 != strcmp(STR_ID_PRESS_OK, str))
		{
			GUI_SetProperty(item_widget, "string", STR_ID_PRESS_OK); //show " Save    Press OK"
		}
		return;
	}
	else
	{
		GUI_GetProperty(item_widget, "string", &str);
		if(str != NULL && 0 == strcmp(STR_ID_PRESS_OK, str))
		{
			GUI_SetProperty(item_widget, "string", " "); //hide "Press OK"
		}			
	}
	}
	

	if(vpn_state)
	{
		vpn_conn_state = *vpn_state;
		if(reset_timer(s_vpn_statemsg_timer) != 0)
		{
			s_vpn_statemsg_timer = create_timer(_vpn_set_state_msg, 10, (void *)&vpn_conn_state, TIMER_ONCE);
		}
	}
}

static void _vpn_openvpn_menu_get_focus(void)
{
	int cur_type = 0;
	int sel = 0 ;

	GUI_SetProperty("text_system_setting_blue", "state", "show");
	GUI_SetProperty("img_system_setting_blue", "state", "show");

	GUI_GetProperty(app_system_get_combobox(_VPN_TYPE), "select", &cur_type);
	if (cur_type == VPN_TYPE_OPENVPN)
	{
		GUI_GetProperty(app_system_get_combobox(_VPN_SERVER), "select", &sel);
		if(sel == 1) // import
		{
			GUI_SetProperty(TEXT_SYS_SET_YELLOW_KEY, "state", "show");
			GUI_SetProperty(IMG_SYS_SET_YELLOW_KEY, "state", "show");
		}
	}
}

static void _vpn_openvpn_menu_lost_focus(void)
{
	int cur_type = 0;
	int sel = 0 ;

	GUI_SetProperty("text_system_setting_blue", "state", "hide");
	GUI_SetProperty("img_system_setting_blue", "state", "hide");

	GUI_GetProperty(app_system_get_combobox(_VPN_TYPE), "select", &cur_type);
	if (cur_type == VPN_TYPE_OPENVPN)
	{
		GUI_GetProperty(app_system_get_combobox(_VPN_SERVER), "select", &sel);
		if(sel == 1) // import
		{
			GUI_SetProperty(TEXT_SYS_SET_YELLOW_KEY, "state", "hide");
			GUI_SetProperty(IMG_SYS_SET_YELLOW_KEY, "state", "hide");
		}
	}
}

static void _vpn_setting_full_keyboard_proc(PopKeyboard *data)
{
    _vpn_openvpn_menu_get_focus();

    if(data->in_ret == POP_VAL_CANCEL)
    {
        return;
    }
    if(data->out_name == NULL)
    {
        return;
    }

    if((_VPN_PASS == s_sel)||(_VPN_PSK == s_sel))
    {
        if(strlen(data->out_name)>0)
        {
            GUI_SetProperty(app_system_get_edit(s_sel), "string", VPN_PASS_DISPLAY_STR);
        }
        else
        {
            GUI_SetProperty(app_system_get_edit(s_sel), "string", NULL);
        }
        if(_VPN_PSK == s_sel)
        {
            memset(s_cur_psk, 0, strlen(s_cur_psk));
            memcpy(s_cur_psk, data->out_name, strlen(data->out_name));
        }
        else
        {
            memset(s_cur_pass, 0, strlen(s_cur_pass));
            memcpy(s_cur_pass, data->out_name, strlen(data->out_name));
        }
    }
    else
    {
        GUI_SetProperty(app_system_get_edit(s_sel), "string", data->out_name);
        GUI_SetInterface("flush", NULL);
    }

	if(strlen(data->out_name)>0)
	{
		if((_VPN_USER == s_sel && strcmp(sg_Vpn_info.vpn_username, data->out_name) != 0) ||
			(_VPN_PASS == s_sel && strcmp(sg_Vpn_info.vpn_password, data->out_name) != 0))
		{	
			s_key_opt_mode = VPN_OPT_KYEBORAD_STR;
		}
	}

    return;
}

static status_t _vpn_openvpn_check_file_user_pass(int sel, char *user,char *pass)
{
	FILE *fp = NULL;
	char line_str[100] = {0};
	char *login_file = NULL;

	if ( user == NULL || pass == NULL )
		return GXCORE_SUCCESS ;

	if(sel == 0)
	{
		login_file = s_OpenVpn_ConfInfo[LOGIN_CONF].default_path;
	}
	else
	{
		login_file = s_OpenVpn_ConfInfo[LOGIN_CONF].user_path;
	}
	if (login_file == NULL)
	{
		return GXCORE_SUCCESS ;
	}

	if(GxCore_FileExists(login_file) != GXCORE_FILE_EXIST)
	{
		return GXCORE_SUCCESS ;
	}

	if((fp = fopen(login_file, "r")) == NULL)
	{
		app_log_error("_vpn_get_user_pass = %s \r\n", login_file);
		return GXCORE_ERROR;
	}

	//username
	memset(line_str, 0, sizeof(line_str));
	fgets(line_str, sizeof(line_str), fp);
	if(line_str[strlen(line_str) - 1] == '\n')
		line_str[strlen(line_str) - 1] = 0;

	if(strcmp(user, line_str) != 0)
	{
		fclose(fp);
		return GXCORE_ERROR ;
	}

	//password
	memset(line_str, 0, sizeof(line_str));
	fgets(line_str, sizeof(line_str), fp);
	if(line_str[strlen(line_str) - 1] == '\n')
		line_str[strlen(line_str) - 1] = 0;

	if(strcmp(pass, line_str) != 0)
	{
		fclose(fp);
		return GXCORE_ERROR ;
	}

	fclose(fp);

	return GXCORE_SUCCESS;
}

static int _check_param_change(void)
{
    char *p[4] = {NULL};
    int cur_type;
    int sel = 0;
    int vpn_nopasswd = 0;
    AppMsgProperty_VpnInfo vpn_info;

    memset(&vpn_info, 0, sizeof(AppMsgProperty_VpnInfo));
    app_send_msg_exec(APPMSG_VPN_GET_PARAM, &vpn_info);
    GUI_GetProperty(app_system_get_combobox(_VPN_TYPE), "select", &cur_type);
    GUI_GetProperty(app_system_get_edit(_VPN_SERVER), "string", &p[0]);
    GUI_GetProperty(app_system_get_edit(_VPN_USER), "string", &p[1]);

	vpn_nopasswd = 0;
	if(cur_type == VPN_TYPE_OPENVPN)
	{
		vpn_nopasswd = 1;
		GUI_GetProperty(app_system_get_combobox(_VPN_SERVER), "select", &sel);
		if(sel <= 1)
			p[0] = s_OpenVpn_ConfSource[sel];
	}

    p[2] = s_cur_pass;
    p[3] = s_cur_psk;


    if (((cur_type != VPN_TYPE_OPENVPN) && (NULL == p[0] || 0 == strlen(p[0]) ) )
    		|| ((1 != vpn_nopasswd) && (NULL == p[1] || NULL == p[2] ||0 == strlen(p[1]) || 0 == strlen(p[2])))
            || ((cur_type == VPN_TYPE_L2TP)&&((NULL == p[3])|| 0 == strlen(p[3]))))
    {
        app_log_error("-------->>%s %d \r\n",__FUNCTION__,__LINE__);
        return _VPN_PARAM_INVILED;
    }
    if ((cur_type != vpn_info.vpn_type || strcmp(vpn_info.vpn_server, p[0])
            || strcmp(vpn_info.vpn_username, p[1])
            || strcmp(vpn_info.vpn_password, p[2]))
	    ||((cur_type == VPN_TYPE_L2TP)&& strcmp(vpn_info.vpn_psk, p[3])))
    {
		app_log_error("-------->>%s %d \r\n",__FUNCTION__,__LINE__);
        return _VPN_PARAM_CHANGE;
    }

    if(cur_type == VPN_TYPE_OPENVPN)
    {
        //server change, first start then save, it can not save
        if ( GXCORE_SUCCESS != _vpn_openvpn_check_file_user_pass(sel,vpn_info.vpn_username,vpn_info.vpn_password) )
        {
            app_log_error("-------->>%s %d fup \r\n",__FUNCTION__,__LINE__);
            return _VPN_PARAM_CHANGE;
        }
    }

    return _VPN_PARAM_NOCHANGE;
}

static int _check_saved_param_valid(void)
{
	int ret = 0;
	int sel = 0;
	AppMsgProperty_VpnInfo vpn_info;
	AppMsgProperty_VpnState vpn_state;
	
	memset(&vpn_info, 0, sizeof(AppMsgProperty_VpnInfo));
	app_send_msg_exec(APPMSG_VPN_GET_PARAM, &vpn_info);

	if(vpn_info.vpn_type != VPN_TYPE_OPENVPN)
	{
		if( (strlen(vpn_info.vpn_server) > 0) 
		        && (strlen(vpn_info.vpn_username) > 0) 
		        && (strlen(vpn_info.vpn_password) > 0))
		{
			if(_check_param_change() == _VPN_PARAM_CHANGE)
			{
				app_send_msg_exec(APPMSG_VPN_STOP, NULL);
			}

			if(vpn_info.vpn_type == VPN_TYPE_PPTP)
				ret = 0;
			else if((vpn_info.vpn_type == VPN_TYPE_L2TP)&&(strlen(vpn_info.vpn_psk) > 0))
				ret = 0;
			}
	}
	else if(vpn_info.vpn_type == VPN_TYPE_OPENVPN)
	{
		app_send_msg_exec(APPMSG_VPN_GET_STATE, &vpn_state);
		if(VPN_STATE_OK != vpn_state && VPN_STATE_START != vpn_state)
		{
			GUI_GetProperty(app_system_get_combobox(_VPN_SERVER), "select", &sel);
			if(sel == 1) // import
			{
				ret = _vpn_openvpn_check_option_file(&s_OpenVpn_ImportConf);
				ret = _vpn_openvpn_check_option_file_exist(VPN_CONFIG_IMPORT);
			}
			else{
				ret = _vpn_openvpn_check_config_file(NULL,&s_OpenVpn_ImportConf, VPN_CONFIG_INSIDE);
				ret = _vpn_openvpn_check_option_file_exist(VPN_CONFIG_INSIDE);
			}
		}
	}

	return ret;
}

static void _vpn_pop_info_show(char *info_str)
{
	PopDlg pop;
	memset(&pop, 0, sizeof(PopDlg));
	pop.type = POP_TYPE_NO_BTN;
	pop.format = POP_FORMAT_DLG;
	pop.str = info_str;
	pop.title = NULL;
	pop.mode = POP_MODE_UNBLOCK;
	pop.create_cb = NULL;
	pop.exit_cb = NULL;
	pop.timeout_sec = 2;
	popdlg_create(&pop);
}

static int _app_vpn_import_exit_cb(WndStatus ret)
{
	int str_len = 0;

	if (ret == WND_OK)
	{
		app_get_file_real_path(&s_file_path);
		if (s_file_path != NULL)
		{
			////backup the path...////
			APP_FREE(s_file_path_bak);
			str_len = strlen(s_file_path);
			s_file_path_bak = (char *)GxCore_Malloc(str_len + 1);
			if (s_file_path_bak != NULL)
			{
				memcpy(s_file_path_bak, s_file_path, str_len);
				s_file_path_bak[str_len] = '\0';
			}
			if((!s_file_path)||(str_len == 0))
			{
				app_log_error("input error\n");
			}
			else
			{
				_vpn_openvpn_import_conf_file(s_file_path, &s_OpenVpn_ImportConf);
			}

			
		}
	}
	s_key_opt_mode = VPN_OPT_IMPORT_FILE_DLG_END;
	return 0;
}

void app_vpn_msg_proc(GxMessage *msg)
{
    if(msg == NULL)
        return;

    switch(msg->msg_id)
    {
        case APPMSG_VPN_STATE_REPORT:
        {
            AppMsgProperty_VpnState *vpn_state = GxBus_GetMsgPropertyPtr(msg, AppMsgProperty_VpnState);
            //app_log_info("[%s]%d VPN STATE, vpn_state = %d\n", __func__, __LINE__, *vpn_state);
            _vpn_state_set_proc(vpn_state);
            break;
        }

        default:
            break;
    }
}

char* app_vpn_openvpn_get_conf_file(void)
{
	int sel = 0;
	int cur_type = 0;
	char* file = s_OpenVpn_ConfInfo[CLIENT_CONF].default_path;

	if(true == s_VpnMenu_Open)
	{
		GUI_GetProperty(app_system_get_combobox(_VPN_TYPE), "select", &cur_type);
		if (cur_type == VPN_TYPE_OPENVPN)
		{
			GUI_GetProperty(app_system_get_combobox(_VPN_SERVER), "select", &sel);
			if(sel == 1) // import
			{
				file = s_OpenVpn_ConfInfo[CLIENT_CONF].user_path;
			}
		}
	}

	if(GXCORE_FILE_EXIST != GxCore_FileExists(file))
	{
		app_log_debug("openvpn conf file :%s not exist !! \n", file);
		file = s_OpenVpn_ConfInfo[CLIENT_CONF].default_path;
		if(GXCORE_FILE_EXIST != GxCore_FileExists(file))
		{
			file = NULL;
		}
		/* update the server type to default */
		if(file != NULL)
		{
			if(strncmp(sg_Vpn_info.vpn_server, s_OpenVpn_ConfSource[0], strlen(s_OpenVpn_ConfSource[0])) != 0)
			{
				strcpy(sg_Vpn_info.vpn_server, s_OpenVpn_ConfSource[0]);
				app_send_msg_exec(APPMSG_VPN_SET_PARAM, &sg_Vpn_info); // udpate to default server
			}
		}
	}

	return file;
}

static int app_vpn_openvpn_setting_import_conf_file(void)
{
	int ret = 2;
	FileListParam file_para;

	if (check_usb_status() == false)
	{
		_vpn_pop_info_show(STR_ID_INSERT_USB);
		return 2;
	}
	if (GXCORE_FILE_EXIST != GxCore_FileExists(s_file_path_bak))
		APP_FREE(s_file_path_bak);

	memset(&file_para, 0, sizeof(file_para));
	file_para.cur_path = s_file_path_bak;
	file_para.dest_path = &s_file_path;
	file_para.suffix = IMPORT_KEY_FILE_SUFFIX;
	if(s_OpenVpn_ImportConf.import_status <= IMPORT_CONF_OVPN)
	{
		file_para.suffix = IMPORT_CONF_FILE_SUFFFIX;
		s_OpenVpn_ImportConf.import_status = IMPORT_CONF_OVPN;
	}
	s_key_opt_mode = VPN_OPT_IMPORT_FILE_DLG;
	file_para.dest_mode = DEST_MODE_FILE;
	file_para.exit_cb = _app_vpn_import_exit_cb;
	app_get_file_path_dlg(&file_para);
	
	return ret;
}

static int app_vpn_setting_server_reshow_keypress(int key)
{
	int ret = 0;
	ret =app_vpn_setting_server_edit_press_callback(key);

	return ret;
}

static int app_vpn_setting_server_item_reshow(int sel)
{
	//int ret = 0;
	int reshow = 0;
	int id_sel = 0;
	static ItemSettingType item_server_type =  0;//s_VpnItem[_VPN_SERVER].itemType;
	int openvpn_conf = 0; //0 - default; 1- user import

	if(s_VpnMenu_Open == false )
		return 2;

	item_server_type = s_VpnItem[_VPN_SERVER].itemType;
	if(sel == 2) //openvpn
	{
		if(strncmp(sg_Vpn_info.vpn_server, s_OpenVpn_ConfSource[1], strlen(s_OpenVpn_ConfSource[1])) == 0)
		{
			openvpn_conf = 1;
		}
	
		s_VpnItem[_VPN_SERVER].itemTitle = "Server";
		s_VpnItem[_VPN_SERVER].itemType = ITEM_CHOICE;

		s_VpnItem[_VPN_SERVER].itemProperty.itemPropertyCmb.content = "[Default, User Import]";
		s_VpnItem[_VPN_SERVER].itemProperty.itemPropertyCmb.sel = openvpn_conf;
		s_VpnItem[_VPN_SERVER].itemCallback.cmbCallback.CmbChange = app_vpn_setting_server_reshow_keypress;
		s_VpnItem[_VPN_SERVER].itemStatus = ITEM_NORMAL;

		s_VpnOpt.menuFuncKey.yellowKey.keyName = STR_ID_IMPORT;
		s_VpnOpt.menuFuncKey.yellowKey.keyPressFun = app_vpn_setting_yellow_key;
	}
	else
	{
		app_vpn_setting_server_item_init();
	}

	if(item_server_type != s_VpnItem[_VPN_SERVER].itemType)
	{
		s_VpnOpt.menuFuncKey.blueKey.keyName = STR_ID_START;
		if(VPN_STATE_START == vpn_conn_state || VPN_STATE_OK == vpn_conn_state){
			s_VpnOpt.menuFuncKey.blueKey.keyName = STR_ID_STOP;
		}
	
		item_server_type = s_VpnItem[_VPN_SERVER].itemType;
		GUI_GetProperty(app_system_get_combobox(_VPN_TYPE), "select", &id_sel);

		if(id_sel != sel)
		{
			//ret = GUI_SetProperty(app_system_get_combobox(_VPN_TYPE), "select", &id_sel);
		}
		app_system_set_item_data_update(_VPN_TYPE);
		app_system_set_create(&s_VpnOpt);
		app_vpn_setting_key_icon_show();
		reshow = 1;
	}

	return reshow;
}

static int app_vpn_setting_save_param(void)
{
    int ret = 0;
    int sel = 0;
    AppMsgProperty_VpnAuto vpn_auto, cur_auto;

    app_send_msg_exec(APPMSG_VPN_GET_AUTO, &vpn_auto);
    GUI_GetProperty(app_system_get_combobox(_VPN_AUTO), "select", &cur_auto);
    ret = _check_param_change();
    if (_VPN_PARAM_INVILED == ret)
    {
        _vpn_pop_info_show(STR_ID_INVALID);
        return EVENT_TRANSFER_STOP;
    }

    if (cur_auto != vpn_auto)
    {
        vpn_auto = cur_auto;
        app_send_msg_exec(APPMSG_VPN_SET_AUTO, &vpn_auto);
        _vpn_pop_info_show(STR_ID_SAVING);
    }

    if (_VPN_PARAM_CHANGE == ret)
    {
        int cur_type;
        char *p[4] = {NULL};

        memset(&sg_Vpn_info, 0, sizeof(AppMsgProperty_VpnInfo));
        GUI_GetProperty(app_system_get_edit(_VPN_SERVER), "string", &p[0]);
        GUI_GetProperty(app_system_get_edit(_VPN_USER), "string", &p[1]);
        GUI_GetProperty(app_system_get_combobox(_VPN_TYPE), "select", &cur_type);
        p[2] = s_cur_pass;
        p[3] = s_cur_psk;
        sg_Vpn_info.vpn_type = cur_type;
        memcpy(sg_Vpn_info.vpn_server, p[0], strlen(p[0]));
        memcpy(sg_Vpn_info.vpn_username, p[1], strlen(p[1]));
        memcpy(sg_Vpn_info.vpn_password, p[2], strlen(p[2]));
        if(sg_Vpn_info.vpn_type == VPN_TYPE_L2TP)
        {
            memcpy(sg_Vpn_info.vpn_psk, p[3], strlen(p[3]));
        }
        else if(sg_Vpn_info.vpn_type == VPN_TYPE_OPENVPN)
        {
            GUI_GetProperty(app_system_get_combobox(_VPN_SERVER), "select", &sel);
            if(sel <= 1)
            {
                memset(sg_Vpn_info.vpn_server,0,sizeof(sg_Vpn_info.vpn_server));
                strcpy(sg_Vpn_info.vpn_server, s_OpenVpn_ConfSource[sel]);
            }
			_vpn_openvpn_save_login(p[1], p[2]);
        }
		s_key_opt_mode = VPN_OPT_SETTING;
        app_send_msg_exec(APPMSG_VPN_SET_PARAM, &sg_Vpn_info);
        _vpn_pop_info_show(STR_ID_SAVING);
    }
    return 0;
}

static int app_vpn_setting_key_icon_show(void)
{
	int sel = 0;
	int cur_type = 0;
	int show_import = 0;
	GUI_GetProperty(app_system_get_combobox(_VPN_TYPE), "select", &cur_type);
	if (cur_type == VPN_TYPE_OPENVPN)
	{
		GUI_GetProperty(app_system_get_combobox(_VPN_SERVER), "select", &sel);
		if(sel == 1) // import
		{
			show_import = 1;
			GUI_SetProperty(TEXT_SYS_SET_YELLOW_KEY, "string", "Import");
			GUI_SetProperty(TEXT_SYS_SET_YELLOW_KEY, "state", "show");
			GUI_SetProperty(IMG_SYS_SET_YELLOW_KEY, "state", "show");
		}
	}
	if(0 == show_import)
	{
		GUI_SetProperty(TEXT_SYS_SET_YELLOW_KEY, "state", "hide");
		GUI_SetProperty(IMG_SYS_SET_YELLOW_KEY, "state", "hide");
	}

	return 0;
}

static int app_vpn_setting_yellow_key(void)
{
	int ret = 0;
	int sel = 0;
	int cur_type;
	GUI_GetProperty(app_system_get_combobox(_VPN_TYPE), "select", &cur_type);
	if (cur_type == VPN_TYPE_OPENVPN)
	{
		GUI_GetProperty(app_system_get_combobox(_VPN_SERVER), "select", &sel);
		if(sel == 1)//import
		{
			_vpn_openvpn_init_importinfo();
			ret = app_vpn_openvpn_setting_import_conf_file();
			if(ret == 0 )
			{
				_vpn_pop_info_show(STR_ID_SAVING);
			}
		}
	}
	else
	{
		//_vpn_pop_info_show(STR_ID_INVALID);
	}

	return 0;
}

static int app_vpn_setting_blue_key(void)
{
	int ret = 0;
	int sel = 0;
	ret = _check_saved_param_valid();
    if (ret == 0)
    {
        AppMsgProperty_VpnState vpn_state;
		AppMsgProperty_VpnInfo vpn_info = {0};     
		app_send_msg_exec(APPMSG_VPN_GET_PARAM, &vpn_info);
        
        s_key_opt_mode = VPN_OPT_CONNECT;
        app_send_msg_exec(APPMSG_VPN_GET_STATE, &vpn_state);
        if (VPN_STATE_OK == vpn_state || VPN_STATE_START == vpn_state)
        {
            s_send_vpn_msg = APPMSG_VPN_STOP;
            app_send_msg_exec(APPMSG_VPN_STOP, NULL);
        }
        else
        {
        	if(vpn_info.vpn_type == VPN_TYPE_OPENVPN)
        	{
        		if((APPMSG_VPN_STOP == s_send_vpn_msg) && (VPN_STATE_CLOSE != vpn_state))
        		{
					_vpn_pop_info_show(STR_ID_INVALID);
					app_log_debug("openvpn stopping connect and then can not respone Start again \n");
					return 0;
        		}
        		s_OpenVpn_ImportConf.import_status = IMPORT_CONF_OVPN;
        		GUI_GetProperty(app_system_get_combobox(_VPN_SERVER), "select", &sel);
        		if(sel <= 1) //start connect ,  update config source type
			{
				strcpy(sg_Vpn_info.vpn_server, s_OpenVpn_ConfSource[sel]);
				app_send_msg_exec(APPMSG_VPN_SET_PARAM, &sg_Vpn_info);
			}
        	}
            s_send_vpn_msg = APPMSG_VPN_START;
            app_send_msg_exec(APPMSG_VPN_START, NULL);
        }
        _vpn_pop_info_show(STR_ID_WAITING);
    }
    else
    {
		if(ret == 2)
		{
			/*openvpn info*/
			_vpn_pop_info_show(openvpn_err_info);
			/*key/crt file not import success*/
			s_OpenVpn_ImportConf.import_status = IMPORT_KEY_CRT_FILES_MANUAL_MODE;
		}
		else{
			_vpn_pop_info_show(STR_ID_INVALID);
		}
    }

    return 0;
}

static int app_vpn_setting_type_change_callback(int sel)
{
	app_system_set_item_state_update(_VPN_SERVER,ITEM_NORMAL);
    if(sel == 0) //pptp
    {
      app_system_set_item_state_update(_VPN_PSK,ITEM_DISABLE);
    }
    else if(sel == 2) //openvpn
    {
		app_system_set_item_state_update(_VPN_PSK,ITEM_DISABLE);
		app_system_set_item_state_update(_VPN_AUTO,ITEM_DISABLE);
    }
    else //L2TP
    {
       app_system_set_item_state_update(_VPN_PSK,ITEM_NORMAL);
    }

    app_vpn_setting_server_item_reshow(sel);

	return EVENT_TRANSFER_KEEPON;
}

static int app_vpn_setting_server_edit_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_STOP;
    switch (key)
    {
        case STBK_OK:
        {
            PopKeyboard keyboard;
            char *p = NULL;
			int cur_type = 0;
			GUI_GetProperty(app_system_get_combobox(_VPN_TYPE), "select", &cur_type);
			if(cur_type == VPN_TYPE_OPENVPN)
				return EVENT_TRANSFER_STOP;

            GUI_GetProperty(app_system_get_edit(_VPN_SERVER), "string", &p);

            s_sel = _VPN_SERVER;
            memset(&keyboard, 0, sizeof(PopKeyboard));
            keyboard.in_name    = p;//should get current username;
            keyboard.max_num = BUF_LEN - 1;
            keyboard.out_name   = NULL;
            keyboard.change_cb  = NULL;
            keyboard.release_cb = _vpn_setting_full_keyboard_proc;
            keyboard.usr_data   = NULL;
            keyboard.pos.x = 500;
            keyboard.pos.y = 60;
            _vpn_openvpn_menu_lost_focus();
            multi_language_keyboard_create(&keyboard);
            return EVENT_TRANSFER_STOP;
        }
        break;
        case STBK_0:
        case STBK_1:
        case STBK_2:
        case STBK_3:
        case STBK_4:
        case STBK_5:
        case STBK_6:
        case STBK_7:
        case STBK_8:
        case STBK_9:
        case STBK_LEFT:
        case STBK_RIGHT:
        case STBK_EXIT:
        case STBK_DOWN:
        case STBK_UP:
        {
            ret = EVENT_TRANSFER_KEEPON;
        }
        break;
        	/*yellow import key*/
        case STBK_YELLOW:
        case GUIK_B:
        {
			app_vpn_setting_yellow_key();
			ret = EVENT_TRANSFER_STOP;
        }
        	break;
        default:
        {
        		/*service combobox sel id [Default, User Import]*/
			app_vpn_setting_key_icon_show();
        }
            break;
    }
    return ret;
}
static int app_vpn_setting_psk_edit_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_STOP;
    switch (key)
    {
        case STBK_OK:
        {
            PopKeyboard keyboard;
            char *p = NULL;

            p = s_cur_psk;
            app_log_debug("\nfunc:%s , p:%s\n", __FUNCTION__, p);

            s_sel = _VPN_PSK;
            memset(&keyboard, 0, sizeof(PopKeyboard));
            keyboard.in_name    = p;
            keyboard.max_num = BUF_LEN - 1;
            keyboard.out_name   = NULL;
            keyboard.change_cb  = NULL;
            keyboard.release_cb = _vpn_setting_full_keyboard_proc;
            keyboard.usr_data   = NULL;
            keyboard.pos.x = 500;
            keyboard.pos.y = 60;
            _vpn_openvpn_menu_lost_focus();
            multi_language_keyboard_create(&keyboard);
            return EVENT_TRANSFER_STOP;
        }
        case STBK_LEFT:
        case STBK_RIGHT:
        case STBK_EXIT:
        case STBK_DOWN:
        case STBK_UP:
        {
            ret = EVENT_TRANSFER_KEEPON;
        }
        default:
            break;
    }
    return ret;
}

static int app_vpn_setting_user_edit_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_STOP;
    switch (key)
    {
        case STBK_OK:
        {
            PopKeyboard keyboard;
            char *p = NULL;

            GUI_GetProperty(app_system_get_edit(_VPN_USER), "string", &p);

            s_sel = _VPN_USER;
            memset(&keyboard, 0, sizeof(PopKeyboard));
            keyboard.in_name    = p;//should get current username;
            keyboard.max_num = BUF_LEN - 1;
            keyboard.out_name   = NULL;
            keyboard.change_cb  = NULL;
            keyboard.release_cb = _vpn_setting_full_keyboard_proc;
            keyboard.usr_data   = NULL;
            keyboard.pos.x = 500;
            keyboard.pos.y = 60;
            _vpn_openvpn_menu_lost_focus();
            multi_language_keyboard_create(&keyboard);
            return EVENT_TRANSFER_STOP;
        }
        case STBK_0:
        case STBK_1:
        case STBK_2:
        case STBK_3:
        case STBK_4:
        case STBK_5:
        case STBK_6:
        case STBK_7:
        case STBK_8:
        case STBK_9:
        case STBK_LEFT:
        case STBK_RIGHT:
        case STBK_EXIT:
        case STBK_DOWN:
        case STBK_UP:
        {
            ret = EVENT_TRANSFER_KEEPON;
        }
        default:
            break;
    }
    return ret;
}
static int app_vpn_setting_password_edit_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_STOP;
    switch (key)
    {
        case STBK_OK:
        {
            PopKeyboard keyboard;
            char *p = NULL;

            p = s_cur_pass;

            s_sel = _VPN_PASS;
            memset(&keyboard, 0, sizeof(PopKeyboard));
            keyboard.in_name    = p;
            keyboard.max_num = BUF_LEN - 1;
            keyboard.out_name   = NULL;
            keyboard.change_cb  = NULL;
            keyboard.release_cb = _vpn_setting_full_keyboard_proc;
            keyboard.usr_data   = NULL;
            keyboard.pos.x = 500;
            keyboard.pos.y = 60;
            _vpn_openvpn_menu_lost_focus();
            multi_language_keyboard_create(&keyboard);
            return EVENT_TRANSFER_STOP;
        }
        case STBK_LEFT:
        case STBK_RIGHT:
        case STBK_EXIT:
        case STBK_DOWN:
        case STBK_UP:
        {
            ret = EVENT_TRANSFER_KEEPON;
        }
        default:
            break;
    }
    return ret;
}

static int app_vpn_setting_save_button_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_KEEPON;

    switch (key)
    {
        case STBK_OK:
            app_vpn_setting_save_param();
            break;
    }
    return ret;
}

static void app_vpn_setting_type_item_init(void)
{
    s_VpnItem[_VPN_TYPE].itemTitle = STR_ID_TYPE;
    s_VpnItem[_VPN_TYPE].itemType = ITEM_CHOICE;
    s_VpnItem[_VPN_TYPE].itemProperty.itemPropertyCmb.content = "[PPTP,L2TP,OpenVPN]";
    s_VpnItem[_VPN_TYPE].itemProperty.itemPropertyCmb.sel = sg_Vpn_info.vpn_type;
    s_VpnItem[_VPN_TYPE].itemCallback.cmbCallback.CmbChange = app_vpn_setting_type_change_callback;
    s_VpnItem[_VPN_TYPE].itemStatus = ITEM_NORMAL;
}
static void app_vpn_setting_psk_item_init(void)
{
    s_VpnItem[_VPN_PSK].itemTitle = "Pre-shared Keys";
    s_VpnItem[_VPN_PSK].itemType = ITEM_EDIT;
    s_VpnItem[_VPN_PSK].itemProperty.itemPropertyEdit.format = "edit_string";
    s_VpnItem[_VPN_PSK].itemProperty.itemPropertyEdit.maxlen = "32";
    s_VpnItem[_VPN_PSK].itemProperty.itemPropertyEdit.intaglio = "*";
    s_VpnItem[_VPN_PSK].itemProperty.itemPropertyEdit.default_intaglio = NULL;
    s_VpnItem[_VPN_PSK].itemProperty.itemPropertyEdit.string = VPN_PASS_DISPLAY_STR;
    if (strlen(sg_Vpn_info.vpn_psk) > 0)
    {
            s_VpnItem[_VPN_PSK].itemProperty.itemPropertyEdit.string = VPN_PASS_DISPLAY_STR;
    }
    else
    {
        s_VpnItem[_VPN_PSK].itemProperty.itemPropertyEdit.string = NULL;
    }
    s_VpnItem[_VPN_PSK].itemCallback.editCallback.EditReachEnd = NULL;
    s_VpnItem[_VPN_PSK].itemCallback.editCallback.EditPress = app_vpn_setting_psk_edit_press_callback;
    if(sg_Vpn_info.vpn_type == VPN_TYPE_PPTP)
    {
        s_VpnItem[_VPN_PSK].itemStatus = ITEM_DISABLE;
    }
    else
    {
        s_VpnItem[_VPN_PSK].itemStatus = ITEM_NORMAL;
    }
}

static void app_vpn_setting_server_item_init(void)
{
    s_VpnItem[_VPN_SERVER].itemTitle = "Server";
    s_VpnItem[_VPN_SERVER].itemType = ITEM_EDIT;
    s_VpnItem[_VPN_SERVER].itemProperty.itemPropertyEdit.format = "edit_string";
    s_VpnItem[_VPN_SERVER].itemProperty.itemPropertyEdit.maxlen = "32";
    s_VpnItem[_VPN_SERVER].itemProperty.itemPropertyEdit.intaglio = NULL;
    s_VpnItem[_VPN_SERVER].itemProperty.itemPropertyEdit.default_intaglio = NULL;
    s_VpnItem[_VPN_SERVER].itemProperty.itemPropertyEdit.string = sg_Vpn_info.vpn_server;
    s_VpnItem[_VPN_SERVER].itemCallback.editCallback.EditReachEnd = NULL;
    s_VpnItem[_VPN_SERVER].itemCallback.editCallback.EditPress = app_vpn_setting_server_edit_press_callback;
    s_VpnItem[_VPN_SERVER].itemStatus = ITEM_NORMAL;
}
static void app_vpn_setting_user_item_init(void)
{
    s_VpnItem[_VPN_USER].itemTitle = "Username";
    s_VpnItem[_VPN_USER].itemType = ITEM_EDIT;
    s_VpnItem[_VPN_USER].itemProperty.itemPropertyEdit.format = "edit_string";
    s_VpnItem[_VPN_USER].itemProperty.itemPropertyEdit.maxlen = "32";
    s_VpnItem[_VPN_USER].itemProperty.itemPropertyEdit.intaglio = NULL;
    s_VpnItem[_VPN_USER].itemProperty.itemPropertyEdit.default_intaglio = NULL;
    s_VpnItem[_VPN_USER].itemProperty.itemPropertyEdit.string = sg_Vpn_info.vpn_username;
    s_VpnItem[_VPN_USER].itemCallback.editCallback.EditReachEnd = NULL;
    s_VpnItem[_VPN_USER].itemCallback.editCallback.EditPress = app_vpn_setting_user_edit_press_callback;
    s_VpnItem[_VPN_USER].itemStatus = ITEM_NORMAL;
}
static void app_vpn_setting_password_item_init(void)
{
    s_VpnItem[_VPN_PASS].itemTitle = "Password";
    s_VpnItem[_VPN_PASS].itemType = ITEM_EDIT;
    s_VpnItem[_VPN_PASS].itemProperty.itemPropertyEdit.format = "edit_string";
    s_VpnItem[_VPN_PASS].itemProperty.itemPropertyEdit.maxlen = "32";
    s_VpnItem[_VPN_PASS].itemProperty.itemPropertyEdit.intaglio = "*";
    s_VpnItem[_VPN_PASS].itemProperty.itemPropertyEdit.default_intaglio = NULL;
    if (strlen(sg_Vpn_info.vpn_password) > 0)
    {
        s_VpnItem[_VPN_PASS].itemProperty.itemPropertyEdit.string = VPN_PASS_DISPLAY_STR;
    }
    else
    {
        s_VpnItem[_VPN_PASS].itemProperty.itemPropertyEdit.string = NULL;
    }
    s_VpnItem[_VPN_PASS].itemCallback.editCallback.EditReachEnd = NULL;
    s_VpnItem[_VPN_PASS].itemCallback.editCallback.EditPress = app_vpn_setting_password_edit_press_callback;

    s_VpnItem[_VPN_PASS].itemStatus = ITEM_NORMAL;
}

static void app_vpn_setting_auto_item_init(void)
{
    AppMsgProperty_VpnAuto vpn_auto;
    app_send_msg_exec(APPMSG_VPN_GET_AUTO, &vpn_auto);

    s_VpnItem[_VPN_AUTO].itemTitle = "Auto Link";
    s_VpnItem[_VPN_AUTO].itemType = ITEM_CHOICE;
    s_VpnItem[_VPN_AUTO].itemProperty.itemPropertyCmb.content = "[Off,On]";
    s_VpnItem[_VPN_AUTO].itemProperty.itemPropertyCmb.sel = vpn_auto;
    s_VpnItem[_VPN_AUTO].itemCallback.cmbCallback.CmbChange = NULL;
    s_VpnItem[_VPN_AUTO].itemStatus = ITEM_NORMAL;
}

static void app_vpn_setting_save_item_init(void)
{
    s_VpnItem[_VPN_SAVE].itemTitle = STR_ID_SAVE;
    s_VpnItem[_VPN_SAVE].itemType = ITEM_PUSH;
    s_VpnItem[_VPN_SAVE].itemProperty.itemPropertyBtn.string = STR_ID_PRESS_OK;
    s_VpnItem[_VPN_SAVE].itemCallback.btnCallback.BtnPress = app_vpn_setting_save_button_press_callback;
    s_VpnItem[_VPN_SAVE].itemStatus = ITEM_NORMAL;
}

static int app_vpn_setting_exit_callback(ExitType exit_type)
{
    _vpn_state_stop_timer();
    s_VpnMenu_Open = false;
    s_OpenVpn_ImportConf.import_status = IMPORT_IDLE;
    return EXIT_RET_DEFAULT;
}

void app_vpn_menu_create(void)
{
    AppMsgProperty_VpnState vpn_state;
    extern time_t app_get_networktime(time_t *timer);
    extern int app_net_mktime_sync_message(void);
    memset(&sg_Vpn_info, 0, sizeof(AppMsgProperty_VpnInfo));
    app_send_msg_exec(APPMSG_VPN_GET_PARAM, &sg_Vpn_info);
    memset(s_cur_psk, 0, strlen(s_cur_psk));
    memcpy(s_cur_psk, sg_Vpn_info.vpn_psk, strlen(sg_Vpn_info.vpn_psk));
    memset(s_cur_pass, 0, strlen(s_cur_pass));
    memcpy(s_cur_pass, sg_Vpn_info.vpn_password, strlen(sg_Vpn_info.vpn_password));

    memset(&s_VpnOpt, 0, sizeof(SystemSettingOpt));
    s_VpnOpt.menuTitle = STR_ID_VPN_SETTING;
    s_VpnOpt.itemNum = _VPN_MAX;
    s_VpnOpt.item = s_VpnItem;
    s_VpnOpt.timeDisplay = TIP_HIDE;
    s_VpnOpt.menuFuncKey.blueKey.keyName = STR_ID_START;
    s_VpnOpt.menuFuncKey.blueKey.keyPressFun = app_vpn_setting_blue_key;

    app_vpn_setting_type_item_init();
    app_vpn_setting_psk_item_init();
    app_vpn_setting_server_item_init();
    app_vpn_setting_user_item_init();
    app_vpn_setting_password_item_init();
    app_vpn_setting_auto_item_init();
    app_vpn_setting_save_item_init();

    s_VpnOpt.exit = app_vpn_setting_exit_callback;
    app_system_set_create(&s_VpnOpt);
    s_VpnMenu_Open = true;
    s_key_opt_mode = VPN_OPT_NULL;
    GUI_SetProperty("btn_update_usb_confirm", "state", "hide");
    GUI_SetProperty("txt_update_usb_confirm", "state", "hide");
    app_send_msg_exec(APPMSG_VPN_GET_STATE, &vpn_state);
    _vpn_state_set_proc(&vpn_state);
    GUI_SetProperty(TEXT_SYS_SET_STATUS_SHOW, "state", "show");
    GUI_SetProperty(IMG_SYS_SET_STATUS_BAK, "state", "show");
    _vpn_openvpn_init_importinfo();
    if(sg_Vpn_info.vpn_type == VPN_TYPE_OPENVPN)
    {
        app_vpn_setting_server_item_reshow(VPN_TYPE_OPENVPN);
    }
    app_net_mktime_sync_message();
    if(ssl_time_regist_start == 0)
    {
        wolfssl_setnetworktime(app_get_networktime);
        ssl_time_regist_start = 1;
    }

}

#endif
