#ifndef __PLAYER_UTILS_H__
#define __PLAYER_UTILS_H__

extern int acodec_url2std(int type);
extern int acodec_std2url(int type);
extern int vcodec_url2std(int type);
extern int vcodec_std2url(int type);
extern int acodec_fourcc2std(int type);
extern int vcodec_fourcc2std(int type);
extern char *acodec_get_fourcc2_name(int type);
extern char *vcodec_get_fourcc2_name(int type);
extern int acodec_std2hw(int type);
extern int acodec_hw2std(int type);
extern int vcodec_std2hw(int type);
extern int acodec_std2codecid(int type);
extern int vcodec_std2codecid(int type);
extern int acodec_url2fourcc(int url);
extern int vcodec_url2fourcc(int url);
extern int acodec_check_ad_enable(int type0, int type1);
extern const char *acodec_std2str(int std);
extern const char *vcodec_std2str(int std);

#endif

