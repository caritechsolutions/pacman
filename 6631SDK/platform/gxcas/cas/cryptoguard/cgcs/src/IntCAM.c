// IntCAM version 2.14 2018-02-21
//
// Fingerprinting
// Video rules
// ECM,EMM processing
// Force tuning
// Pairing
// MMI
//
/// @file IntCAM.c
/// @author Kent Johansson CryptoGuard
/// @date 2018-02-21
/// @brief Mainfile for integration cryptoguard CAS in STB or CAM module.
//
// ver 2.00 2014-06-18 Initial release
// ver 2.01 2014-07-17 added callback
// ver 2.02	2014-09-25 Fixed missing forcemessage, adjusted number of keys programmed, fixed small bug EMM fingerprint, unixtime no longer a pointer
// ver 2.03 2014-11-27 changed %ld to %lu for all print of card and ird numbers, moved color to IntCAM.c, increased size of char menu[] variable, removed NVMEM_RAM_SIZE, added CADeInit();
// ver 2.04 2014-12-15 changed local functions to static, added standard includes, fixed confusing "unsigned long" to correct uint64_t and uint32_t
// ver 2.05 2014-12-19 changed ecm osd errors to 15 seconds from infinite, added more includes, made globals extern
// ver 2.06 2015-01-14 Added OTA Trigger
// ver 2.07 2015-03-16 Changed PPV menu
// ver 2.08 2015-10-05 Changed subscription menu, fixed possible pointer problem, adjusted fingerprint timings, added 15 minute margin on EMM time to allow for headend/SMS time difference and EMM process time.
//					   Added CA Version X.XX and __TIMESTAMP__  to CAS Menu
// ver 2.09 2016-01-05 Added Parental control (pincode), added more to menu, added Callback_PinRequired
// ver 2.10 2016-05-20 fixed store_long endian bug, added Time sync EMM, removed useless #ifdefs, removed dead code, fixed bug in Callback_STB_VideoRulesChanged, checking Manufacturer ID before OTA callback
// ver 2.10.1 2017-02-27 fixed some type errors, made working makefiles fixed some faulty esc characters , added CI Plus URI added encoding to messages in callbacks.c
// ver 2.11 2017-08-10 Added scrolling messages, added new type scroll_descriptor, added Interval to node struct, set default values for new variables in createnode
//                     fixed 1 byte was uninitialized when sending to card in lockWithoutPIN()
// ver 2.12 2017-10-26 moved some variables from headerfile,fixed some compiler warnings, added strict parental control to menu, removed OSD for missing time EMM
//                     removed garr_LastEcmSectionId and use crc on ECM instead because some integrators skipped using getECMPid so changing channel got slow
// ver 2.13 2018-01-15 Added example of character encoding implementation to OSD_Message, changed scrollcounter to signed variable.
// ver 2.14 2018-02-21 changed getECMPid to find individual service scrambling PIDs also, only supports 1 ECM pid so returns on first found. (added some doxyformattiong to comments)
//
// Planned upcoming functions:
//

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "Utils.h"
#include "IntCAM.h"
#include "Callbacks.h"
#include "ecb-des.c"
#include "Utils.c"
#include "cryptoguard_misc.h"
#include "gxcas_dbg.h"

#define CG_API_PRINTF(fmt, arg...) CAS_DBG(CASLIB, fmt, ##arg)//printf 
//#define CG_API_PRINTF(...) do{} while(0)

#define _DEBUG_ECM	///< Define this to enable debugprint of all received ECMs
#define _DEBUG_EMM	///< Define this to enable debugprint of all received EMMs

#define CAM			///< Define this to parse CIPlus tags in ECMs (this feature is for CAMs)
#ifdef _DEBUG_ECM
#define CG_ECM_PRINTF CG_API_PRINTF
#else
#define CG_ECM_PRINTF(...) do{} while(0)
#endif

#ifdef _DEBUG_EMM
#define CG_EMM_PRINTF CG_API_PRINTF
#else
#define CG_EMM_PRINTF(...) do{} while(0)
#endif
// debug page information
#define MAX_CAID 32
unsigned short pmt_caid_pid[MAX_CAID][2] = {{0},{0}}, cat_caid_pid[MAX_CAID][2] = {{0},{0}};

// Session parameters
uint32_t g_CardNumber, g_GroupNumber, g_IrdNumber, g_LastSTBControl[2]= {0,0}, last_ecm_crc[2] = {0,0};
unsigned short g_Caid, g_SID[2], errorcode;
unsigned char garr_Version[2], garr_CW1[2][8], garr_CW2[2][8], gLastEcmPtr31Data[2][18], gStrictParentalRating = 1;
emm_time_t time_offset;

List * g_EmmFingerprintList = NULL, * g_EmmMessageAndForcetuneList = NULL;
ecm_fingerprint_events	gs_FingerprintEvent[2];
ecm_STB_videorules STB_VideoRules[2];
nvmem_storage nvmem_cache;
///this key is use to encrypt data stored in flash memory, please randomize this key first time and then keep using the same for future FW updates.
unsigned char intcam_key[] = { 0x9d, 0x27, 0x82, 0x6b, 0xbb, 0x23, 0x74, 0xc5, 0xc2, 0x1f, 0x13, 0xa1, 0xcf, 0x03, 0xfd, 0xdd };
#define THIS_MANUFACTURER	0x00010000 // => 65536 Development This number is used for testing only
// Please contact Cryptoguard to receive an ID if OTA triggers are going to be used (this feature is optional)

// ECM and EMM fingerprint version 0 color assignment
const unsigned int fp0color[8] = {
	0x00000000,	///< 0 = Random
	0xFF808080,	///< 1 = Dark gray (gray)
	0xFF000000,	///< 2 = Black
	0xFFFFFFFF,	///< 3 = White
	0xFF0000FF,	///< 4 = Blue (blue)
	0xFF00FF00,	///< 5 = Green (lime)
	0xFFFF0000,	///< 6 = Red (red)
	0xFFFFFF00	///< 7 = Yellow (yellow)
};

// ECM and EMM fingerprint version 1 color assignment
const unsigned int fp1color[18] = {
	0xFF000000,	// 0 = Black
	0xFFFFFFFF,	// 1 = White
	0xFF00FFFF,	// 2 = Teal (aqua)
	0xFFFF00FF,	// 3 = Purple (fuchsia)
	0xFF0000FF,	// 4 = Blue (blue)
	0xFFC0C0C0,	// 5 = Light gray (silver)
	0xFF808080,	// 6 = Dark gray (gray)
	0xFF008080,	// 7 = Dark teal (teal)
	0xFF800080,	// 8 = Dark purple (purple)
	0xFF000080,	// 9 = Dark blue (navy)
	0xFFFFFF00,	// 10 = Yellow (yellow)
	0xFF00FF00,	// 11 = Green (lime)
	0xFF808000,	// 12 = Dark yellow (olive)
	0xFF008000,	// 13 = Dark green (green)
	0xFFFF0000,	// 14 = Red (red)
	0xFF800000,	// 15 = Dark red (maroon)
	0,			// 16 = Random
	0x00000000	// 17 = Transparent
};

#if defined(_DEBUG_EMM) || defined(_DEBUG_ECM)
/// <summary>
/// Internal function
/// Prints MPEG-section
/// </summary>
/// <param name="section">Pointer to a section to print</param>
/// <returns>nothing</returns>

static void printSection(unsigned char *section) {
	uint32_t j,length;
    length = ((section[1] & 0x0F) << 8) + section[2] + 3;

	for(j = 0; j < length; j ++)
		CG_API_PRINTF("%02X ", section[j]);
	CG_API_PRINTF("\r\n");
}
#endif

unsigned char EMM_Index;
uint32_t EMM_crc_list[EMM_list_len];

/**
* @brief checks to see if received EMM has already been handled.
*
* @param new_EMM_crc [in] CRC value of the received EMM.
* @return 0 if EMM has been handled already.
* @return 1 if this is a new EMM.
* @details This function scans through EMM_crc_list of previosly handled EMM to prevent old EMMs to trigger new events.
* @note EMM_list_len set the size of the ringbuffer EMM_crc_list this list is recomened to be at least 64 crc long.
* @todo nothing.
*/
static unsigned char checkIfEmmHandled(uint32_t new_EMM_crc) {
	unsigned char j;
	for (j=0; j<EMM_list_len; j++) {
		// check if we allredy received this ID ?
		if (new_EMM_crc == EMM_crc_list[j])
			// if we already received then do nothing
			return 0;
	}
	return 1;
}

static void setEmmHandled(uint32_t EMM_crc) {
	EMM_crc_list[EMM_Index] = EMM_crc;
	EMM_Index = (EMM_Index + 1) % EMM_list_len;
	ReadNVMEMRAM();
	memcpy(nvmem_cache.EMM_crc_list, EMM_crc_list, sizeof(EMM_crc_list));
	nvmem_cache.EMM_Index = EMM_Index;
	WriteNVMEMRAM();
}

static void doEcmFingerprint(unsigned char CH, time_t unixtime){
	fingerprint_descriptor	gs_EcmFingerprint;

	if (gLastEcmPtr31Data[CH][2] < 2) {	// ECM Version 0 or 1
		// Version common settings
		switch ((gLastEcmPtr31Data[CH][3] >> 6) & 0x03)	{
		case 1:
			sprintf(gs_EcmFingerprint.DisplayText, "%011u", g_CardNumber);
			break;
		case 2:
			sprintf(gs_EcmFingerprint.DisplayText, "%011u", g_IrdNumber);
			break;
		case 3:
			sprintf(gs_EcmFingerprint.DisplayText, "%011u %011u", g_CardNumber, g_IrdNumber);
			break;
		}
		if (((gLastEcmPtr31Data[CH][3] >> 5) & 0x01) == 1) {
			// Randomize X and Y
			gs_EcmFingerprint.X = rand()/DIV_16;
			gs_EcmFingerprint.Y = rand()/DIV_16;
		}
		else {
			gs_EcmFingerprint.X = ((gLastEcmPtr31Data[CH][4] >> 4) & 0x0F);
			gs_EcmFingerprint.Y = (gLastEcmPtr31Data[CH][4] & 0x0F);
		}
		// Set duration of the fingerprint
		gs_EcmFingerprint.DurationMs = gLastEcmPtr31Data[CH][5] << 8 | gLastEcmPtr31Data[CH][6];
		// Set timestamp for next display
		gs_FingerprintEvent[CH].TimeForNext = ((int)unixtime + ((gLastEcmPtr31Data[CH][3] & 0x1F) * 60)) +(gs_EcmFingerprint.DurationMs/1000);
		// Version specific
		if (gLastEcmPtr31Data[CH][2] == 1) {
			// Check if font color is set to random
			if ((gLastEcmPtr31Data[CH][13] & 0x1F) == 16) {
				gs_EcmFingerprint.FontColor = fp1color[rand()/DIV_16];
			}
			else {
				gs_EcmFingerprint.FontColor = fp1color[gLastEcmPtr31Data[CH][13] & 0x1F];
			}
			// Check if background color is set to random
			if ((gLastEcmPtr31Data[CH][14] & 0x1F) == 16) {
				gs_EcmFingerprint.BackgroundColor = fp1color[rand()/DIV_16];
			}
			else {
				gs_EcmFingerprint.BackgroundColor = fp1color[gLastEcmPtr31Data[CH][14] & 0x1F];
			}
		}
		else if (gLastEcmPtr31Data[CH][2] == 0) {
			if ((gLastEcmPtr31Data[CH][8] & 0x0F) == 0) {
				gs_EcmFingerprint.FontColor = fp0color[(rand()/DIV_7)+1];
			}
			else {
				gs_EcmFingerprint.FontColor = fp0color[gLastEcmPtr31Data[CH][8] & 0x0F];
			}
			// Version 0 fingerprint always have transparent background
			gs_EcmFingerprint.BackgroundColor = 0x00000000;
		}
	}
	gs_FingerprintEvent[CH].DisplayFingerprint = 0;
	Callback_STB_DisplayFingerPrint(CH, &gs_EcmFingerprint);
}

