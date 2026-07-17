/** @addtogroup si_module*/
/*@{*/
/* Define to prevent recursive inclusion */
#ifndef __SI_NIT_H__
#define __SI_NIT_H__

/* Includes --------------------------------------------------------------- */
#include "si_public.h"
#include "gxcore.h"

__BEGIN_DECLS

/* Exported Constants ----------------------------------------------------- */

/* Error Constants -----------------------------------------------------------*/

/* Exported Types --------------------------------------------------------- */
/** satellite transport stream information */
typedef struct {
	uint32_t  freq;                /**< frequency (MHz)*/
	uint32_t  sym_rate;            /**< symbol rate(28) (Ksymbol/s)*/
	int16_t   orbit;               /**< orbital position (degree*10)*/
	uint8_t   we_flag:1;           /**< west east flag(1) */
	uint8_t   polar:2;             /**< polarization(2) */
	uint8_t   mod_system:5;        /**< modulatiion(5) */
	uint8_t   fec_inner;           /**< fec inner(4) */
}niti_sat_info;

/** terristrial transport stream information */
typedef struct {
	uint32_t  centr_freq;          /**< centre frequency(32) */
	uint8_t   bandwidth;           /**< bandwidth */
	uint8_t   constellation:2;     /**< constellation(2) */
	uint8_t   hierarchy:3;         /**< hierarchy information(3) */
	uint8_t   trans_mode:2;        /**< transmission mode(2) */
	uint8_t   other_freq:1;        /**< other frequency flag(1) */
	uint8_t   code_rate_HP:3;      /**< code rate hp stream(3) */
	uint8_t   code_rate_LP:3;      /**< code rate lp stream (3)*/
	uint8_t   guard_interval:2;    /**< guard interval(2) */
    uint8_t   reserved;
}niti_ter_info;

/** cable transport stream information */
typedef struct {
	uint32_t  freq;            /**< frequency */
	uint32_t  sym_rate;        /**< symbol rate(28)*/
	uint8_t   fec_outer:4;     /**< fec outer(4) */
	uint8_t   fec_inner:4;     /**< fec inner(4) */
	uint8_t   modulation;      /**< modulation   0x00-undefine 0x01-16QAM 0x02-32QAM 0x03-64QAM 0x04-128QAM 0x05-256QAM */
    uint16_t  reserved;
}niti_cab_info;

/** dtmb transport stream information */
typedef struct {
    uint32_t centr_freq;

    uint8_t FEC :4;
    uint16_t modulation :4;
    uint16_t number_of_subcarrier :4;
    uint16_t frame_header_mode :4;
    uint16_t interleaving_mode :4;
    uint16_t other_frequency_flag :1;
    uint16_t sfn_mfn_flag :1;
    uint16_t reserved :10;
}niti_dtmb_info;

typedef struct {
	uint8_t delivery_type;
	union {                           /**< the union to store any type transport stream information */
		niti_sat_info sat;     /**< satellite */
		niti_cab_info cab;     /**< cable */
		niti_ter_info ter;     /**< terristrial */
		niti_dtmb_info dtmb;
	};
} niti_delivery_info_t;

/** transport stream information */
typedef struct ts_info
{
	uint16_t   ts_id;                         /**< transport_stream_id(16) */
	uint16_t   orig_network_id;               /**< original_network_id(16) */
	uint8_t    private_count;
	uint8_t    reserved;
	service_list_info_t *service_list;        /**< service list descriptor information */
	void       *private_descriptor;
	uint8_t    delivery_count;
	niti_delivery_info_t *delivery_info;   /**< the list to store any type DVB delivery stream information */
}ts_info_t;

/** network information storage structure */
#define NIT_TSINFO_MAX (64)
/*********** NITINFO BUF ****************************************************
ts_info[0]|delivery_info[012..]|ts_info[1]|delivery_info[012..]|ts_info[2]...
*****************************************************************************/
typedef struct
{
	si_info_t    si_info;                  /**< si  basic information storage structure  */
	uint16_t     network_id;               /**< network_id(16) */
	uint16_t     ts_count;                 /**< transport stream count  */
	ts_info_t    *ts_info[NIT_TSINFO_MAX]; /**< transport stream information array */
	uint8_t      *tail;                    /**< record the des data, move from tail to begin*/
	uint8_t      buf[SI_NIT_BUF_SIZE];     /**< data buf */
}NitInfo;

int32_t nit_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
int32_t nit_lp_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t des_sat_del_sys(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t des_cab_del_sys(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t des_ter_del_sys(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t des_dtmb_del_sys(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

__END_DECLS

#endif /* __SI_NIT_H__ */
/*@}*/
