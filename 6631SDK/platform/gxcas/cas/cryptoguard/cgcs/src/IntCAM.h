#ifndef INTCAM_H_INCLUDED
#define INTCAM_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif
#include <time.h>
#include <stdint.h>
// if testing on windows "remove" __attribute__(x)
#ifdef WIN32
#define __attribute__(x)
#endif
// Define demo if you want to testrun IntCAM on a linux machine connection to
// simulcast stream and PCSCD smarcard driver
//


// Internal functions
void ClearNVMEMRAM(void);
void ReadNVMEMRAM(void);
void WriteNVMEMRAM(void);

typedef struct {
  time_t seconds;
  time_t time_set;
} emm_time_t;

#define DIV_7 (RAND_MAX / 7)
#define DIV_16 (RAND_MAX / 16)
#define EMM_list_len 64

typedef struct {
	uint32_t NvMemCRC;
	uint32_t IrdNumber;
	unsigned char STB_SystemGlobalKey[16];
	unsigned char STB_ManufacturerGlobalKey[16];
	unsigned char STB_GroupKey[16];
	unsigned char STB_UniqueKey[16];
	unsigned char STB_CardlessGroupKey[16];
	unsigned char STB_CardlessUniqueKey[16];
	unsigned char STB_PairingKey[16];
	uint32_t STB_PairingCardnumber;
	unsigned char EMM_Index;
	uint32_t EMM_crc_list[EMM_list_len];
	unsigned char StrictParentalControl;
	unsigned char dummy[7];
}
nvmem_storage;

extern nvmem_storage nvmem_cache;

#define CAM_SUCCESS                 0x0000
#define CAM_CRC_FAILURE             0x0001
#define CAM_TABLE_FAILURE           0x0002
#define CAM_CAID_NOTFOUND           0x0003
#define CAM_INIT_ERROR              0x0004
#define CAM_NOT_ADRESSED            0x0005
#define CAM_ECM_ALREADY_PROCESSED   0x0006
#define CAM_NOACCESS                0x0007
#define CAM_CARD_ERROR              0x0008
#define CAM_EMM_OTA_ERROR           0x0009
#define CAM_PIN_ERROR               0x000A

#define EMM_FP31    0x31
#define EMM_FT32    0x32
#define EMM_MS33    0x33
#define EMM_TM34    0x34
#define EMM_SC35    0x35
#define EMM_REMOVE  0xFF

// ECM fingerprint version 0 color assignment
extern const unsigned int fp0color[];

// ECM and EMM fingerprint version 1 color assignment
extern const unsigned int fp1color[];
extern unsigned short g_Caid;
extern unsigned char garr_CW1[2][8], garr_CW2[2][8];
extern char menu[];

extern uint32_t g_CardNumber;
extern uint32_t g_GroupNumber;
extern uint32_t g_IrdNumber;

typedef struct {
	unsigned short DestinationSID;
	unsigned char  tagvalue;			// 0x5A = DVB-T,  0x44 = DVB-C,  0x43 = DVB-S
	unsigned char  len;
	unsigned char  descriptordata[11];	// specified in Digital Video Broadcasting (DVB) Specification for Service Information (SI) in DVB systems (en_300468v011101p.pdf)
										// depending on type "Satellite delivery system descriptor", "Cable delivery system descriptor" or "Terrestrial delivery system descriptor"
}
__attribute__((packed)) delivery_descriptor;

typedef struct {
char        ForcedUpgrade;  // Forced upgrade
uint32_t    Manufacturer;   // Manufacturer ID used for OTA trigger, contact Cryptoguard to receive ID if needed
uint32_t    Model;          // Manufacturer model identifier
uint32_t    Version;        // Sw version in OTA stream
uint32_t    PID;            // Data PID
uint32_t    Delivery;       // 1 = Cable delivery, 2 = Terrestial delivery, 3 = Satellite
uint32_t    Frequency;      // frequency
uint32_t    Symbolrate;     // Symbolrate 0x1ADB => 6875
uint16_t    Fec;            // fec (satellite)
uint16_t    Polarity;       // polarity (satellite)
uint16_t    Modulation;     // modulation 2=qam64 3=qam128 4=qam256
uint16_t    Bandwith;       // bandwith (terrestial), 8 = 8mhz and 7 = 7mhz
}
ota_trigger;

