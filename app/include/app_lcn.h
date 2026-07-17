#ifndef __APP_COMMON_LCN_H__
#define __APP_COMMON_LCN_H__

#define PROG_MAX_LCN 500
#define INVALID_LCN (0x7fff)
#define INVALID_PROG (0x80fff)
#define INVALID_NETWORK_ID (0xffff)
#if LCN_CONFLICT_HANDLE_SUPPORT
#define VIRTUAL_LCN_START           (8000)
#define MAX_VIRTUAL_LCN_NUM	        (600)
#endif

#include "common/list.h"

/*Structure of lcn information*/
typedef struct _LCNInfoOne_t
{
    struct gxlist_head  list;
    uint32_t original_id;               ///< determine the only service
    uint32_t network_id;                ///< which serves as a label to identify the delivery system,
                                        ///< about which the NIT informs, from any other delivery system.
    uint32_t ts_id;                     ///< determine the only service
    uint32_t service_id;                ///< determine the only service
    uint32_t lcn;                       ///< logicnum
    uint8_t prog_name[MAX_PROG_NAME];   ///< program name
    uint32_t frequency;                 ///< frequency strength
    uint16_t freq_quality;              ///< frequency quality
    uint16_t freq_strength;             ///< frequency of tp
    uint32_t tp_id;                     ///< id of tp in pm
    uint32_t ref_service_id;
    uint32_t nvod_service_id;
    uint16_t service_type;
    uint8_t  visible_flag;
    uint8_t lcn_tag;
}LCNInfoOne_t;

/*Structure of conflict prog lcn information*/
typedef struct _LCNConflProgInfo
{
    struct gxlist_head  list;
    uint32_t original_id;
    uint32_t ts_id;
    uint32_t service_id;
    uint32_t frequency;
    uint16_t freq_quality;              ///< frequency quality
    uint16_t freq_strength;             ///< frequency of tp
    uint16_t service_type;
}LCNConflProgInfoOne_t;

/*Structure of conflict lcn information*/
typedef struct _LCNConflInfoOne_t
{
    struct gxlist_head  list;
    struct gxlist_head  prog_list;
    uint32_t lcn;
}LCNConflInfoOne_t;

/*LCN conflict handling function*/
typedef int32_t (*ConflictHandleCb)(struct gxlist_head *lcn_info_list, struct gxlist_head *lcn_confl_list);
/*LCN valid check function*/
typedef int32_t (*LCNValidCheck)(LCNInfoOne_t *lcn, struct gxlist_head *lcn_info_list);
/*LCN conflict sort function*/
typedef void (*LCNSortCb)(struct gxlist_head *lcn_confl_list);

/**
 * @brief init lcn list
 *
 * @param uint32_t *sat_id_array: the array of sat id
 *        uint32_t num: number of elements in the array
 *
 * @return  void
 */
void     app_lcn_list_init(uint32_t *sat_id_array, uint32_t num);

/**
 * @brief destroy lcn list
 *
 * @param void
 *
 * @return  void
 */
void     app_lcn_list_destroy(void);

/**
 * @brief get logicnum of program form lcn list
 *
 * @param uint32_t original_id
 *        uint32_t ts_id
 *        uint32_t service_id
 *        uint32_t frequency: the frequency of tp of program
 *
 * @return  uint32_t: logicnum of program
 */
uint32_t app_lcn_list_lcn_get(uint32_t original_id, uint32_t ts_id, uint32_t service_id, uint32_t frequency);

/**
 * @brief add nonexistent lcn node to lcn list, update program name
 *        and tv flag(indicate whether the marked program is tv) if lcn exists
 *
 * @param LCNInfoOne_t *data: structure of lcn information
 *
 * @return  void
 */
void     app_lcn_list_add_noexist_one(LCNInfoOne_t *data);

/**
 * @brief add lcn node info to lcn list, update program logicnum if lcn node exists
 *
 * @param uint32_t original_id
 *        uint32_t ts_id
 *        uint32_t service_id
 *        uint32_t frequency: the frequency of tp of program
 *
 * @return  void
 */