static void doEmmFingerprint(Node * current, time_t unixtime){
	fingerprint_descriptor	EmmFingerprint;

	// Check that it's version 1 fingerprint (all emm fingerprint are version 1)
	if (current->EmmData[2] == 1) {
		switch ((current->EmmData[3] >> 6) & 0x03) {
		case 1:
			sprintf(EmmFingerprint.DisplayText, "%011u", g_CardNumber);
			break;
		case 2:
			sprintf(EmmFingerprint.DisplayText, "%011u", g_IrdNumber);
			break;
		case 3:
			sprintf(EmmFingerprint.DisplayText, "%011u %011u", g_CardNumber, g_IrdNumber);
			break;
		}
		if (((current->EmmData[3] >> 5) & 0x01) == 1) {
			// Randomize X and Y
			EmmFingerprint.X = rand()/DIV_16;
			EmmFingerprint.Y = rand()/DIV_16;
		}
		else {
			EmmFingerprint.X = ((current->EmmData[4] >> 4) & 0x0F);
			EmmFingerprint.Y = (current->EmmData[4] & 0x0F);
		}
		// Set duration of the fingerprint
		EmmFingerprint.DurationMs = current->EmmData[5] << 8 | current->EmmData[6];
		// Set timestamp for next display
		current->TimeForNext = (unixtime + ((current->EmmData[3] & 0x1F) * 60)) +(EmmFingerprint.DurationMs/1000);
		// Version specific
		if (current->EmmData[2] == 1) {
			// Check if font color is set to random
			if ((current->EmmData[13] & 0x1F) == 16) {
				EmmFingerprint.FontColor = fp1color[rand()/DIV_16];
			}
			else {
				EmmFingerprint.FontColor = fp1color[current->EmmData[13] & 0x1F];
			}
			// Check if background color is set to random
			if ((current->EmmData[14] & 0x1F) == 16) {
				EmmFingerprint.BackgroundColor = fp1color[rand()/DIV_16];
			}
			else {
				EmmFingerprint.BackgroundColor = fp1color[current->EmmData[14] & 0x1F];
			}
		}
		// First check if CH 0 Sid is in the list
		if (findSid(g_SID[0], g_EmmFingerprintList)) {
			// so it was in list, is it this one ?
			if (current->SID == g_SID[0]) {
				Callback_STB_DisplayFingerPrint(0, &EmmFingerprint);
			}
		}
		else {
			if (current->SID == 0) {
				Callback_STB_DisplayFingerPrint(0, &EmmFingerprint);
			}
		}
		// make sure stream 1 is active
		if (g_SID[1] != 0) {
			// First check if CH 1 Sid is in the list
			if (findSid(g_SID[1], g_EmmFingerprintList)) {
				// so it was in list, is it this one ?
				if (current->SID == g_SID[1]) {
					Callback_STB_DisplayFingerPrint(1, &EmmFingerprint);
				}
			}
			else {
				if (current->SID == 0) {
					Callback_STB_DisplayFingerPrint(1, &EmmFingerprint);
				}
			}
		}
	}
}

#define FIVE_MINUTES (5 * 60)

/*
/ <summary>
/ API function
/ Main function for maintain timing of event
/ </summary>
/ <param name="unixtime">current time</param>
/ <returns>nothing</returns>
*/
void TimerTick(time_t unixtime) {
	unsigned char CH;
	Node * current;
	static int time_error_countdown = FIVE_MINUTES; // Display error if No time sync emm message received for 5 minutes

	current = g_EmmFingerprintList->head;
	// Check through EMM list for any active fingerprints
	while(current != NULL) {
		Node *currentNext = current->next;
		// Check if it's time to display a fingerprint
		if ((current->TimeForNext == 0) | (current->TimeForNext <= unixtime)) {
			CG_API_PRINTF("%d,%d\r\n",g_SID[0],g_SID[1]);
			doEmmFingerprint(current, unixtime);
			current->NumberOf--;
			if (current->NumberOf < 1) {
				// if the EMM was active on played video, then delay next ECM Fingerprint
				if (((current->SID == g_SID[0]) || (current->SID == 0)) && (gs_FingerprintEvent[0].DisplayFingerprint)) {
					gs_FingerprintEvent[0].TimeForNext =
							((int)unixtime + ((gLastEcmPtr31Data[0][3] & 0x1F) * 60)) +		// Add delay minutes until next from ECM Fingerprint data
							((current->EmmData[5] << 8 | current->EmmData[6]) /1000);	// Then add the duration of the last EMM fingerprint
				}
				// Blacklist this EMM as handeled
				setEmmHandled(current->CRC_Tag);
				// Remove the EMM from list
				del(current->CRC_Tag, g_EmmFingerprintList);
			}
		}
		current = currentNext;
	}

	for (CH=0; CH<2; CH++) {
		// Check For ECM Fingerprints in order
		if (gs_FingerprintEvent[CH].DisplayFingerprint) {
			// First check if it's the first time to display fingerprint
			if ((gs_FingerprintEvent[CH].TimeForNext == 0) | (gs_FingerprintEvent[CH].TimeForNext <= unixtime)) {
				// Make sure there isn't a SID specific or glabal EMM fingerprint active
				// EMM Fingerprint SID Specific > EMM Fingerprints > ECM Fingerprint
				if (!findSid(g_SID[CH], g_EmmFingerprintList)) {
					if (!findSid(0, g_EmmFingerprintList)) {
						doEcmFingerprint(CH, unixtime);
					}
				}
			}
		}
	}
	// If STB time has been set correct then handle Timedependent Events
	// And that it has been received within the last 5 minutes
	if ((time_offset.time_set) && (abs((int)time_offset.time_set - (int)unixtime) < FIVE_MINUTES ))
	{
		// Reset error countdown
		time_error_countdown = FIVE_MINUTES;
		// Adjust time so it's the same a SMS headend
		unixtime += time_offset.seconds;
		current = g_EmmMessageAndForcetuneList->head;
		// Check through EMM list if it's time for Forcetune or Message
		while(current != NULL) {
			Node *currentNext = current->next;
			if (current->EmmType == EMM_REMOVE) {
                // Blacklist this EMM as handeled
                setEmmHandled(current->CRC_Tag);
                // Remove from event list
                del(current->CRC_Tag, g_EmmMessageAndForcetuneList);
			}
            else if (current->TimeForNext <= unixtime) {
				switch (current->EmmType) {
					case EMM_FT32:
						Callback_STB_DoForcetune((delivery_descriptor*)current->EmmData);
						// Blacklist this EMM as handeled
						setEmmHandled(current->CRC_Tag);
						// Remove from event list
						del(current->CRC_Tag, g_EmmMessageAndForcetuneList);
						break;

					case EMM_MS33:
						// Check if SID specific matched or global message
						if ((current->SID == 0) | (current->SID == g_SID[0])) {
							Callback_STB_DisplayMessage((message_descriptor*)current->EmmData);
						}
						// Blacklist this EMM as handeled
						setEmmHandled(current->CRC_Tag);
						// Remove from event list
						del(current->CRC_Tag, g_EmmMessageAndForcetuneList);
						break;

					case EMM_SC35:
						Callback_STB_DisplayScrollMessage((scroll_descriptor*)current->EmmData);
						current->NumberOf--;
						current->TimeForNext = (unixtime + (current->Interval * 60))
                            + ((scroll_descriptor*)(current->EmmData))->Duration;

						if (current->NumberOf < 1) {
                            // Blacklist this EMM as handeled
                            setEmmHandled(current->CRC_Tag);
                            // Remove from event list
                            del(current->CRC_Tag, g_EmmMessageAndForcetuneList);
						}
						break;
				}
			}
			current = currentNext;
		}
	}
	else {
		if (!time_error_countdown--)
		{
            CG_API_PRINTF("\x1B[31mTime EMM not received for the last 5 minutes\x1B[0m\n");
		}
		CG_API_PRINTF("\x1B[31mTime EMM not received yet\x1B[0m\n");
	}
}

/*

*/

uint32_t crc32_table[256] = {
	0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
	0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
	0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
	0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
	0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
	0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
	0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
	0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
	0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
	0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
	0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
	0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
	0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
	0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
	0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
	0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
	0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
	0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
	0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
	0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
	0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
	0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
	0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
	0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
	0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
	0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
	0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
	0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
	0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
	0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
	0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
	0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
	0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
	0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
	0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
	0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
	0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
	0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
	0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
	0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
	0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
	0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
	0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

// ------------------
// INTERNAL FUNCTIONS
// ------------------

// <summary>
// Internal function
// Calculates 32 bits CRC checksum
// </summary>
// <param name="crc">Starting CRC value</param>
// <param name="data">Buffer to calculate CRC on</param>
// <param name="len">Size of buffer</param>
// <returns>The calculated CRC value</returns>
static uint32_t crc32_le (uint32_t crc, unsigned char const *data, size_t len) {
    size_t i;

	for (i=0; i<len; i++) {
		crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ (*data++)) & 0xff];
	}
	return crc;
}

// <summary>
// Internal function
// Check if a data packet has correct CRC checksum
// </summary>
// <param name="data">Pointer to the data packet</param>
// <returns>1 if the CRC of the data packet is ok, otherwise 0</returns>
static unsigned char CheckPacket(unsigned char const *data) {
    size_t len;
	uint32_t cr;

	len = (data[1] & 0x0f) << 8 | data[2];
	cr = crc32_le(~0, data, len-1);

	if((data[len-1] == ((cr >> 24) & 0xff) ) &
			(data[len] == ((cr >> 16) & 0xff) ) &
			(data[len+1] == ((cr >> 8) & 0xff) ) &
			(data[len+2] == (cr & 0xff))) {
		return 1;
	}
	return 0;
}

