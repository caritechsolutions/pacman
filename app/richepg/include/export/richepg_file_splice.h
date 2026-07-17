#ifndef __RICHEPG_FILE_SPLICE_H__
#define __RICHEPG_FILE_SPLICE_H__

int richepg_files_splice_ctrl_init(void);
int richepg_files_splice_index_json_parse_all(void);
char *richepg_files_splice_memory_file_path_get(const char *file_path);
void richepg_files_splice_memory_file_delete(const char *filename, char *path);
bool richepg_file_splice_special_path_check(const char *file_path);
/*
- add 20221130
- file_path : special file name . eg: special:special:162/images1/epg-14298.jpg
- dst_file  : pvr logo(chlogo) save file name  eg: /mnt/usb01/GxPvr/Al Jazeera_20200720143617/logo.jpg
- return : 0 : success ;  other faile
*/
int richepg_files_splice_copy_special_file_to_file(const char *file_path, const char *dst_file);

void richepg_files_splice_ctrl_release(void);

#endif