void     app_lcn_list_add_one(LCNInfoOne_t *data);

/**
 * @brief save lcn info to flash
 *
 * @param void
 *
 * @return  void
 */
void     app_lcn_list_info_save(void);

/**
 * @brief get lcn node from lcn list
 *
 * @param uint32_t original_id:
 *        uint32_t ts_id:
 *        uint32_t service_id:
 *        uint32_t frequency: the frequency of tp of program
 *        LCNInfoOne_t *lcn: the lcn node you get
 *
 * @return  0: success
 *         -1: error
 */
int32_t  app_lcn_list_info_get(uint32_t original_id, uint32_t ts_id, uint32_t service_id, uint32_t frequency, LCNInfoOne_t *lcn);

/**
 * @brief set tp id being searched
 *
 * @param uint32_t tp_id: tp id of pm
 *
 * @return  void
 */
void     app_lcn_searching_tp_id_set(uint32_t tp_id);

/**
 * @brief set frequency being searched
 *
 * @param uint32_t frequency: frequency of tp
 *
 * @return  void
 */
void     app_lcn_searching_frequency_set(uint32_t frequency);

/**
 * @brief get tp id being searched
 *
 * @param void
 *
 * @return  uint32_t
 */
uint32_t app_lcn_searching_tp_id_get(void);

/**
 * @brief get frequency being searched
 *
 * @param void
 *
 * @return  uint32_t
 */
uint32_t app_lcn_searching_frequency_get(void);

/**
 * @brief get flag of whether lcn list needs to be updated to flash
 *
 * @param void
 *
 * @return  1: need to update
 *          0: no need to update
 */
uint32_t app_lcn_update_flag_get(void);

/**
 * @brief execute lcn conflict handling function
 *
 * @param void
 *
 * @return  0: success
 *          other: failed
 */
int32_t  app_lcn_handle_conflict_exec(void);

/**
 * @brief execute lcn valid check function
 *
 * @param LCNInfoOne_t *lcnOne: structure of lcn information
 *
 * @return  0: success
 *          other: failed
 */
int32_t  app_lcn_valid_check_exec(LCNInfoOne_t *lcnOne);

/**
 * @brief register lcn conflict handling function
 *
 * @param ConflictHandleCb handle_conf_cb: lcn conflict handling function
 *
 * @return  void
 */
void    app_lcn_conflict_handle_register(ConflictHandleCb handle_conf_cb);

/**
 * @brief register lcn valid check function
 *
 * @param LCNValidCheck lcn_valid_check: lcn valid check function
 *
 * @return  void
 */
void    app_lcn_valid_check_register(LCNValidCheck lcn_valid_check);

/**
 * @brief register lcn conflict list sorting function
 *
 * @param LCNSortCb lcn_sort_cb: lcn conflict list sorting function
 *
 * @return  void
 */
void app_lcn_conflict_sort_method_register(LCNSortCb lcn_sort_cb);

/**
 * @brief get lcn config
 *
 * @param void
 *
 * @return  1: enable lcn support
 *          0: disable lcn support
 */
int32_t  app_lcn_config_get(void);

/**
 * @brief set the quality and strength of the locked tp
 *
 * @param uint16_t strength: the strength of locked tp
 *        uint16_t quality:  the quality of locked tp
 *
 * @return  void
 */
void    app_lcn_searching_SQ_set(uint16_t strength, uint16_t quality);

/**
 * @brief get the quality and strength of the locked tp
 *
 * @param uint16_t *strength: store the strength of locked tp
 *        uint16_t *quality : store the quality of locked tp
 *
 * @return  void
 */
void    app_lcn_searching_SQ_get(uint16_t *strength, uint16_t *quality);

#if LCN_CONFLICT_HANDLE_SUPPORT
/**
 * @brief register lcn conflict handle by project configuration
 *
 * @param void
 *
 * @return void
 *
 */
void    app_lcn_conflict_handle_common_register(void);
#endif
#endif


