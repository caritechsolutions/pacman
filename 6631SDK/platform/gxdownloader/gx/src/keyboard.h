#ifndef __KEY_API_H__
#define __KEY_API_H__
#include "gxtype.h"

typedef enum {
      OTAK_0,
      OTAK_1,
      OTAK_2,
      OTAK_3,
      OTAK_4,
      OTAK_5,
      OTAK_6,
      OTAK_7,
      OTAK_8,
      OTAK_9,
      OTAK_OK,
      OTAK_UP,
      OTAK_DOWN,
      OTAK_LEFT,
      OTAK_RIGHT,
      OTAK_MAX
}key_type;

extern uint32_t key_aidiou[OTAK_MAX+1];
extern uint32_t key_anhui[OTAK_MAX+1];
extern uint32_t key_aolaishi1[OTAK_MAX+1];
extern uint32_t key_aolaishi[OTAK_MAX+1];
extern uint32_t key_changjiangdianxun[OTAK_MAX+1];
extern uint32_t key_gongban_nationalchip_new[OTAK_MAX+1];
extern uint32_t key_guangxi[OTAK_MAX+1];
extern uint32_t key_hebei[OTAK_MAX+1];
extern uint32_t key_hunan_kuruixing[OTAK_MAX+1];
extern uint32_t key_hunan_taihui_new[OTAK_MAX+1];
extern uint32_t key_jinya[OTAK_MAX+1];
extern uint32_t key_keneng[OTAK_MAX+1];
extern uint32_t key_kingvon[OTAK_MAX+1];
extern uint32_t key_kuanhong[OTAK_MAX+1];
extern uint32_t key_mysoto[OTAK_MAX+1];
extern uint32_t key_neimeng_cable[OTAK_MAX+1];
extern uint32_t key_neimeng_chezai[OTAK_MAX+1];
extern uint32_t key_neimeng_gx3113c[OTAK_MAX+1];
extern uint32_t key_runde[OTAK_MAX+1];
extern uint32_t key_shenzhou[OTAK_MAX+1];
extern uint32_t key_sichuan[OTAK_MAX+1];
extern uint32_t key_sidake1[OTAK_MAX+1];
extern uint32_t key_sidake2[OTAK_MAX+1];
extern uint32_t key_skyworth[OTAK_MAX+1];
extern uint32_t key_ssht[OTAK_MAX+1];
extern uint32_t key_tdtsat[OTAK_MAX+1];
extern uint32_t key_tianditong[OTAK_MAX+1];
extern uint32_t key_tonghui_pingjiang[OTAK_MAX+1];
extern uint32_t key_tonghui[OTAK_MAX+1];
extern uint32_t key_xinlongwei[OTAK_MAX+1];
extern uint32_t key_xinniu[OTAK_MAX+1];
extern uint32_t key_xinsida[OTAK_MAX+1];
extern uint32_t key_yinhe[OTAK_MAX+1];
extern uint32_t key_yuenan_keneng_new[OTAK_MAX+1];
extern uint32_t key_zhiboxing2[OTAK_MAX+1];
extern uint32_t key_zhiboxing3[OTAK_MAX+1];
extern uint32_t key_zhiboxing[OTAK_MAX+1];
extern uint32_t key_zhiling_lufeng[OTAK_MAX+1];
extern uint32_t key_zhiling_new[OTAK_MAX+1];
extern uint32_t key_zhiling[OTAK_MAX+1];
extern uint32_t key_zhongda[OTAK_MAX+1];

int32_t keyboard_register(uint32_t key[OTAK_MAX+1]);
key_type get_key(uint32_t code);
#endif