typedef struct {
	unsigned char  	DisplayFingerprint;	// 0: Fingerprint disabled 1: Fingerprint Active
	int				TimeForNext;		// UTC Timestamp to display next
}
ecm_fingerprint_events;

typedef struct {
	char  		   	DisplayText[24];	// 11 cardnumber 1 space 11 Irdnumber 1 NULL
	unsigned short 	DurationMs;			// 0-65535: Duration in ms for display of fingerprint.
	unsigned char  	X;					// 0-15: Placement X cordinate
	unsigned char  	Y;					// 0-15: Placement Y cordinate
	int			   	FontColor;			// Font color
	int 		   	BackgroundColor;	// Background color
}
fingerprint_descriptor;

typedef struct {
	// CGMSA,HDCP,Macrovision,AnaHD,DigHD
	unsigned char CGMSA;
	// 0: Neutral operation.
	// 1: Off, forced deactivation.
	// 2: On, supported units.
	// 3: On, mandatory on all units.

	unsigned char HDCP;
	// 0: Neutral operation.
	// 1: Off, forced deactivation.
	// 2: On, supported units.
	// 3: On, mandatory on all units.

	unsigned char Macrovision;
	// 0: Neutral operation.
	// 1: Off, forced deactivation.
	// 2: On, supported units.
	// 3: On, mandatory on all units.

	unsigned char AnaHD;
	// 0: Neutral operation.
	// 1: Max analogue resultion is 576i.

	unsigned char DigHD;
	// 0: Neutral operation.
	// 1: Max digital resultion is 1080p.
	// 2: Max digital resultion is 1080i.
	// 3: Max digital resultion is 720p.
	// 4: Max digital resultion is 720i.
	// 5: Max digital resultion is 576i.

} ecm_STB_videorules;

typedef struct {
	unsigned char	Forced;
	unsigned short	Duration;
	unsigned char	Text[2048];
}
__attribute__((packed)) message_descriptor;

typedef struct {
    unsigned short 	Duration;   		// 0-65535: Duration in seconds for display of scrolling message.
	unsigned char  	Y;					// 0-15: Placement Y cordinate
	int			   	FontColor;			// Font color
	int 		   	BackgroundColor;	// Background color
	unsigned short  ScrollSpeed;        // scrollspeed in ms
	unsigned char	Forced;
	unsigned char   FontSize;
	unsigned char	Text[256];
}
__attribute__((packed)) scroll_descriptor;


// <summary>
// External function
// Initilizes the CA system and check status of card
// </summary>
// <returns>Returns true if OK and false if not. When success sets the global variable g_IrdNumber</returns>
unsigned char CAInit(void);

/*
/ <summary>
/ External function
/ De Initilizes the CA system and frees allocated resources
/ </summary>
*/
void CADeInit(void);

/*
/ IntCAM API calls
/ <summary>
/ External function
/ Returns the EMM PID from the CAT section
/ </summary>
/ <param name="cat_section">The CAT section to search EMM PID</param>
/ <returns>Returns the EMM PID if success otherwise returns 0 and sets the global variable errorcode to 1 = CRC failure, 2 = Table failure, 3 = CA-ID not found, 4 = Initilization error</returns>
*/
unsigned short getEMMPid(unsigned char *cat_section);

/*
/ <summary>
/ External function
/ Returns the ECM PID from the PMT section
/ </summary>
/ <param name="pmt_section">The PMT section to search ECM PID</param>
/ <param name="CH">current stream 0 or 1</param>
/ <returns>Returns the ECM PID if success otherwise returns 0 and sets the global variable errorcode to 1 = CRC failure, 2 = Table failure, 3 = CA-ID not found, 4 = Initilization error</returns>
*/
unsigned short getECMPid(unsigned char CH, unsigned char *pmt_section);