// Internal function
void ClearNVMEMRAM(void) {
	memset(&nvmem_cache, 0, sizeof(nvmem_storage));
}

// Internal function
void ReadNVMEMRAM(void) {
    size_t j;

	Callback_LoadMemory(&nvmem_cache);
	// Decrypt data
	for(j = 0; j < sizeof(nvmem_storage); j += 8) {
		ecb_3des((unsigned char *)&nvmem_cache+j, intcam_key, 1);
	}
}

// Internal function
void WriteNVMEMRAM(void) {
    size_t j;

	nvmem_cache.NvMemCRC = crc32_le(~0, (unsigned char const*)&nvmem_cache + sizeof(uint32_t) , sizeof(nvmem_storage) - sizeof(uint32_t));
	// Encrypt data before storage.
	for(j = 0; j < sizeof(nvmem_storage); j += 8) {
		ecb_3des((unsigned char *)&nvmem_cache+j, intcam_key, 0);
	}
	Callback_StoreMemory(&nvmem_cache);
	ClearNVMEMRAM();
}

/*
/ <summary>
/ API function
/ Main function for programming STB at factory wiith IRD number and decryption keys
/ run only once at production, never run at reboot
/ </summary>
/ <param name="initvalues">stucture filled with new IRD number and keys to be programmed to nvmem</param>
/ <param name="ForceReprogram">0 for normal programming 1 for Forced reprogramming</param>
/ <returns>0 if success otherwise it will return current IRD number</returns>
*/
uint32_t InitNVMEMRAM(nvmem_storage initvalues, unsigned char ForceReprogram) {
	uint32_t result, nvmem_CRC;

	ReadNVMEMRAM();
	nvmem_CRC = crc32_le(~0, (unsigned char const*)&nvmem_cache + sizeof(uint32_t) , sizeof(nvmem_storage) - sizeof(uint32_t));
	// Check if IRD number and keys already been programmed
	if ((nvmem_CRC == nvmem_cache.NvMemCRC) & (ForceReprogram == 0)){
		CG_API_PRINTF("CRC ok\r\n");
		result = nvmem_cache.IrdNumber;
		ClearNVMEMRAM();
		return result;
	}
	memset(&nvmem_cache, 0, sizeof(nvmem_storage));
	nvmem_cache.IrdNumber = initvalues.IrdNumber;
	memcpy(nvmem_cache.STB_SystemGlobalKey, initvalues.STB_SystemGlobalKey, 16);
	memcpy(nvmem_cache.STB_ManufacturerGlobalKey, initvalues.STB_ManufacturerGlobalKey, 16);
	memcpy(nvmem_cache.STB_GroupKey, initvalues.STB_GroupKey, 16);
	memcpy(nvmem_cache.STB_UniqueKey, initvalues.STB_UniqueKey, 16);
	memcpy(nvmem_cache.STB_CardlessGroupKey, initvalues.STB_CardlessGroupKey, 16);
	memcpy(nvmem_cache.STB_CardlessUniqueKey, initvalues.STB_CardlessUniqueKey, 16);

	WriteNVMEMRAM();
	return 0;
}

unsigned char em_hash[8], h_key[16], hptr;
// <summary>
// Internal function
// Clears the hash table in NVMEMRAM
// </summary>
// <param name="index">The memory address</param>
// <returns>nothing</returns>
static void EM_zeroHash(unsigned char index) {
	for(hptr = 0; hptr < 8; hptr ++)
		em_hash[hptr] = 0;
	hptr = 0;
	memcpy(h_key, "\x54\x84\x45\x12\x9c\xe4\x3f\x73\x28\xc8\x95\x35\x12\xcc\x12\x09", 16);
}

// internal function
static unsigned char EM_hashByte(unsigned char hashbyte) {

	em_hash[hptr] ^= hashbyte;
	if(hptr == 7) {
		ecb_3des(em_hash, h_key, 0);
		hptr = 0;
		return hashbyte;
	}
	hptr++;
	return hashbyte;
}

