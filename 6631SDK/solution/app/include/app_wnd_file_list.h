#include "app.h"

typedef enum
{
	DEST_MODE_FILE = 0,
	DEST_MODE_DIR
}DestPathMode;

typedef int (*FileListExitCb)(WndStatus);

typedef struct
{
    DestPathMode dest_mode;
    char **dest_path;
    char *cur_path;
    char *suffix;
    int pos_x;
    int pos_y;
    FileListExitCb  exit_cb;
}FileListParam;

WndStatus app_get_file_path_dlg(FileListParam *file_list);

/*
add : 2023.08.14
app_get_file_path_dlg() 实现已经改成UNBLOCK模式，所以外部调用的时候，在exit_cb传入回调函数，
当exit_cb的参数WndStatus返回WND_OK的时候，通过app_get_file_real_path()接口获取最后的路径或地址。
*/
void app_get_file_real_path(char **dest_path);
int app_update_xml_m3u_path(char *cur_path, char **dest_path, int (*exit_cb)(WndStatus));