/*
/ <summary>
/ External function
/ Main function for processing EMM
/ </summary>
/ <param name="CH">current stream 0 or 1</param>
/ <param name="emm_section">Pointer to the EMM section in the stream</param>
/ <returns>Returns 1 if success otherwise returns 0 and sets the global variable errorcode to 5 = CAM not addressed</returns>
*/
unsigned short ProcessEMM(unsigned char *emm_section, time_t unixtime);

/*
/ <summary>
/ External function
/ Main function for processing ECM
/ </summary>
/ <param name="CH">current stream 0 or 1</param>
/ <param name="ecm_section">Pointer to the ECM section in the stream</param>
/ <returns>Returns 1 if success otherwise returns 0 and sets the global variable errorcode to 1 = CRC failure, 2 = Table failure, 5 = CAM not addressed, 6 = ECM already processed, 7 = CAM no success</returns>
*/
unsigned short ProcessECM(unsigned char CH, unsigned char *ecm_section);

/*
/ <summary>
/ API function
/ Main function for maintain timing of event
/ </summary>
/ <param name="unixtime">current time</param>
/ <returns>nothing</returns>
*/
void TimerTick(time_t unixtime);

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
uint32_t InitNVMEMRAM(nvmem_storage initvalues, unsigned char ForceReprogram);

// <summary>
// External function
// Prints the CA-menu in the STB
// </summary>
// <param name="level">The menu sub-level, 0 = Main menu, 1 = Subscription information, 2 = Pay Per View information, 3 = About CA</param>
// <returns>Nothing</returns>
void GetMMI(unsigned char level);

// <summary>
// External function
// Sends a PIN code to card for verification, and to overide cards maturity rating
// </summary>
// <param name="pincode">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <returns>1 if success otherwise returns 0</returns>
unsigned char unlockUsingPIN(char *pincode);

// <summary>
// External function
// set card back to maturity rating set inside card
// </summary>
// <returns>1 if success otherwise returns 0</returns>
unsigned char lockWithoutPIN(void);

// <summary>
// External function
// Change PIN code in card
// </summary>
// <param name="oldpin">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <param name="newpin">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <returns>1 if success otherwise returns 0</returns>
unsigned char changePIN(char *oldpin, char *newpin);

// <summary>
// External function
// Get current maturity level (age) set in card
// </summary>
// <returns>0 if fail, 1 on success and sets level to maturity level (age)</returns>
unsigned char getMaturityLevel(unsigned char *level);

// <summary>
// External function
// Change Maturity rating in card
// </summary>
// <param name="pincode">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <param name="age"> New age maturity level (age) set in card
// <returns>1 if success otherwise returns 0</returns>
unsigned char changeMaturityLevel(char *pincode, unsigned char age);

// <summary>
// External function
// Toggle strict parental rating, if disable, using pin in card for verification of disable
// </summary>
// <param name="pincode">4 char text string in ascii 0=0x30 1=0x31 2=0x32  ........
// <returns>1 if success otherwise returns 0</returns>
unsigned char ToggleStrictParentalRatingUsingPIN(char *pincode);

// For card INIT message
#define IRDNUMBER					0x01 	// 8 Byte IRD Number

#define PAIREDWITH 					0x02 	// 8 byte what CARD number STB is paired with right now

#define PAIRING_MODE				0x03 	// 1 Byte What pairing capabilitys STB has
// Sub to pairing mode
#define NO_PAIRING						0x00
#define SOFT_PAIRING					0x01
#define HARD_PAIRING					0x02
#define LOCKED_PAIRING					0x03 // Always pairing


#ifdef __cplusplus
}
#endif
#endif // INTCAM_H_INCLUDED

