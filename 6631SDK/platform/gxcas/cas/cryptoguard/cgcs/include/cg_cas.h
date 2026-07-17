#ifndef CG_CAS_H_INCLUDED
#define CG_CAS_H_INCLUDED
#include "cg_cas_type.h"

#ifdef __cplusplus
extern "C"
{
#endif


// hold infrmation about initialization status
// version information and date of library and a string holding cardless identity
extern cg_status_t CG;

#define cg_gmtime gmtime

// returns long libarary version information
// like "Cardless library Fri May 25 12:42:55 CEST 2018 Version 1.0.5"
const char *cg_get_lib_ver_long(void);

// returns short libarary version information
// like "1.0.5"
const char *cg_get_lib_ver(void);

// returns date of library
// like 2018-06-20
const char *cg_get_lib_date(void);

cg_error_t cg_verify_cl_emm_address(uint64_cg in_emm_adress);


// Initializes the CA library and verifies identity and flash memory.
// it allocates memory for ECM and EMM queue and resets all events.
// Returns CG_SUCCESS if OK otherwise an error from cg_error_t
cg_error_t cg_cas_init(void);


// De Initilizes the CA library and frees allocated resources
// Returns CG_SUCCESS if OK otherwise an error from cg_error_t
cg_error_t cg_cas_deinit(void);

//cg_error_t cg_cas_card_init(void);

// <summary>
// External function to get EMM pids from CAT section
// Send in CAT section and function fills emm_pid_list array with PIDs
// unsigned short emm_pid_list[NR_OF_CASID] (NR_OF_CASID defaults 3)
// <returns>Returns CG_SUCCESS if OK otherwise an error from cg_error_t</returns>
cg_error_t cg_get_emmp_pid(uint8_cg *cat_section, unsigned short *emm_pid_list);


// <summary>
// External function to get ECM pids from PMT section
// Send in PMT section and function fills ecm_pid_list array with PIDs
// unsigned short ecm_pid_list[NR_OF_CASID] (NR_OF_CASID defaults 3)
// <returns>Returns CG_SUCCESS if OK otherwise an error from cg_error_t</returns>
// IMPORTENT library uses this to know what SIDs are currently beeing used
// CH is current stream, 0 for viwed or 1 for DVR
// <returns>Returns CG_SUCCESS if OK otherwise an error from cg_error_t</returns>
cg_error_t cg_get_ecm_pid(uint8_cg CH, uint8_cg *pmt_section, unsigned short *ecm_pid_list);


// <summary>
// External function to queue up incomming EMMs
// Main function for processing EMM
// </summary>
// <param name="emm_section">Pointer to the EMM section in the stream</param>
// <returns>Returns CG_SUCCESS if OK otherwise an error from cg_error_t</returns>
cg_error_t cg_emm_parser(uint16_cg pid, caid_t caid_idx, uint8_cg reason, uint8_cg *buffer, int32_cg length);


// <summary>
// External function to queue up incomming ECMs
// Main function for processing ECM
// </summary>
// CH is current stream, 0 for viwed or 1 for DVR
// <param name="ecm_section">Pointer to the ECM section in the stream</param>
// <returns>Returns CG_SUCCESS if OK otherwise an error from cg_error_t</returns>
cg_error_t cg_ecm_parser(uint16_cg pid, uint8_cg session_nr, caid_t caid_idx, uint8_cg *buffer, int32_cg length);


// <summary>
// External function for library heartbeat (this is called constantly with something like a sleep 30 between calls)
// Main function for maintain timing of event and parsing of EMMs and ECMs
// </summary>
// <returns>Returns CG_SUCCESS if OK otherwise an error from cg_error_t</returns>
cg_error_t cg_cas_tick(void);

// <summary>
// External function
// fills arrary menu with strings for the CA-menu.
// </summary>
// <param name="level">The menu sub-level, 0 = Main menu, 1 = Subscription information, 2 = Pay Per View information, 4 = About CA</param>
// <param name="menu">char array to hold menu string should be size 25000 // If device holds all 1024 possible subscriptions, this string will be ~24K
// <returns>Returns CG_SUCCESS if OK otherwise an error from cg_error_t</returns>
cg_error_t cg_get_MMI(uint8_cg level, char *menu);

// <summary>
// External function
// Sends a PIN code to card for verification
// </summary>
// <param name="pincode">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <returns>Returns CG_SUCCESS if OK otherwise an error from cg_error_t</returns>
cg_error_t cg_verify_PIN(char *pincode);

// <summary>
// External function
// Change PIN code in card
// </summary>
// <param name="oldpin">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <param name="newpin">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <returns>Returns CG_SUCCESS if OK otherwise an error from cg_error_t</returns>
cg_error_t cg_change_PIN(char *oldpin, char *newpin);

// <summary>
// External function
// Change maturity level (age) in card
// </summary>
// <param name="pincode">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <param name="age">New age maturity level (age) set in card
// <returns>Returns CG_SUCCESS if OK otherwise an error from cg_error_t</returns>
cg_error_t cg_change_maturity_level(char *pincode, unsigned char age);

// <summary>
// External function
// Get current maturity level (age) set in "card"
// </summary>
// <returns>Returns CG_SUCCESS if OK otherwise an error from cg_error_t</returns>
cg_error_t cg_get_maturity_rating(unsigned char *rating);

// <summary>
// External function
// Sends a PIN code to library for overide CAS maturity rating
// If strict parental rating is OFF, it overides (unlocks) until reset lib
// If strict parental rating is ON, it overides (unlocks) until channel change
// </summary>
// <param name="pincode">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <returns>Returns CG_SUCCESS if OK otherwise an error from cg_error_t</returns>
cg_error_t cg_unlock_using_PIN(unsigned char CH, char *pincode);

cg_error_t cg_set_strict_parental_rating(unsigned char enable, char* pincode);

cg_error_t cg_get_strict_parental_rating(unsigned char *enable);

#ifdef __cplusplus
}
#endif
#endif // CG_CAS_H_INCLUDED