// internal function
static void EM_finishHash(void) {

	if(hptr) {
		ecb_3des(em_hash, h_key, 0);
	}
	memcpy(h_key, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\00", 16);
	hptr = 0;
}


static unsigned char CIPKey[16] = {
  0xe3, 0x56, 0x67, 0x19, 0xc4, 0x90, 0xa2, 0x27, 0xb5, 0xee, 0x46, 0x32, 0x83, 0x6d, 0x19, 0xf5
};

unsigned char URI[8];						// defined in ci+ plus specs.


// <summary>
// Internal function
// Processes the ECM information
// </summary>
// <param name="index"></param>
// <param name="ecm">Pointer to an ECM message</param>
// <returns>nothing</returns>
static void ProcessECMinfo(unsigned char CH, unsigned char index, unsigned char *ecm) {
	unsigned char ptr, ptr_31, cnt, pi, li, j,f1_found, chash[8], _15[8], _11[16], _19[8];
    unsigned char PI19 = 0, CIPSum[8];

	memset(_15, 0 , 8);
	memset(_11, 0 , 16);
	memset(_19, 0 , 8);

	f1_found = 0;
	ptr = 2;
	ptr_31 = 0;
	cnt = 128;
	CH &= 0x01;

	for(j = 0; j < 8; j ++)
		URI[j] = 0xFF;
	URI[0] = 0x02;		// URI Version 0x02

	EM_zeroHash(index);
	for(j = 0; j < 8; j ++)
		chash[j] = 0;

	do {
		pi = ecm[ptr+0];
		li = ecm[ptr+1];

		switch(pi) {

		case 0xF0:
		    CG_ECM_PRINTF("\x1B[31mF0\n\x1B[0m");
			break;

		case 0xF1:
            CG_ECM_PRINTF("\x1B[31mF1\n\x1B[0m");
			f1_found = 1;
			for(j = 0; j < 8; j ++)
				chash[j] = ecm[ptr+2+j];
			break;

		case 0x31:
			EM_hashByte(pi);
			EM_hashByte(li);
			for(j = 0; j < li; j ++)
				EM_hashByte(ecm[ptr+2+j]);

			ptr_31 = ptr;
			// First check if STB Control has changed
			if (g_LastSTBControl[CH] !=((uint32_t) (ecm[ptr_31+7] << 16) | (uint32_t) (((ecm[ptr_31+8] >> 4) & 0x0F) << 8) | ecm[ptr_31+9])) {
				g_LastSTBControl[CH] =(uint32_t) (ecm[ptr_31+7] << 16) | (uint32_t) (((ecm[ptr_31+8] >> 4) & 0x0F) << 8) | ecm[ptr_31+9];
				// Update with latest settings
				STB_VideoRules[CH].CGMSA = (ecm[ptr_31+7] >> 4) & 0x0F;
				STB_VideoRules[CH].HDCP = ecm[ptr_31+7] & 0x0F;
				STB_VideoRules[CH].Macrovision = (ecm[ptr_31+8] >> 4) & 0x0F;
				STB_VideoRules[CH].AnaHD = (ecm[ptr_31+9] >> 4) & 0x0F;
				STB_VideoRules[CH].DigHD = ecm[ptr_31+9] & 0x0F;
				Callback_STB_VideoRulesChanged(CH, &STB_VideoRules[CH]);
			}

			// Check if Fingerprint is still active
			if ((ecm[ptr_31 + 3] >> 6) & 0x03) {
				gs_FingerprintEvent[CH].DisplayFingerprint = 1;
			}
			// Check if fingerprint has changed attributes
			if (memcmp(&gLastEcmPtr31Data[CH], &ecm[ptr], (ecm[ptr+1] & 0x0F)+2)) {
				memcpy(&gLastEcmPtr31Data[CH], &ecm[ptr], (ecm[ptr+1] & 0x0F)+2);
				CG_ECM_PRINTF("FP has changed!\r\n");
				// Make sure Fingerprint with changed information is displayed next tick
				gs_FingerprintEvent[CH].TimeForNext = 0;
			}
			break;
        case 0x19:
            CG_ECM_PRINTF("\x1B[31mCIPlusURI received ");
            for(j = 0; j < li; j ++)
				CG_ECM_PRINTF("%02X", ecm[ptr+2+j]);
            CG_ECM_PRINTF("\n\x1B[0m");

            URI[1] = ecm[ptr+2];
			URI[2] = ecm[ptr+3];
			for(j = 0; j < 8; j ++) {
				_19[j] = ecm[ptr+4+j];
			}
			PI19 = 1;
			EM_hashByte(pi);
			EM_hashByte(li);
			for(j = 0; j < li; j ++)
				EM_hashByte(ecm[ptr+2+j]);
			break;

  		case 0x11:
			if(ecm[ptr+1] < 0x10) {
				return;
			}
			for(j = 0; j < 16; j ++) {
				_11[j] = ecm[ptr+2+j];
			}

			EM_hashByte(pi);
			EM_hashByte(li);
			for(j = 0; j < li; j ++)
				EM_hashByte(ecm[ptr+2+j]);

			break;

        case 0x15:
			if(ecm[ptr+1] != 0x08) {
				return;
			}
			for(j = 0; j < 8; j ++) {
				_15[j] = ecm[ptr+2+j];
			}

			EM_hashByte(pi);
			EM_hashByte(li);
			for(j = 0; j < li; j ++)
				EM_hashByte(ecm[ptr+2+j]);

			break;

		default:
			// card used pi, just calc hash
			CG_ECM_PRINTF("Card PI:%02X LI:%02X\r\n", pi, li);
			EM_hashByte(pi);
			EM_hashByte(li);
			for(j = 0; j < li; j ++)
				EM_hashByte(ecm[ptr+2+j]);

			break;
		}
		ptr += li + 2;
		// Safety cnt
		if(!cnt--) {
			CG_ECM_PRINTF("Safety cnt\r\n");
			return;
		}

	}
	while(pi != 0xf0);

//
#ifdef CAM
	// this handling CIPlus tags in ECM this feature is for CAM
	if (PI19) {

        // HASH Check

        CIPSum[0] = _15[0];
        CIPSum[1] = _15[1];
        CIPSum[2] = _15[2];
        CIPSum[3] = _15[3];
        CIPSum[4] = _15[4];
        CIPSum[5] = _15[5];
        CIPSum[6] = _15[6];
        CIPSum[7] = _15[7];
        ecb_3des(CIPSum, CIPKey, 0);
        CIPSum[0] ^= _11[0];
        CIPSum[1] ^= _11[1];
        CIPSum[2] ^= _11[2];
        CIPSum[3] ^= _11[3];
        CIPSum[4] ^= _11[4];
        CIPSum[5] ^= _11[5];
        CIPSum[6] ^= _11[6];
        CIPSum[7] ^= _11[7];
        ecb_3des(CIPSum, CIPKey, 0);
        CIPSum[0] ^= _11[8];
        CIPSum[1] ^= _11[9];
        CIPSum[2] ^= _11[10];
        CIPSum[3] ^= _11[11];
        CIPSum[4] ^= _11[12];
        CIPSum[5] ^= _11[13];
        CIPSum[6] ^= _11[14];
        CIPSum[7] ^= _11[15];
        ecb_3des(CIPSum, CIPKey, 0);
        CIPSum[0] ^= URI[1];
        CIPSum[1] ^= URI[2];
        ecb_3des(CIPSum, CIPKey, 0);


        printf("CIPSum: ");
        for(j = 0; j < 8; j ++) {
			printf("%02X ", CIPSum[j]);
        }
        printf("\n");

        if( (CIPSum[0] == _19[0]) && (CIPSum[1] == _19[1]) && (CIPSum[2] == _19[2]) && (CIPSum[3] == _19[3]) &&
            (CIPSum[4] == _19[4]) && (CIPSum[5] == _19[5]) && (CIPSum[6] == _19[6]) && (CIPSum[7] == _19[7])) {

            printf("URI present and valid.\n");
        } else {
   		    CG_ECM_PRINTF("\x1B[31m");
            printf("HASH failure, URI is invalid.\n");
   		    CG_ECM_PRINTF("\x1B[0m");
        }
        printf("PI19 ");
        for(j = 0; j < 8; j ++) {
			printf("%02X ", _19[j]);
        }
        printf("\n");
    }
#endif // CAM

	EM_finishHash();

	// if there was no 0x31 PI there are no Fingerprint data and Fingerprint has stopped, and STBControl are all 0
	if(!ptr_31) {
		gs_FingerprintEvent[CH].DisplayFingerprint = 0;
		if (g_LastSTBControl[CH])
		{
			g_LastSTBControl[CH] = 0;
			memset(&STB_VideoRules[CH], 0, sizeof(ecm_STB_videorules));
			Callback_STB_VideoRulesChanged(CH, &STB_VideoRules[CH]);
		}
	}

	// Compare hash
	if(memcmp(em_hash, chash, 8)) {
		if (f1_found)
			CG_ECM_PRINTF("F1: Hash Fail! CH:%d\r\n", CH);
		return;
	}
	CG_ECM_PRINTF("F1: Hash ok!\r\n");
}
/**
 * @brief Parses inner part of EMMs to find Fingerprints, Forcetunes, Message, Scroll Messages and timestamps
 *
 * @param emm
 * @param tot_len
 * @param unixtime
 */
static void ProcessSTBEMMinfo(unsigned char *emm, short tot_len, time_t unixtime) {
	unsigned short parse_index, msg_len, tag_len, SID;
	delivery_descriptor forcetune;
	message_descriptor message;
	scroll_descriptor scroll;
	uint32_t in_EMM_crc;
	Node * EmmEvent = NULL;
	Node * current;
	time_t emmtime;

	in_EMM_crc = crc32_le(~0, emm, tot_len-1);

	// check if we already received and handled this EMM
	if (!checkIfEmmHandled(in_EMM_crc)) {
		CG_EMM_PRINTF("Repeat already handled EMM received\r\n");
		return;
	}

	parse_index = 0;
	while (parse_index < tot_len) {

		switch(emm[parse_index++]) {

		// STB Control string
		case 0xEF:
			tag_len = ((unsigned short)emm[parse_index] << 8) | (unsigned short)emm[parse_index + 1];

			if (!(emm[parse_index + 9 - ((g_CardNumber % 64) / 8)] & (1 << ((g_CardNumber % 64) % 8)))) {
				CG_EMM_PRINTF("My card is NOT listed !!\r\n");
				// Ignore rest of message
				parse_index = tot_len;
			}
			parse_index += tag_len + 2;
			break;

		// Fingerprint
		case EMM_FP31:
			tag_len = (((unsigned short)emm[parse_index] << 8) | (unsigned short)emm[parse_index + 1]);
			SID = emm[parse_index + 15] << 8 | emm[parse_index + 16];
			// First check that there is not already a FP on that SID
			if (!findSid(SID, g_EmmFingerprintList)){
				EmmEvent = add(in_EMM_crc, tag_len, g_EmmFingerprintList);
				if (EmmEvent != NULL) {
					CG_EMM_PRINTF("\x1B[33mAdded new fingerprint\x1B[0m\r\n");
					EmmEvent->EmmType = EMM_FP31;
					EmmEvent->SID = SID;
					// Set how many times to be displayed
					EmmEvent->NumberOf = emm[parse_index + 11] << 8 | emm[parse_index + 12];
					// Set time for next to 0 to start display as soon as possible
					EmmEvent->TimeForNext = 0;
					memcpy(EmmEvent->EmmData, &emm[parse_index], tag_len);
				}
			}
			else
			{
                CG_EMM_PRINTF("Rejected for now\r\n");
            }
			parse_index += tag_len + 2;
			break;

		// Force tuning
		case EMM_FT32:
			tag_len = ((unsigned short)emm[parse_index] << 8) | (unsigned short)emm[parse_index + 1];
			// If STB time has been set correct then handle Timedependent Events
			// And that it has been received within the last 5 minutes
			if ((time_offset.time_set) && (abs((int)time_offset.time_set - (int)unixtime) < FIVE_MINUTES ))
			{
				emmtime = getEMMtime(&emm[parse_index+3]);
				// Check if event already passed
				if (((unixtime + time_offset.seconds) - 900) > emmtime)
				{
					setEmmHandled(in_EMM_crc);
					CG_EMM_PRINTF("Emm Event time already passed\r\n");
				}
				else {
					// check that it's a version 1 forcetune
					if (emm[parse_index + 2] == 1) {
						EmmEvent = add(in_EMM_crc, sizeof(delivery_descriptor), g_EmmMessageAndForcetuneList);
						if (EmmEvent != NULL) {
							CG_EMM_PRINTF("\x1B[33mAdded new forcetuning\x1B[0m\n");
							EmmEvent->EmmType = EMM_FT32;
							forcetune.DestinationSID = (unsigned short) emm[parse_index + 8] << 8 | (unsigned short) emm[parse_index + 9];
							forcetune.tagvalue = emm[parse_index + 10];
							forcetune.len = emm[parse_index + 11];// should be 11
							memcpy(&forcetune.descriptordata, &emm[parse_index + 12], 11);// use 11 to protect against copy outside of descriptordata size
							EmmEvent->TimeForNext = emmtime;
							memcpy(EmmEvent->EmmData, &forcetune, sizeof(delivery_descriptor));
						}
					}
				}
			}
			parse_index += tag_len + 2;
			break;

		// Messaging
		case EMM_MS33:
			tag_len = ((unsigned short)emm[parse_index] << 8) | (unsigned short)emm[parse_index + 1];

			// If STB time has been set correct then handle Timedependent Events
			// And that it has been received within the last 5 minutes
			if ((time_offset.time_set) && (abs((int)time_offset.time_set - (int)unixtime) < FIVE_MINUTES ))
			{
				emmtime = getEMMtime(&emm[parse_index+4]);
				// Check if event already passed
				if (((unixtime + time_offset.seconds) - 900) > emmtime) {
					setEmmHandled(in_EMM_crc);
					CG_EMM_PRINTF("EMM Event time has already passed\r\n");
				}
				else {
					// Make sure it's a version 1 message
					if (emm[parse_index + 2] == 1) {
						EmmEvent = add(in_EMM_crc, sizeof(message_descriptor), g_EmmMessageAndForcetuneList);
						if (EmmEvent != NULL) {
							EmmEvent->EmmType = EMM_MS33;
							EmmEvent->SID = (unsigned short) emm[parse_index + 11] << 8 | (unsigned short) emm[parse_index + 12];
							memset (&message, 0, sizeof(message_descriptor));
							message.Forced = emm[parse_index + 3];
							message.Duration = (unsigned short) emm[parse_index + 9] << 8 | (unsigned short) emm[parse_index + 10];
							msg_len = (unsigned short)emm[parse_index + 13] << 8 | (unsigned short)emm[parse_index + 14];
							memcpy(message.Text, &emm[parse_index + 15], msg_len);
							message.Text[msg_len] = 0; // NULL terminate string
							EmmEvent->TimeForNext = emmtime;
							memcpy(EmmEvent->EmmData, &message, sizeof(message_descriptor));
						}
					}
				}
			}
			parse_index += tag_len + 2;
			break;

		case EMM_TM34:
		    tag_len = ((unsigned short)emm[parse_index] << 8) | (unsigned short)emm[parse_index + 1];
			time_offset.time_set = unixtime;
			time_offset.seconds = ((emm[parse_index + 2] << 24) | (emm[parse_index + 3] << 16) | (emm[parse_index + 4] << 8) | emm[parse_index + 5]) - time_offset.time_set;
			CG_EMM_PRINTF("\x1B[36mEMM_TIME offset : %2d\x1B[0m\n", (int)time_offset.seconds);
			parse_index += tag_len + 2;
			break;

		case EMM_SC35:
			tag_len = ((unsigned short)emm[parse_index] << 8) | (unsigned short)emm[parse_index + 1];
		    // Make sure it's a version 1 message
            if (emm[parse_index + 2] == 1) {
                EmmEvent = add(in_EMM_crc, sizeof(scroll_descriptor), g_EmmMessageAndForcetuneList);
                if (EmmEvent != NULL) {
                    // First find and remove any older scrolling messages
                    current = g_EmmMessageAndForcetuneList->head;
                    while (current != NULL)	{
                        if (current->EmmType == EMM_SC35) {
                            current->EmmType = EMM_REMOVE;
                            CG_EMM_PRINTF("\x1B[33mRemoved old still active scroll \x1B[0m\r\n");
                        }
                        current = current->next;
                    }
                    EmmEvent->TimeForNext = 0xFFFFFFFF;
                    CG_EMM_PRINTF("\x1B[33mAdded new scrolling message to be displayed %d time(s).\x1B[0m\r\n", emm[parse_index + 15]);
                    EmmEvent->EmmType = EMM_SC35;
                    EmmEvent->Interval = emm[parse_index + 12];
                    EmmEvent->NumberOf = emm[parse_index + 15];
                    memset (&scroll, 0, sizeof(scroll_descriptor));

                    scroll.Forced = emm[parse_index + 8];
                    scroll.FontSize = emm[parse_index + 11];
                    scroll.Duration = (unsigned short) emm[parse_index + 13] << 8 | (unsigned short) emm[parse_index + 14];
                    scroll.FontColor = fp1color[emm[parse_index + 16]];
                    scroll.BackgroundColor = fp1color[emm[parse_index + 17]];
                    scroll.Y = emm[parse_index + 18];
                    scroll.ScrollSpeed = emm[parse_index + 19] << 8 | (unsigned short)emm[parse_index + 20];
                    msg_len = (unsigned short)emm[parse_index + 22];
                    memcpy(scroll.Text, &emm[parse_index + 23], msg_len);
                    scroll.Text[msg_len] = 0; // NULL terminate string just in case it's not
                    EmmEvent->TimeForNext = 0;
                    memcpy(EmmEvent->EmmData, &scroll, sizeof(scroll_descriptor));
                }
            }
            else {
                    CG_EMM_PRINTF("Unknown scrolling message version\r\n");
            }
			parse_index += tag_len + 2;
			break;

		default:
			tag_len = ((unsigned short)emm[parse_index] << 8) | (unsigned short)emm[parse_index + 1];
			CG_EMM_PRINTF("Unknown tag: %02X in a 0x86 Message\r\n", emm[parse_index-1]);
			parse_index += tag_len + 2;
			break;
		}
	}
}

// <summary>
// Internal function
//
// </summary>
// <param name="PI"></param>
// <param name="LI"></param>
// <returns></returns>
static unsigned char collectBytes(unsigned char PI, unsigned char LI, unsigned char maxlen, unsigned char *data) {
	unsigned char ptr;

	if(!LI)
		LI = data[1];

	ptr = 0;
	do {
		if( (data[ptr] == PI) & (data[ptr+1] == LI) ) {
			ptr += 2;
			return ptr;
		}
		ptr += LI + 2;
	}
	while(maxlen < ptr);
	return 0;
}

/*
/ <summary>
/ External function
/ Returns the EMM PID from the CAT section
/ </summary>
/ <param name="cat_section">The CAT section to search EMM PID</param>
/ <returns>Returns the EMM PID if success otherwise returns 0 and sets the global variable errorcode to 1 = CRC failure, 2 = Table failure, 3 = CA-ID not found, 4 = Initilization error</returns>
*/
unsigned short getEMMPid(unsigned char *cat_section) {
	unsigned short section_len, section_ptr, emm_pid = 0;
	unsigned char li, faultycnt, i = 0;

	if(!g_Caid) {
		CG_EMM_PRINTF("cam init error %04X", g_Caid);
		errorcode = CAM_INIT_ERROR;
		return 0;
	}

	if(cat_section[0] != 0x01) {
		errorcode = CAM_TABLE_FAILURE;
		return 0;
	}
	section_len = ((unsigned short) cat_section[1] << 8 | (unsigned short) cat_section[2]) & 0x0fff;
	// CRC check
	if(!CheckPacket(cat_section)) {
		errorcode = CAM_CRC_FAILURE;
		return 0;
	}
	section_len -= 5;
	section_ptr = 8;
	faultycnt = 255;
	do {
		li = cat_section[section_ptr+1];

        cat_caid_pid[i][0] = (cat_section[section_ptr+2] << 8 | cat_section[section_ptr+3]);
		cat_caid_pid[i][1] = (cat_section[section_ptr+4] << 8 | cat_section[section_ptr+5]) & 0x1fff;
		if(g_Caid == cat_caid_pid[i][0]) {
			errorcode = CAM_SUCCESS;
			emm_pid = cat_caid_pid[i][1];
		}
		i++;
		section_ptr += (li + 2);
		if(!faultycnt--) {
			errorcode = CAM_TABLE_FAILURE;
			return 0;
		}
	} while( (section_ptr < section_len));

	if (emm_pid){
        errorcode = CAM_SUCCESS;
        return emm_pid;
	}

	errorcode = CAM_CAID_NOTFOUND;
	return 0;
}

/*
/ <summary>
/ External function
/ Returns the ECM PID from the PMT section , only returns first for found PID
/ </summary>
/ <param name="pmt_section">The PMT section to search ECM PID</param>
/ <returns>Returns the ECM PID if success otherwise returns 0 and sets the global variable errorcode to 1 = CRC failure, 2 = Table failure, 3 = CA-ID not found, 4 = Initilization error</returns>
*/
unsigned short  getECMPid(unsigned char CH, unsigned char *pmt_section) {
	unsigned char  *discriptor_pointer = NULL;
	unsigned char  *discriptor_pointer2 = NULL;
	unsigned char  loop_length = 0;
	unsigned short prog_info_len = 0;
	unsigned short section_len = 0;
	unsigned short es_info_len = 0;
	unsigned short left_len = 0;
	unsigned short SID = 0;

	CH &= 0x01;
	if(!g_Caid) {
		CG_ECM_PRINTF("cam init error %04x\r\n", g_Caid);
		errorcode = CAM_INIT_ERROR;
		return 0;
	}
	if((pmt_section == NULL) || (pmt_section[0] != 0x02)) {
		errorcode = CAM_TABLE_FAILURE;
		return 0;
	}
	// CRC check
	if(!CheckPacket(pmt_section)) {
		errorcode = CAM_CRC_FAILURE;
		return 0;
	}
	section_len = ((pmt_section[1] & 0x03) << 8) | pmt_section[2];
	// IF SID has changed deactivate ECM fingerprint
	SID = ((unsigned short) pmt_section[3] << 8 | (unsigned short) pmt_section[4]);
	if (g_SID[CH] != SID) {
		g_SID[CH] = SID;
		memset(&gs_FingerprintEvent[CH], 0, sizeof(ecm_fingerprint_events));
		if (gStrictParentalRating == 1) {
            lockWithoutPIN();
        }
	}

	prog_info_len = ((pmt_section[10] & 0x03) << 8) | pmt_section[11];
	left_len = section_len - 9 - prog_info_len - 4;
	discriptor_pointer = &pmt_section[12];
	discriptor_pointer2 = discriptor_pointer + prog_info_len;

	while (prog_info_len > 0) {
		// check match CA descriptor tag
		if (0x09 == *discriptor_pointer) {
			if (((discriptor_pointer[2] << 8) | discriptor_pointer[3]) == g_Caid) {
				errorcode = CAM_SUCCESS;
				return ((discriptor_pointer[4] & 0x1F) << 8) | discriptor_pointer[5];
			}
		}
		prog_info_len -= 2 + discriptor_pointer[1];
		discriptor_pointer += 2 + discriptor_pointer[1];
	}
	discriptor_pointer = discriptor_pointer2;

	while (left_len > 0) {
		es_info_len = ((discriptor_pointer[3] & 0x03) << 8) | discriptor_pointer[4];
		loop_length = es_info_len;
		discriptor_pointer += 5;

		while (loop_length > 0)	{
			// check match CA descriptor tag
			if (0x09 == *discriptor_pointer) {
				if (((discriptor_pointer[2] << 8) | discriptor_pointer[3]) == g_Caid) {
					errorcode = CAM_SUCCESS;
					return ((discriptor_pointer[4] & 0x1F) << 8) | discriptor_pointer[5];
				}
			}
			loop_length -= 2 + discriptor_pointer[1];
			discriptor_pointer += 2 + discriptor_pointer[1];
		}
		left_len -= 5 + es_info_len;
	}
    // didn't find any ECM pid

    errorcode = CAM_CAID_NOTFOUND;
	return 0;
}

// <summary>
// External function
// Main function for processing ECM
// </summary>
// <param name="CH">current stream 0 or 1</param>
// <param name="ecm_section">Pointer to the ECM section in the stream</param>
// <returns>Returns 1 if success otherwise returns 0 and sets the global variable errorcode to 1 = CRC failure, 2 = Table failure, 5 = CAM not addressed, 6 = ECM already processed, 7 = CAM no success</returns>
unsigned short ProcessECM(unsigned char CH, unsigned char *ecm_section) {
	unsigned char head[5], buffer[256], index;
#ifdef _DEBUG_ECM
	unsigned char j;
#endif // _DEBUG_ECM
	unsigned short sw, length;
	uint32_t mcn, in_ecm_crc;
	CH &= 0x01;

	if(!( (ecm_section[0] == 0x80) || (ecm_section[0] == 0x81) )) {
		errorcode = CAM_TABLE_FAILURE;
		return 0;
	}

	length = ((ecm_section[1] & 0x0F) << 8) + ecm_section[2] + 3;
    in_ecm_crc = crc32_le(~0, ecm_section, length-1);

    if (in_ecm_crc == last_ecm_crc[CH]) {
        errorcode = CAM_ECM_ALREADY_PROCESSED;
        return 0;
    }
    last_ecm_crc[CH] = in_ecm_crc;

#ifdef _DEBUG_ECM
	if (ecm_section[0] == 0x81)
		CG_ECM_PRINTF("CH %d ECM: \x1B[32m", CH);
	else
		CG_ECM_PRINTF("CH %d ECM: \x1B[34m", CH);

	printSection(ecm_section);
	CG_ECM_PRINTF("\x1B[0m");
#endif

	// Soft CA check
	index = ecm_section[8] & 0x0F;
	ProcessECMinfo(CH, index, ecm_section+9);

	memcpy(head, "\xa0\x10\x00\x00", 4);
	head[2] = CH;
	head[3] = ecm_section[8];
	head[4] = ecm_section[2] - 6;
	sw = Callback_ISO7816_TransmitCommand(head, ecm_section+9);

	switch( (sw & 0xFF00) ) {

		case 0x9100:
			memcpy(head, "\xa0\x20\x00\x00", 4);
			head[4] = (unsigned char) (sw & 0x00ff);
            if (Callback_ISO7816_ReceiveCommand(head, buffer) == 0x9900) {
				errorcode = CAM_CARD_ERROR;
				return 0;
			}

			if (buffer[0] == 7)
			{
				buffer[ buffer[1] + 2 ] = 0;
				Callback_PinRequired(buffer[2] * 1000, (char *) buffer+3);
			}
			else
			{
                buffer[ buffer[1] + 2 ] = 0;
                Callback_OSDMessage(buffer[2] * 1000, (char *) buffer+3);
            }
			errorcode = CAM_NOACCESS;
			return 0;

		case 0x9400:
			memcpy(head, "\xa0\x20\x00\x00", 4);
			head[4] = (unsigned char) (sw & 0x00ff);
            if (Callback_ISO7816_ReceiveCommand(head, buffer) == 0x9900) {
				errorcode = CAM_CARD_ERROR;
				return 0;
			}

			memcpy(garr_CW1[CH], buffer+2, 8);
			memcpy(garr_CW2[CH], buffer+12, 8);
			break;

		case 0x9500:
			ReadNVMEMRAM();
			mcn = nvmem_cache.STB_PairingCardnumber;
			ClearNVMEMRAM();
			// Check if not paired.
			CG_ECM_PRINTF("MCN:%011u\r\n",mcn);
			if(mcn == 0 ) {
				Callback_OSDMessage(5 * 1000, "This channel is locked. Please call your operator.");
				return 0;
				break;
			}
			// Paired to another card.
			if(mcn != g_CardNumber) {
				sprintf((char *) buffer, "Please use card %011u in this set top box.", mcn);
				Callback_OSDMessage(5 * 1000, (char *) buffer);
				return 0;
				break;
			}
			memcpy(head, "\xa0\x20\x00\x00", 4);
			head[4] = (unsigned char) (sw & 0x00ff);
            if (Callback_ISO7816_ReceiveCommand(head, buffer) == 0x9900) {
				errorcode = CAM_CARD_ERROR;
				return 0;
			}

			ReadNVMEMRAM();
			ecb_3des(nvmem_cache.STB_PairingKey, nvmem_cache.STB_UniqueKey, 1);
			ecb_3des(nvmem_cache.STB_PairingKey+8, nvmem_cache.STB_UniqueKey, 1);
			ecb_3des(buffer+2, nvmem_cache.STB_PairingKey, 1);
			ecb_3des(buffer+12, nvmem_cache.STB_PairingKey, 1);
			ClearNVMEMRAM();

			memcpy(garr_CW1[CH], buffer+2, 8);
			memcpy(garr_CW2[CH], buffer+12, 8);
			break;

		case 0x9900:
			errorcode = CAM_NOACCESS;
			return 0;

		default:
			break;
	};

#ifdef _DEBUG_ECM
	CG_ECM_PRINTF("Controlword 1: ");
	for(j = 0; j < 8; j ++) {
		CG_ECM_PRINTF("%02X ", garr_CW1[CH][j]);
	}
	CG_ECM_PRINTF("\r\n");

	CG_ECM_PRINTF("Controlword 2: ");
	for(j = 0; j < 8; j ++) {
		CG_ECM_PRINTF("%02X ", garr_CW2[CH][j]);
	}
	CG_ECM_PRINTF("\r\n");
#endif

	CPG_SetDescrCW(8, garr_CW1[CH], garr_CW2[CH], 0, 0);
	errorcode = CAM_SUCCESS;
	return 1;
}

static bool SendConfigToCard(void) {

	unsigned char buffer[32], head[5];
	unsigned short sw;

    if ((garr_Version[0] >= 1) && (garr_Version[1] >=18))
    {
        memcpy(head, "\xa0\x04\x00\x00", 4);

        buffer[0] = IRDNUMBER;
        buffer[1] = 8; //Size
		buffer[2] = 0;
        buffer[3] = 0;
        buffer[4] = 0;
        buffer[5] = 0;
        buffer[6] = (g_IrdNumber >> 24) & 0xFF;
        buffer[7] = (g_IrdNumber >> 16) & 0xFF;
        buffer[8] = (g_IrdNumber >> 8) & 0xFF;
        buffer[9] = (g_IrdNumber) & 0xFF;

        buffer[10] = PAIREDWITH;
		buffer[11] = 8; //Size
        buffer[12] = 0;
        buffer[13] = 0;
        buffer[14] = 0;
        buffer[15] = 0;
        ReadNVMEMRAM();
        buffer[16] = (nvmem_cache.STB_PairingCardnumber >> 24) & 0xFF;
        buffer[17] = (nvmem_cache.STB_PairingCardnumber >> 16) & 0xFF;
        buffer[18] = (nvmem_cache.STB_PairingCardnumber >> 8) & 0xFF;
        buffer[19] = (nvmem_cache.STB_PairingCardnumber) & 0xFF;
        ClearNVMEMRAM();

        buffer[20] = PAIRING_MODE;
        buffer[21] = 1;
        buffer[22] = SOFT_PAIRING;

        head[4] = 23;//sizeof(config_string);

        // As example if card 00010012345 and stbid 00990000001, then buffer should be
        // 01 08 00 00 00 00 3b 02 33 81 02 08 00 00 00 00 00 98 c6 b9 03 01 01 90 00

        sw = Callback_ISO7816_TransmitCommand(head, buffer);
        if ((sw & 0xFF00) != 0x9000)
        {
            CG_API_PRINTF("Send config to card failed\n");
            return FALSE;
        }
    }
    return TRUE;
}

static bool group_adress_valid(unsigned char *emm_section, unsigned short length)
{
	unsigned char* section_ptr = emm_section;
	do {
		switch (section_ptr[0]) {
		case 0xEF:
			if (section_ptr[2 + 7 - ((g_CardNumber & 0x3F) / 8)] & (1 << (g_CardNumber & 0x07))) {
				CG_EMM_PRINTF("Group bit found\n");
				return true;
			}
			CG_EMM_PRINTF("Group bit NOT found\n");
			break;
		default:
			break;
		}
		section_ptr += (section_ptr[1] + 2);
	} while (section_ptr < (emm_section + length));
	CG_EMM_PRINTF("No valid group address found\n");
	return false;
}

// <summary>
// External function
// Main function for processing EMM
// </summary>
// <param name="emm_section">Pointer to the EMM section in the stream</param>
// <returns>Returns 1 if success otherwise returns 0 and sets the global variable errorcode to 5 = CAM not addressed</returns>
unsigned short ProcessEMM(unsigned char *emm_section, time_t unixtime) {
	unsigned short sw, length;
	unsigned char head[5];
	unsigned char buffer[256];
	uint32_t emm_adress, pcardnr;
	ota_trigger Ota;

	// EMM Filters for CardNumber GroupNumber IRDNumber or Broadcast
	emm_adress = emm_section[6] << 24 | emm_section[7] << 16 | emm_section[8] << 8 | emm_section[9];
	if(!((emm_adress == g_CardNumber) || (emm_adress == g_GroupNumber) || (emm_adress == g_IrdNumber) || (emm_adress == 0) )) {
		errorcode = CAM_NOT_ADRESSED;
		return 0;
	}
    length = ((emm_section[1] & 0x0F) << 8) + emm_section[2] + 3;

    // if group is adressed, check my card is tagged
    if (emm_adress == g_GroupNumber) {
        if (!group_adress_valid(emm_section + 12, length - 12)) {
            errorcode = CAM_NOT_ADRESSED;
            return 0;
        }
    }
#ifdef _DEBUG_EMM
    CG_EMM_PRINTF("\r\nEMMType:%02x ", emm_section[0]);
	if (emm_adress == g_CardNumber)
		CG_EMM_PRINTF("Card adressed\r\n");
	else if (emm_adress ==  g_GroupNumber)
		CG_EMM_PRINTF("Group adressed: %011u  , %02u\r\n", g_GroupNumber, g_CardNumber & 0x3F);
	else if (emm_adress ==  g_IrdNumber)
		CG_EMM_PRINTF("IRD adressed\r\n");
	else if (emm_adress ==  0)
		CG_EMM_PRINTF("Zero adressed\r\n");
	CG_EMM_PRINTF("EMM raw Data: ");
	printSection(emm_section);
#endif

	switch(emm_section[0]) {
	// 0x82 section is card emm
	case 0x82:
		memcpy(head, "\xa0\x12\x00\x00", 4);
		head[4] = emm_section[2] - 9;
		head[3] = emm_section[11];
		sw = Callback_ISO7816_TransmitCommand(head, emm_section+12);
		switch( (sw & 0xFF00) ) {
		case 0x9000:
        CG_EMM_PRINTF("****** 0x9000 *******\r\n");
		break;

		case 0x9100:
			// Card has xx bytes of information to read out for the receiver by INS 20
			CG_EMM_PRINTF("****** 0x9100 *******\r\n");
			memcpy(head, "\xa0\x20\x00\x00", 4);
			head[4] = (unsigned char) (sw & 0x00ff);
            if (Callback_ISO7816_ReceiveCommand(head, buffer) == 0x9900) {
				errorcode = CAM_CARD_ERROR;
				return 0;
			}

			switch(buffer[0]) {
			case 0x01:// OSD Message
				buffer[ buffer[1] + 2 ] = 0;
				Callback_OSDMessage(buffer[2] * 1000, (char *) buffer+3);
				break;
			// 050100 = MASTER
			// 050101 = SLAVE
			case 0x05:// Multiroom CFG String
				if (buffer[2] == 0)
					CG_EMM_PRINTF("Master STB\r\n");
				if (buffer[2] == 1)
					CG_EMM_PRINTF("Slave STB\r\n");

				break;
			}
			break;

		case 0x9300:
			// Card has xx pairing configuration bytes for the receiver, read out by INS 20
			printf("\x1B[31m");
			CG_EMM_PRINTF("*********************\r\n");
			CG_EMM_PRINTF("****** 0x9300 *******\r\n");
			CG_EMM_PRINTF("*********************\r\n");
			printf("\x1B[0m");
			memcpy(head, "\xa0\x20\x00\x00", 4);
			head[4] = (unsigned char) (sw & 0x00ff);
			if (Callback_ISO7816_ReceiveCommand(head, buffer) == 0x9900) {
				errorcode = CAM_CARD_ERROR;
				return 0;
			}

            ReadNVMEMRAM();
            ecb_3des(buffer+18, nvmem_cache.STB_UniqueKey, 1);
            ClearNVMEMRAM();

			// Same cn check
			pcardnr = (uint32_t) (buffer[18] << 24) | (uint32_t) (buffer[19] << 16) | (uint32_t) (buffer[20] << 8) | buffer[21];
			if(pcardnr != g_CardNumber) {
				break;
			}

			// Fill buffer check
			if(buffer[22] ^ buffer[23] ^ buffer[24] ^ buffer[25]) {
				break;
			}

			ReadNVMEMRAM();
			memcpy(&nvmem_cache.STB_PairingKey, buffer+2, 16);
			nvmem_cache.STB_PairingCardnumber = pcardnr;
			WriteNVMEMRAM();
			SendConfigToCard();
			CG_EMM_PRINTF("*   STB Nr Stored   *\r\n");
			break;

		case 0x9900:
			CG_EMM_PRINTF("*   0x9900 FAIL !!!!!!  *\r\n");
			break;

		default:
			CG_EMM_PRINTF("*   Unknown answer  %04X *\r\n", sw);
			break;
		}
		break;

	// 0x83 EMM OTA Triggers
	case 0x83:
        // Check if version 0
        if (emm_section[4] != 0) {
            memset(&Ota, 0, sizeof(Ota));
            errorcode = CAM_EMM_OTA_ERROR;
            return 0;
        }
        // First check is to make sure trigger is for me
        Ota.Manufacturer = (uint32_t) (emm_section[24] << 24) | (uint32_t) (emm_section[25] << 16) | (uint32_t) (emm_section[26] << 8) | emm_section[27];
        if (Ota.Manufacturer == THIS_MANUFACTURER)
        {
			if (memcmp(emm_section+10, "CRYPTOGUARDOTA", 14) == 0) {
				Ota.ForcedUpgrade = FALSE;
			}
			else if (memcmp(emm_section+10, "CRYPTOFORCEOTA", 14) == 0) {
				Ota.ForcedUpgrade = TRUE;
			}
			else {
				memset(&Ota, 0, sizeof(Ota));
				errorcode = CAM_EMM_OTA_ERROR;
				return 0;
			}
			Ota.Manufacturer    = (uint32_t) (emm_section[24] << 24) | (uint32_t) (emm_section[25] << 16) | (uint32_t) (emm_section[26] << 8) | emm_section[27];
			Ota.Model           = (uint32_t) (emm_section[28] << 24) | (uint32_t) (emm_section[29] << 16) | (uint32_t) (emm_section[30] << 8) | emm_section[31];
			Ota.Version         = (uint32_t) (emm_section[32] << 24) | (uint32_t) (emm_section[33] << 16) | (uint32_t) (emm_section[34] << 8) | emm_section[35];
			Ota.PID             = (uint32_t) (emm_section[36] << 24) | (uint32_t) (emm_section[37] << 16) | (uint32_t) (emm_section[38] << 8) | emm_section[39];
			Ota.Delivery        = (uint32_t) (emm_section[40] << 24) | (uint32_t) (emm_section[41] << 16) | (uint32_t) (emm_section[42] << 8) | emm_section[43];
			Ota.Frequency       = (uint32_t) (emm_section[44] << 24) | (uint32_t) (emm_section[45] << 16) | (uint32_t) (emm_section[46] << 8) | emm_section[47];
			Ota.Symbolrate      = (uint32_t) (emm_section[48] << 24) | (uint32_t) (emm_section[49] << 16) | (uint32_t) (emm_section[50] << 8) | emm_section[51];
			Ota.Fec             = (uint32_t) (emm_section[52] << 8) | emm_section[53];
			Ota.Polarity        = (uint32_t) (emm_section[54] << 8) | emm_section[55];
			Ota.Modulation      = (uint32_t) (emm_section[56] << 8) | emm_section[57];
			Ota.Bandwith        = (uint32_t) (emm_section[58] << 8) | emm_section[59];
			Callback_STB_OtaTriggerReceived(&Ota);
        }
		break;

	// 0x86 EMM Fingerprint Focetune and messages     // STB emming
	case 0x86:
		if (emm_section[12] == 0xEA) {
			ProcessSTBEMMinfo(emm_section + 15, (((unsigned short) emm_section[13] << 8) |  (unsigned short) emm_section[14]), unixtime);
		}
		break;

	default:
#ifdef _DEBUG_EMM
		CG_EMM_PRINTF("Default\r\n");
		printSection(emm_section);
#endif
		break;

	}
	errorcode = CAM_SUCCESS;
	return 1;
}

/// <summary>
/// External function
/// Initilizes the CA system and check status of card
/// </summary>
/// <returns>Returns true if OK and false if not. When success sets the global variable g_IrdNumber</returns>
unsigned char CAInit(void) {
	unsigned short sw;
	unsigned char head[5];
	unsigned char buffer[256], ptr;
	uint32_t nvmem_CRC;

	srand((unsigned int)time(NULL));
	memset(&gLastEcmPtr31Data, 0, 2*18);
	memset(&STB_VideoRules, 0, 2*sizeof(ecm_STB_videorules));

	// Do not reallocate list if CAInit is called again without a CADeInit between
	if (g_EmmFingerprintList == NULL) {
		g_EmmFingerprintList = initList();
	}
	if (g_EmmMessageAndForcetuneList == NULL) {
		g_EmmMessageAndForcetuneList = initList();
	}

	// Send empty STB rules before starting to receive ECM
	Callback_STB_VideoRulesChanged(0, &STB_VideoRules[0]);
	Callback_STB_VideoRulesChanged(1, &STB_VideoRules[1]);

	ReadNVMEMRAM();
	nvmem_CRC = crc32_le(~0, (unsigned char const*)&nvmem_cache + sizeof(uint32_t) , sizeof(nvmem_storage) - sizeof(uint32_t));
	// Check if IRD number and keys been programmed correct
	if (nvmem_CRC == nvmem_cache.NvMemCRC){
		g_IrdNumber = nvmem_cache.IrdNumber;
		memcpy(EMM_crc_list, nvmem_cache.EMM_crc_list, sizeof(EMM_crc_list));
		EMM_Index = nvmem_cache.EMM_Index;
		ClearNVMEMRAM();
	}
	else {
		ClearNVMEMRAM();
		Callback_OSDMessage(0, "Memory problem, please contact your operator.");
		return FALSE;
	}

	memcpy(head, "\xa0\x02\x00\x00\x01", 5);
	sw = Callback_ISO7816_TransmitCommand(head, (unsigned char *) "\x02");
	switch( (sw & 0xFF00) ) {
	case 0x9100:
		memcpy(head, "\xa0\x20\x00\x00", 4);
		head[4] = (unsigned char) (sw & 0x00FF);
		if (Callback_ISO7816_ReceiveCommand(head, buffer) == 0x9900) {
			errorcode = CAM_CARD_ERROR;
			return 0;
		}

		ptr = collectBytes(0x02, 0x04, head[4], buffer);
		if(ptr) {
			g_CardNumber = (uint32_t) (buffer[0+ptr] << 24) | (uint32_t) (buffer[1+ptr] << 16) | (uint32_t) (buffer[2+ptr] << 8) | buffer[3+ptr];
		}
		else {
			Callback_OSDMessage(0, "Card is invalid. Check your viewing card.");
			return FALSE;
		}
		break;
	case 0x9900:
		g_CardNumber = 0;
		break;
	default:
		Callback_OSDMessage(0, "Card is invalid. Check your viewing card.");
		return FALSE;
		break;
	}

	memcpy(head, "\xa0\x02\x00\x00\x01", 5);
	sw = Callback_ISO7816_TransmitCommand(head, (unsigned char *) "\x04");
	switch( (sw & 0xFF00) ) {
	case 0x9100:
		memcpy(head, "\xa0\x20\x00\x00", 4);
		head[4] = (unsigned char) (sw & 0x00FF);
		if (Callback_ISO7816_ReceiveCommand(head, buffer) == 0x9900) {
			errorcode = CAM_CARD_ERROR;
			return 0;
		}

		ptr = collectBytes(0x04, 0x04, head[4], buffer);
		if(ptr) {
			g_GroupNumber = (uint32_t) (buffer[0+ptr] << 24) | (uint32_t) (buffer[1+ptr] << 16) | (uint32_t) (buffer[2+ptr] << 8) | buffer[3+ptr];
		}
		else {
			Callback_OSDMessage(0, "Card is invalid. Check your viewing card.");
			return FALSE;
		}
		break;
	case 0x9900:
		g_GroupNumber = 0;
		break;
	default:
		Callback_OSDMessage(0, "Card is invalid. Check your viewing card.");
		return FALSE;
		break;
	}

	memcpy(head, "\xa0\x02\x00\x00\x01", 5);
	sw = Callback_ISO7816_TransmitCommand(head, (unsigned char *) "\x06");
	switch( (sw & 0xFF00) ) {
	case 0x9100:
		memcpy(head, "\xa0\x20\x00\x00", 4);
		head[4] = (unsigned char) (sw & 0x00FF);
        if (Callback_ISO7816_ReceiveCommand(head, buffer) == 0x9900) {
			errorcode = CAM_CARD_ERROR;
			return 0;
		}

		ptr = collectBytes(0x06, 0x02, head[4], buffer);
		if(ptr) {
			garr_Version[0] = buffer[0+ptr];
			garr_Version[1] = buffer[1+ptr];
		}
		else {
			Callback_OSDMessage(0, "Card is invalid. Check your viewing card.");
			return FALSE;
		}
		break;
	case 0x9900:
		garr_Version[0] = 0;
		garr_Version[1] = 0;
		break;
	default:
		Callback_OSDMessage(0, "Card is invalid. Check your viewing card.");
		return FALSE;
		break;
	}

	memcpy(head, "\xa0\x02\x00\x00\x01", 5);
	sw = Callback_ISO7816_TransmitCommand(head, (unsigned char *) "\x08");
	switch( (sw & 0xFF00) ) {
	case 0x9100:
		memcpy(head, "\xa0\x20\x00\x00", 4);
		head[4] = (unsigned char) (sw & 0x00FF);
		if (Callback_ISO7816_ReceiveCommand(head, buffer) == 0x9900) {
			errorcode = CAM_CARD_ERROR;
			return 0;
		}

		ptr = collectBytes(0x08, 0x02, head[4], buffer);
		if(ptr) {
			g_Caid = (unsigned short) (buffer[0+ptr] << 8) | buffer[1+ptr];
		}
		else {
			Callback_OSDMessage(0, "Card is invalid. Check your viewing card.");
			return FALSE;
		}
		break;
	case 0x9900:
		g_Caid = 0;
		break;
	default:
		Callback_OSDMessage(0, "Card is invalid. Check your viewing card.");
		return FALSE;
		break;
	}
    SendConfigToCard();
	return TRUE;
}

// <summary>
// External function
// De Initilizes the CA system and frees allocated resources
// </summary>
void CADeInit(void) {
	destroy(g_EmmFingerprintList);
	destroy(g_EmmMessageAndForcetuneList);
	g_CardNumber = 0;
	g_GroupNumber = 0;
	garr_Version[0] = 0;
	garr_Version[1] = 0;
	g_Caid = 0;
}

// External Function
// Menu is is generating maximum 32chars/line of text to display.
// N00- = Line to display
// S01- = Menu selection to level 1,
// S02- = Menu selection to level 2 etc,

char menu[2048];

// <summary>
// External function
// Prints the CA-menu in the STB
// </summary>
// <param name="level">The menu sub-level, 0 = Main menu, 1 = Subscription information, 2 = Pay Per View information, 3 = About CA
// 				4 = Parental Control, 5 = Change Maturity level, 6 = Change card PIN code </param>
// <returns>Nothing</returns>
void GetMMI(unsigned char level) {
	unsigned short sw;
	unsigned char head[5], a;
	unsigned char buffer[256], ptr;
	char tmpbuffer[64];
	unsigned short dateto;

	switch(level) {
	case 0:
		// Only show PIN control menu if card version is atleast 1.18
		if ((garr_Version[0] >= 1) && (garr_Version[1] >=18))
		{
			strcpy(menu, "Cryptoguard CA\nSubscription\nPay-per-view\nAbout CA\nParental control\n");
		}
		else
		{
			strcpy(menu, "Cryptoguard CA\nSubscription\nPay-per-view\nAbout CA\n");
		}
		break;

	case 1:
		strcpy(menu, "Subscription\n");
		memcpy(head, "\xa0\x02\x00\x00\x01", 5);
		sw = Callback_ISO7816_TransmitCommand(head, (unsigned char *) "\x40");
		switch( (sw & 0xFF00) ) {
		case 0x9100:
			memcpy(head, "\xa0\x20\x00\x00", 4);
			head[4] = (unsigned char) (sw & 0x00FF);
			if (Callback_ISO7816_ReceiveCommand(head, buffer) == 0x9900) {
				errorcode = CAM_CARD_ERROR;
				menu[0] = 0;
				return;
			}

			ptr = collectBytes(0x40, 0x00, head[4], buffer);
			if(ptr) {

				a = 0;
				do {

					dateto = buffer[2+ptr] << 8 | buffer[3+ptr];
					sprintf(tmpbuffer, "%02d Expire date %04d-%02d-%02d %08X\n",a ,2000 + ((dateto & 0xFE00) >> 9), (dateto & 0x01E0) >> 5, dateto & 0x001f,
						buffer[4+ptr] << 24 | buffer[5+ptr] << 16 | buffer[6+ptr] << 8 | buffer[7+ptr]);

					strcat(menu, tmpbuffer);
					ptr += 8;

					dateto = buffer[2+ptr] << 8 | buffer[3+ptr];
					sprintf(tmpbuffer, "%02d Expire date %04d-%02d-%02d %08X\n",a ,2000 + ((dateto & 0xFE00) >> 9), (dateto & 0x01E0) >> 5, dateto & 0x001f,
						buffer[4+ptr] << 24 | buffer[5+ptr] << 16 | buffer[6+ptr] << 8 | buffer[7+ptr]);

					strcat(menu, tmpbuffer);
					ptr += 8;
					a++;
				}
				while((ptr < buffer[1]) && (a<33));
			}
			else {
				strcpy(menu, "No subscriptions.\n");
				break;
			}
			break;
		default:
			strcpy(menu, "No subscriptions.\n");
			break;
		}
		break;

	case 2:
		strcpy(menu, "Pay-per-view\n");

		memcpy(head, "\xa0\x02\x00\x00\x01", 5);
		sw = Callback_ISO7816_TransmitCommand(head, (unsigned char *) "\x41");
		switch( (sw & 0xFF00) ) {
		case 0x9100:
			memcpy(head, "\xa0\x20\x00\x00", 4);
			head[4] = (unsigned char) (sw & 0x00FF);
			if (Callback_ISO7816_ReceiveCommand(head, buffer) == 0x9900) {
				errorcode = CAM_CARD_ERROR;
				menu[0] = 0;
				return;
			}

			ptr = collectBytes(0x41, 0x00, head[4], buffer);
			if(ptr) {

				a = 0;
				do {
					if((buffer[4+ptr] << 24 | buffer[5+ptr] << 16 | buffer[6+ptr] << 8 | buffer[7+ptr]) != 0) {
						sprintf(tmpbuffer, "Subscribed to program number: %d\n",	(buffer[4+ptr] << 24 | buffer[5+ptr] << 16 | buffer[6+ptr] << 8 | buffer[7+ptr]) );
						strcat(menu, tmpbuffer);
						a = 1;
					}
					ptr += 8;
				} while((ptr < buffer[1]) && (a<33));

				if(!a) {
					strcpy(menu, "No subscriptions.\n");
				}
			}
			else {
				strcpy(menu, "No subscriptions.\n");
				break;
			}
			break;
		default:
			strcpy(menu, "No subscriptions.\n");
			break;
		}
		break;

	case 3:
		strcpy(menu, "About CA\n");
		sprintf(tmpbuffer, "Interface version %d.%02d\n", garr_Version[0], garr_Version[1]);
		strcat(menu, tmpbuffer);
		sprintf(tmpbuffer, "Card number %011u\n", g_CardNumber);
		strcat(menu, tmpbuffer);
		sprintf(tmpbuffer, "Box id %011u\n", g_IrdNumber);
		strcat(menu, tmpbuffer);
		sprintf(tmpbuffer, "CA System ID 0x%04X\n", g_Caid);
		strcat(menu, tmpbuffer);
		sprintf(tmpbuffer, "CA Version 2.14 %s\n", __TIMESTAMP__ );
		strcat(menu, tmpbuffer);
		break;

	case 4:
		strcpy(menu, "Parental Control\n");
		if (getMaturityLevel(&a))
		{
			sprintf(tmpbuffer, "Maturity level set to %d\n", a);
		}
		else
		{
			sprintf(tmpbuffer, "Unknown maturity level\n");
		}
		strcat(menu, tmpbuffer);

		sprintf(tmpbuffer, "Change Maturity level\n");
		strcat(menu, tmpbuffer);
		sprintf(tmpbuffer, "Change card PIN code\n");
		strcat(menu, tmpbuffer);
        sprintf(tmpbuffer, "Strict parental control is %s\n", gStrictParentalRating ? "ON" : "OFF" );
		strcat(menu, tmpbuffer);
		break;

	case 5:
		strcpy(menu, "Change Maturity level\n");
		// Here goes code to change maturity level
		break;

	case 6:
		strcpy(menu, "Change card PIN code\n");
		// Here goes code to change PIN code
		break;



	case 8:
		strcpy(menu, "Status\n");
		// pmt_caid_pid
		// cat_caid_pid
		// Here goes code to change PIN code
		break;

	default:
		break;
	}
}

// retur 1 if any character is not a number between 0-9
static unsigned char checkNumbers(char *pincode)
{
	if((pincode[0] < '0') || (pincode[0] > '9') ||
		(pincode[1] < '0') || (pincode[1] > '9') ||
		(pincode[2] < '0') || (pincode[2] > '9') ||
		(pincode[3] < '0') || (pincode[3] > '9'))
	{
        CG_API_PRINTF("Invalid character in pin");
		return 1;
	}
	return 0;
}

// <summary>
// External function
// Sends a PIN code to card to overide maturity rating
// </summary>
// <param name="pincode">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <returns>1 if success otherwise returns 0</returns>
unsigned char unlockUsingPIN(char *pincode)
{
	unsigned short sw;
	unsigned char head[5];

	if(checkNumbers(pincode))
	{
		CG_API_PRINTF("Invalid character in pin");
		return 0;
	}

	memcpy(head, "\xa0\x16\x00\x00\x04", 5);
	sw = Callback_ISO7816_TransmitCommand(head, (unsigned char *) pincode);
	switch( (sw & 0xFF00) )
	{
	case 0x9000:
		CG_API_PRINTF("Unlock using pin success with %04X\n", sw);
		// clear crc for ECM to speed up start of decryption
		last_ecm_crc[0] = 0;
		return 1;
		break;
	default:
		CG_API_PRINTF("Unlock using pin failed with %04X\n", sw);
		break;
	}
	return 0;
}

// <summary>
// External function
// Toggle strict parental rating, if disable, using pin in card for verification of disable
// </summary>
// <param name="pincode">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <returns>1 if success otherwise returns 0</returns>
unsigned char ToggleStrictParentalRatingUsingPIN(char *pincode)
{
	unsigned short sw;
	unsigned char head[5];

	if(checkNumbers(pincode)) {
		return 0;
	}

	memcpy(head, "\xa0\x16\x00\x05\x04", 5);
	sw = Callback_ISO7816_TransmitCommand(head, (unsigned char *) pincode);
	if ((sw & 0xFF00) != 0x9000 ) {
		CG_API_PRINTF("Verify pin failed with %04X\n", sw);
		errorcode = CAM_PIN_ERROR;
		return 0;
	}
	gStrictParentalRating = !gStrictParentalRating;
	return 1;
}

// <summary>
// External function
// Sends a PIN code to card for verification
// </summary>
// <param name="pincode">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <returns>1 if success otherwise returns 0</returns>
unsigned char VerifyPIN(char *pincode)
{
	unsigned short sw;
	unsigned char head[5];

	if(checkNumbers(pincode))
	{
		return 0;
	}

	memcpy(head, "\xa0\x16\x00\x05\x04", 5);
	sw = Callback_ISO7816_TransmitCommand(head, (unsigned char *) pincode);
	switch( (sw & 0xFF00) )
	{
	case 0x9000:
		CG_API_PRINTF("Verify pin success with %04X\n", sw);
		return 1;
		break;
	default:
		CG_API_PRINTF("Verify pin failed with %04X\n", sw);
		break;
	}
	return 0;
}

// <summary>
// External function
// set card back to maturity rating set inside card
// </summary>
// <returns>1 if success otherwise returns 0</returns>
unsigned char lockWithoutPIN(void)
{
	unsigned short sw;
	unsigned char head[5];

	memcpy(head, "\xa0\x16\x00\x04\x00", 5);
	sw = Callback_ISO7816_TransmitCommand(head, (unsigned char *) "\x00");
	switch( (sw & 0xFF00) )
	{
	case 0x9000:
		CG_API_PRINTF("Card restored to default maturity rating with %04X\n", sw);
		return 1;
		break;
	default:
		CG_API_PRINTF("command failed with %04X\n", sw);
		break;
	}
	return 0;
}

// <summary>
// External function
// Change PIN code in card
// </summary>
// <param name="oldpin">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <param name="newpin">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <returns>1 if success otherwise returns 0</returns>
unsigned char changePIN(char *oldpin, char *newpin)
{
	unsigned short sw;
	unsigned char head[5], buffer[8];
	if ((checkNumbers(oldpin)) || (checkNumbers(newpin)))
	{
		CG_API_PRINTF("Invalid character in pin");
		return 0;
	}
	memcpy(buffer, oldpin, 4);
	memcpy(buffer+4, newpin, 4);

	memcpy(head, "\xa0\x16\x00\x01\x08", 5);
	sw = Callback_ISO7816_TransmitCommand(head, buffer);
	switch( (sw & 0xFF00) )
	{
	case 0x9000:
		CG_API_PRINTF("Change pin success with %04X\n", sw);
		return 1;
		break;
	default:
		CG_API_PRINTF("Change pin failed with %04X\n", sw);
		break;
	}
	return 0;
}

// <summary>
// External function
// Get current maturity level (age) set in card
// </summary>
// <returns>0 if fail, 1 on success and sets level to maturity level (age)</returns>
unsigned char getMaturityLevel(unsigned char *level)
{
	unsigned short sw;
	unsigned char head[5], buffer[256], ptr;

	memcpy(head, "\xa0\x02\x00\x00\x01", 5);
	sw = Callback_ISO7816_TransmitCommand(head, (unsigned char *) "\x0C");
	if ((sw & 0xFF00) == 0x9100)
	{
		memcpy(head, "\xa0\x20\x00\x00", 4);
		head[4] = (unsigned char) (sw & 0x00FF);
		if (Callback_ISO7816_ReceiveCommand(head, buffer) == 0x9000)
		{
			ptr = collectBytes(0x0C, 0x01, head[4], buffer);
			if(ptr)
			{
				//CG_API_PRINTF("Maturity Level set to: %d\n", buffer[ptr]);
				*level = buffer[ptr];
				return 1;
			}
		}
		CG_API_PRINTF("Received data %04X\n", sw);
	}
	return 0;
}

// <summary>
// External function
// Change Maturity rating in card
// </summary>
// <param name="pincode">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <param name="age"> New age maturity level (age) set in card
// <returns>1 if success otherwise returns 0</returns>
unsigned char changeMaturityLevel(char *pincode, unsigned char age)
{
	unsigned short sw;
	unsigned char head[5], buffer[5];

	memcpy(buffer, pincode, 4);
	buffer[4] = age;
	memcpy(head, "\xa0\x16\x00\x02\x05", 5);
	sw = Callback_ISO7816_TransmitCommand(head, buffer);
	if ((sw & 0xFF00) == 0x9000)
	{
		CG_API_PRINTF("Maturity Level set to: %d\n", age);
		return 1;
	}
	CG_API_PRINTF("Faled to set maturity Level set to: %d\n", age);
	return 0;
}
