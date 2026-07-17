#include "IntCAM.h"
#include <stdio.h>
#include <assert.h>
#include "gxcas_dbg.h"
#include "gxcas_flash.h"
#include "gxcas_smartcard.h"
#include "cryptoguard_api.h"
#include "gxcas_queue.h"

//#define _DEMO
#define _DEBUG_INFO
// Declaration to use in demo mode
#ifdef _DEMO
extern void demo_WriteNVMEM(nvmem_storage *nv);
extern void demo_ReadNVMEM(nvmem_storage *nv);
extern BOOL SMC_ReceiveCommand(LPBYTE pHeader, LPBYTE pData, LPBYTE pSW);
extern BOOL SMC_SendCommand(LPBYTE pHeader, LPBYTE pData, LPBYTE pSW);
#endif

/*
* <summary>
* Callback function
* Display Fingerprint on Screen
* </summary>
* <param name="CH">Stream to be displayed on 0 or 1</param>
* <param name="Fingerprint->DisplayText">The fingerprint string that will be displayed</param>
* <param name="Fingerprint->Duration">The duration the fingerprint will be displayed in milliseconds</param>
* <param name="Fingerprint->X">The X position to display the fingerprint</param>
* <param name="Fingerprint->Y">The Y position to display the fingerprint</param>
* <param name="Fingerprint->FontColor">The fontcolor to use</param>
* <param name="Fingerprint->BackgroundColor">The background color to use</param>
* <returns>nothing</returns>
*/
void Callback_STB_DisplayFingerPrint(unsigned char CH, fingerprint_descriptor *Fingerprint) {
    GxCas_CGFingerInfo  param;

#ifdef _DEBUG_INFO
    CAS_DBG(CAS, "-----------------------------\r\n");
    CAS_DBG(CAS, "%s      %d\r\n", Fingerprint->DisplayText, CH);
    CAS_DBG(CAS, "X:%02d Y:%02d Duration:%07d ms\r\n", Fingerprint->X, Fingerprint->Y, Fingerprint->DurationMs);
    CAS_DBG(CAS, "Font:%08X  Back::%08X \r\n", Fingerprint->FontColor, Fingerprint->BackgroundColor);
    CAS_DBG(CAS, "-----------------------------\r\n");
#endif

    memset(&(param.finger_info), 0, sizeof(fingerprint_descriptor_t));
    memcpy(&(param.finger_info.DisplayText), Fingerprint->DisplayText, sizeof(Fingerprint->DisplayText));
    param.finger_info.DurationMs = Fingerprint->DurationMs;
    param.finger_info.X = Fingerprint->X;
    param.finger_info.Y = Fingerprint->Y;
    param.finger_info.FontColor= Fingerprint->FontColor;
    param.finger_info.BackgroundColor = Fingerprint->BackgroundColor;
    param.finger_info.CH = CH;

    assert(GxCas_QueuePut(GXCAS_CRYPTOGUARD_FINGER_INFO, (void *)&param, sizeof(GxCas_CGFingerInfo), 0) != -1);
}

/*
* <summary>
* Callback function
* Preform a forcetuning to another SID
* </summary>
* <param name="forcetune->DestinationSID">The fingerprint string that will be displayed</param>
* <param name="forcetune->tagvalue">Type of descriptor 0x5A = DVB-T,  0x44 = DVB-C,  0x43 = DVB-S</param>
* <param name="forcetune->len">length of descriptor</param>
* <param name="forcetune->descriptordata">specified in Digital Video Broadcasting (DVB) Specification for Service Information (SI)</param>
* <returns>nothing</returns>
*/
void Callback_STB_DoForcetune(delivery_descriptor *forcetune) {
#ifdef _DEBUG_INFO
    unsigned char j;
    CAS_DBG(CAS, "--------------------------------------\r\n");
    CAS_DBG(CAS, "Do forcetune now\r\n");
    CAS_DBG(CAS, "Data: ");
    for(j = 0; j < 11; j ++) {
        CAS_DBG(CAS, "%02X ", forcetune->descriptordata[j]);
    }
    CAS_DBG(CAS, "\r\n");
    CAS_DBG(CAS, "--------------------------------------\r\n");
#endif
	GxCas_CGDelivery_descriptor  param = {0};
	memcpy(&param,forcetune,sizeof(GxCas_CGDelivery_descriptor));
	assert(GxCas_QueuePut(GXCAS_CRYPTOGUARD_FORCETUNE, (void *)&param, sizeof(GxCas_CGDelivery_descriptor), 0) != -1);
}

/*
* <summary>
* Callback function
* Display EMM message on screen
* </summary>
* <param name="message->Duration">duration to display message on screen</param>
* <param name="message->Forced">0 Message can be canceled with OK or Exit keys, 1 Message has to be shown full duration</param>
* <param name="message->Text">Text string to be displayed on screen, including linefeed and CR for multiple line support</param>
* <returns>nothing</returns>
*/

typedef enum
{
 ENCODE_NONE = 0,
 ENCODE_ISO_8859_1,
 ENCODE_ISO_8859_2,
 ENCODE_ISO_8859_3,
 ENCODE_ISO_8859_4,
 ENCODE_ISO_8859_5,
 ENCODE_ISO_8859_6,
 ENCODE_ISO_8859_7,
 ENCODE_ISO_8859_8,
 ENCODE_ISO_8859_9,
 ENCODE_ISO_8859_10,
 ENCODE_ISO_8859_11,
 ENCODE_ISO_8859_12,
 ENCODE_ISO_8859_13,
 ENCODE_ISO_8859_14,
 ENCODE_ISO_8859_15,
 ENCODE_ISO_8859_16,
 ENCODE_UCS_2_INTERNAL, // unicode
 ENCODE_UCS_4_INTERNAL,
 ENCODE_DEFINED_0X11,
 ENCODE_WINDOWS_1252,
 ENCODE_UTF_8,
 ENCODE_UTF_16,
 ENCODE_NUMBER,
} ENCODE_TYPE;


void Callback_STB_DisplayMessage(message_descriptor *message) {
#ifdef _DEBUG_INFO

    ENCODE_TYPE encoding = ENCODE_NONE;
    int offset = 0;

    if (message->Text[0] < 0x20) {
        offset = 1;
        switch (message->Text[0]) {
        case 0x01:      encoding = ENCODE_ISO_8859_5;       break;
        case 0x02:      encoding = ENCODE_ISO_8859_6;       break;
        case 0x03:      encoding = ENCODE_ISO_8859_7;       break;
        case 0x04:      encoding = ENCODE_ISO_8859_8;       break;
        case 0x05:      encoding = ENCODE_ISO_8859_9;       break;
        case 0x09:      encoding = ENCODE_ISO_8859_13;      break;
        case 0x10:
            offset = 3;
            switch ((message->Text[1]) << 8 | message->Text[2]) {
            case 0x0001:encoding = ENCODE_ISO_8859_1; break;
            case 0x0002:encoding = ENCODE_ISO_8859_2; break;
            case 0x0003:encoding = ENCODE_ISO_8859_3; break;
            case 0x0004:encoding = ENCODE_ISO_8859_4; break;
            case 0x0005:encoding = ENCODE_ISO_8859_5; break;
            case 0x0006:encoding = ENCODE_ISO_8859_6; break;
            case 0x0007:encoding = ENCODE_ISO_8859_7; break;
            case 0x0008:encoding = ENCODE_ISO_8859_8; break;
            case 0x0009:encoding = ENCODE_ISO_8859_9; break;
            case 0x000A:encoding = ENCODE_ISO_8859_10; break;
            case 0x000B:encoding = ENCODE_ISO_8859_11; break;
            case 0x000C:encoding = ENCODE_ISO_8859_12; break;
            case 0x000D:encoding = ENCODE_ISO_8859_13; break;
            case 0x000E:encoding = ENCODE_ISO_8859_14; break;
            case 0x000F:encoding = ENCODE_ISO_8859_15; break;
            // This should not happen
            default:    encoding = ENCODE_NONE; break;} break;

        // This is GB2312 Simplified Chinese missing for some
        case 0x13:  encoding = ENCODE_NONE; break;

        case 0x15:  encoding = ENCODE_UTF_8; break;

        // All below not used by cryptoguard
        case 0x06:  case 0x07:  case 0x08:  case 0x0A:  case 0x0B:
        case 0x0C:  case 0x0D:  case 0x0E:  case 0x0F:  break;
        case 0x11:  case 0x12:  case 0x14:
        default:
            break;
        }
    }

    CAS_DBG(CAS, "----------------------------------------\r\n");
    CAS_DBG(CAS, "%s message to be displayed\r\n", (message->Forced) ? "Forced" : "Normal" );
    CAS_DBG(CAS, "Display duration(sec):%d\r\n", message->Duration);
    CAS_DBG(CAS, "Text: %s\r\n", message->Text+offset);
    CAS_DBG(CAS, "Length: %d\r\n", strlen((const char *)(message->Text)+offset));
    CAS_DBG(CAS, "Number for encoding: %d\r\n", encoding);
    CAS_DBG(CAS, "----------------------------------------\r\n");
#endif

    GxCas_CGShowLongMessage  param = {0};

    if ((message->Text[0]) == 0x10)
    {
        param.encoding = encoding;
        param.long_msg.Forced = message->Forced;
        param.long_msg.Duration = message->Duration;
        memset(param.long_msg.Text, '\0', 2048);
        strcpy((char *)param.long_msg.Text, (const char *)(message->Text + offset));
    }
    else
    {
        param.encoding = encoding;
        param.long_msg.Forced = message->Forced;
        param.long_msg.Duration = message->Duration;
        memset(param.long_msg.Text, '\0', 2048);
        strcpy((char *)param.long_msg.Text, (const char *)message->Text);
    }

    assert(GxCas_QueuePut(GXCAS_CRYPTOGUARD_LONG_MESSAGE, (void *)&param, sizeof(GxCas_CGShowLongMessage), 0) != -1);
}

/*
* <summary>
* Callback function
* Display Scrolling message on screen
* </summary>
* <param name="scroll->Duration">duration to display message on screen</param>
* <param name="scroll->Forced">0 Message can be canceled with OK or Exit keys, 1 Message has to be shown full duration</param>
* <param name="scroll->Text">Text string to be displayed on screen, including character encoding</param>
* <param name="scroll->Y">Y Position 0-15 on screen</param>
* <param name="scroll->ScrollSpeed">speed to shift message text in mS</param>
* <param name="scroll->FontColor">The fontcolor to use</param>
* <param name="scroll->BackgroundColor">The background color to use</param>
* <returns>nothing</returns>
*/
void Callback_STB_DisplayScrollMessage(scroll_descriptor *scroll) {
#ifdef _DEBUG_INFO

    ENCODE_TYPE encoding = ENCODE_NONE;
    int offset = 0;


    if (scroll->Text[0] < 0x20) {
        offset = 1;
        switch (scroll->Text[0]) {
        case 0x01:      encoding = ENCODE_ISO_8859_5;       break;
        case 0x02:      encoding = ENCODE_ISO_8859_6;       break;
        case 0x03:      encoding = ENCODE_ISO_8859_7;       break;
        case 0x04:      encoding = ENCODE_ISO_8859_8;       break;
        case 0x05:      encoding = ENCODE_ISO_8859_9;       break;
        case 0x09:      encoding = ENCODE_ISO_8859_13;      break;
        case 0x10:
            offset = 3;
            switch ((scroll->Text[1]) << 8 | scroll->Text[2])   {
            case 0x0001:encoding = ENCODE_ISO_8859_1; break;
            case 0x0002:encoding = ENCODE_ISO_8859_2; break;
            case 0x0003:encoding = ENCODE_ISO_8859_3; break;
            case 0x0004:encoding = ENCODE_ISO_8859_4; break;
            case 0x0005:encoding = ENCODE_ISO_8859_5; break;
            case 0x0006:encoding = ENCODE_ISO_8859_6; break;
            case 0x0007:encoding = ENCODE_ISO_8859_7; break;
            case 0x0008:encoding = ENCODE_ISO_8859_8; break;
            case 0x0009:encoding = ENCODE_ISO_8859_9; break;
            case 0x000A:encoding = ENCODE_ISO_8859_10; break;
            case 0x000B:encoding = ENCODE_ISO_8859_11; break;
            case 0x000C:encoding = ENCODE_ISO_8859_12; break;
            case 0x000D:encoding = ENCODE_ISO_8859_13; break;
            case 0x000E:encoding = ENCODE_ISO_8859_14; break;
            case 0x000F:encoding = ENCODE_ISO_8859_15; break;
            // This should not happen
            default:    encoding = ENCODE_NONE; break;} break;

        // This is GB2312 Simplified Chinese missing for some
        case 0x13:  encoding = ENCODE_NONE; break;

        case 0x15:  encoding = ENCODE_UTF_8; break;

        // All below not used by cryptoguard
        case 0x06:  case 0x07:  case 0x08:  case 0x0A:  case 0x0B:
        case 0x0C:  case 0x0D:  case 0x0E:  case 0x0F:  break;
        case 0x11:  case 0x12:  case 0x14:
        default:
            break;
        }
    }

    CAS_DBG(CAS, "\x1B[31m");

    CAS_DBG(CAS, "----------------------------------------\r\n");
    CAS_DBG(CAS, "%s message to be displayed\r\n", (scroll->Forced) ? "Forced" : "Normal" );
    CAS_DBG(CAS, "Display duration(sec):%d\r\n", scroll->Duration);
    CAS_DBG(CAS, "Scroll speed(ms):%d\r\n", scroll->ScrollSpeed);
    CAS_DBG(CAS, "Text: %s\r\n", scroll->Text+offset);
    CAS_DBG(CAS, "Number for encoding: %d\r\n", encoding);
    CAS_DBG(CAS, "Y position %02d\r\n", scroll->Y);
    CAS_DBG(CAS, "Font color %08X\r\n", scroll->FontColor);
    CAS_DBG(CAS, "Background color %08X\r\n", scroll->BackgroundColor);
    CAS_DBG(CAS, "----------------------------------------\r\n");

    CAS_DBG(CAS, "\x1B[0m");
#endif

    GxCas_CGScrollMessage  param = {0};

    if (scroll->Text[0] == 0x10)
    {
        param.encoding = encoding;
        param.scroll_msg.Duration = scroll->Duration;
        param.scroll_msg.Y = scroll->Y;
        param.scroll_msg.FontColor = scroll->FontColor;
        param.scroll_msg.BackgroundColor = scroll->BackgroundColor;
        param.scroll_msg.ScrollSpeed = scroll->ScrollSpeed;
        param.scroll_msg.Forced = scroll->Forced;
        param.scroll_msg.FontSize = scroll->FontSize;
        memset(param.scroll_msg.Text, '\0', 256);
        strcpy(param.scroll_msg.Text, (const char *)(scroll->Text + offset));
    }
    else
    {
        param.encoding = ENCODE_NONE;
        param.scroll_msg.Duration = scroll->Duration;
        param.scroll_msg.Y = scroll->Y;
        param.scroll_msg.FontColor = scroll->FontColor;
        param.scroll_msg.BackgroundColor = scroll->BackgroundColor;
        param.scroll_msg.ScrollSpeed = scroll->ScrollSpeed;
        param.scroll_msg.Forced = scroll->Forced;
        param.scroll_msg.FontSize = scroll->FontSize;
        memset(param.scroll_msg.Text, '\0', 256);
		if(scroll->Text[0] == 0x13) //GB2312 show broken
		{
			strcpy(param.scroll_msg.Text,"broken");
		}
		else
		{
		 	strcpy(param.scroll_msg.Text, (const char *)(scroll->Text + offset));
		}
    }
    
    assert(GxCas_QueuePut(GXCAS_CRYPTOGUARD_SCROLL_MESSAGE, (void *)&param, sizeof(GxCas_CGScrollMessage), 0) != -1);
}


/*
* <summary>
* Callback function
* Videorules has changed
* </summary>
* <param name="CH">Stream to be affected 0 or 1</param>
* <param name="videorules->CGMSA">Copy Generation Management System - Analog</param>
* <param name="videorules->HDCP">High-bandwidth Digital Content Protection</param>
* <param name="videorules->Macrovision">Macrovision</param>
* <param name="videorules->AnaHD">AnaHD</param>
* <param name="videorules->DigHD">DigHD</param>
* <returns>nothing</returns>
*/
void Callback_STB_VideoRulesChanged(unsigned char CH, ecm_STB_videorules *videorules){
#ifdef _DEBUG_INFO
    CAS_DBG(CAS, "----------------------------\r\n");
    CAS_DBG(CAS, "Video rules has changed CH:%d\r\n", CH);
    CAS_DBG(CAS, "CGMSA:           %d \r\n", videorules->CGMSA);
    CAS_DBG(CAS, "HDCP:            %d \r\n", videorules->HDCP);
    CAS_DBG(CAS, "Macrovision:     %d \r\n", videorules->Macrovision);
    CAS_DBG(CAS, "AnaHD:           %d \r\n", videorules->AnaHD);
    CAS_DBG(CAS, "DigHD            %d \r\n", videorules->DigHD);
    CAS_DBG(CAS, "----------------------------\r\n");
#endif

    GxCas_CGVideoRules  param;
    memset(&(param.video_rules),0,sizeof(ecm_stb_videorules_t));
    memcpy(&(param.video_rules), videorules, sizeof(ecm_stb_videorules_t));
    assert(GxCas_QueuePut(GXCAS_CRYPTOGUARD_VIDEO_RULES, (void *)&param, sizeof(GxCas_CGVideoRules), 0) != -1);
}

/*
* <summary>
* Callback function
* OTA Trigger received
* </summary>
* <param name="Ota">Ota trigger data</param>
* <returns>nothing</returns>
*/
void Callback_STB_OtaTriggerReceived(ota_trigger *Ota){
#ifdef _DEBUG_INFO
    CAS_DBG(CAS, "--------------------------\r\n");
    CAS_DBG(CAS, "Ota trigger has received \r\n");
    CAS_DBG(CAS, "Manufacturer : %d 0x%08X\r\n", Ota->Manufacturer, Ota->Manufacturer);
    CAS_DBG(CAS, "Model        : %d\r\n", Ota->Model);
    CAS_DBG(CAS, "Version      : %X\r\n", Ota->Version);
    CAS_DBG(CAS, "PID          : %d\r\n", Ota->PID);
    CAS_DBG(CAS, "Delivery     : (%d) ", Ota->Delivery);

    switch (Ota->Delivery) { // 1 = Cable delivery, 2 = Terrestial delivery, 3 = Satellite
    case 1:
        CAS_DBG(CAS, "Cable delivery\r\n");
        break;
    case 2:
        CAS_DBG(CAS, "Terrestial delivery\r\n");
        break;
    case 3:
        CAS_DBG(CAS, "Satellite delivery\r\n");
        break;
    default:
        CAS_DBG(CAS, "Undefined delivery\r\n");
        break;
    }

    CAS_DBG(CAS, "Frequency    : %d MHz\r\n", Ota->Frequency);
    CAS_DBG(CAS, "Symbolrate   : %d\r\n", Ota->Symbolrate);
    CAS_DBG(CAS, "Fec          : %d\r\n", Ota->Fec);
    CAS_DBG(CAS, "Polarity     : %d\r\n", Ota->Polarity);
    CAS_DBG(CAS, "Modulation   : (%d) ", Ota->Modulation);

    switch (Ota->Delivery) { // 1 = Cable delivery, 2 = Terrestial delivery, 3 = Satellite
    case 2:
        CAS_DBG(CAS, "64 QAM\r\n");
        break;
    case 3:
        CAS_DBG(CAS, "128 QAM\r\n");
        break;
    case 4:
        CAS_DBG(CAS, "256 QAM\r\n");
        break;
    default:
        CAS_DBG(CAS, "16 QAM\r\n");
        break;
    }

    CAS_DBG(CAS, "Bandwith     : %d MHz\r\n", Ota->Bandwith);
    CAS_DBG(CAS, "--------------------------\r\n");
#endif

    GxCas_CGOtaTrigger  param;
    memset(&(param.ota_trigger),0,sizeof(ota_trigger_t));
    memcpy(&(param.ota_trigger), Ota, sizeof(ota_trigger_t));
    assert(GxCas_QueuePut(GXCAS_CRYPTOGUARD_OTA_TRIGGER, (void *)&param, sizeof(GxCas_CGOtaTrigger), 0) != -1);
}

/*
* <summary>
* Callback function
* Read structure from NV memmory
* </summary>
* <param name="nvmem">pointer to a nvmem_storage structure to be stored</param>
* nvmem_storage is of sizeof(nvmem_storage)
* <returns>nothing</returns>
*/
void Callback_LoadMemory(nvmem_storage *s_nvmem) {
#ifdef _DEMO
    demo_ReadNVMEM(s_nvmem);
#endif
    uint32_t  Offset = 0;
    
    GxCas_Flash_Read(Offset,(uint8_t *)s_nvmem,sizeof(nvmem_storage));
}

/*
* <summary>
* Callback function
* Store structure to NV memmory
* </summary>
* <param name="nvmem">pointer to a nvmem_storage structure to be stored</param>
* nvmem_storage is of size sizeof(nvmem_storage)
* <returns>nothing</returns>
*/
void Callback_StoreMemory(nvmem_storage *s_nvmem) {
#ifdef _DEMO
    demo_WriteNVMEM(s_nvmem);
#endif
    uint32_t  Offset = 0;
    
    GxCas_Flash_Write(Offset,(uint8_t *)s_nvmem,sizeof(nvmem_storage));
}

/*
* <summary>
* Callback function
* Transmits commands to the SMC
* </summary>
* <param name="header">Header information to the SMC</param>
* <param name="data">The data that should be sent to the SMC</param>
* <returns>Returns the response of the SendCommand</returns>
*/
unsigned short Callback_ISO7816_TransmitCommand(unsigned char header[5], unsigned char *data) {
    if (!header) {
        return -1;
    }

    unsigned short sw = 0;
#ifdef _DEMO
    BYTE d_sw[2];
    SMC_SendCommand(header, data, d_sw);
    sw = d_sw[0] << 8 | d_sw[1];
#endif
    int32_t rv;
    uint8_t buffer[256+5];
    uint8_t reply[256] = {0};
    uint32_t dwDataLength = header[4];
    uint32_t dwRecvLength = 2;

    memcpy(buffer, header, 5); 
    if (dwDataLength && data) {
        memcpy(buffer+5, data, dwDataLength);
    }
#ifdef _DEBUG_INFO
    CAS_DUMP(CAS, buffer, 5, "head :");
    CAS_DUMP(CAS, buffer+5, dwDataLength, "data :");
#endif // _DEBUG_CARD
    rv = GxCas_Smc_Apdu(buffer, dwDataLength+5, reply, &dwRecvLength);
    if(rv != 0){
        CAS_DBG(CAS, "(SendCommand :%d)", rv);
        return 0;
    }

#ifdef _DEBUG_INFO
    CAS_DBG(CAS, ": %02X %02X\r\n", reply[dwRecvLength-2], reply[dwRecvLength-1]);
#endif // _DEBUG_CARD
    sw = reply[dwRecvLength-2] << 8 | reply[dwRecvLength-1];
    return sw;
}

/*
* <summary>
* Callback function
* Receives data from the SMC
* </summary>
* <param name="header">Header information to the SMC</param>
* <param name="data">Pointer to a buffer that will be filled with information from the ReceiveCommand.</param>
* <returns>Returns the response of the ReceiveCommand</returns>
*/
unsigned short Callback_ISO7816_ReceiveCommand(unsigned char header[5], unsigned char *data) {
    if (!header) {
        return -1;
    }

    unsigned short sw = 0;
#ifdef _DEMO
    BYTE d_sw[2];
    SMC_ReceiveCommand(header, data, d_sw);
    sw = d_sw[0] << 8 | d_sw[1];
#endif
    int32_t rv;
    uint8_t reply[256+5] = {0};
    uint32_t dwRecvLength = (uint32_t)header[4]+2;

    rv = GxCas_Smc_Apdu(header, 5, reply, &dwRecvLength);
    if(rv != 0){
        CAS_DBG(CAS, "(SendCommand :%d)", rv);
        return 0;
    }

    if (data && dwRecvLength > 2) {
        memcpy((void*)data, reply, dwRecvLength-2);
    }
    sw = reply[dwRecvLength-2] << 8 | reply[dwRecvLength-1];
#ifdef _DEBUG_INFO
    CAS_DUMP(CAS, header, 5, "head :");
    CAS_DUMP(CAS, reply, dwRecvLength-2, "data :");
    CAS_DBG(CAS, ": %02X %02X\r\n", reply[dwRecvLength-2], reply[dwRecvLength-1]);
#endif // _DEBUG_CARD

    return sw;
}

/*
* <summary>
* Callback function
* Display OSD Message
* </summary>
* <param name="time_ms">The duration the message will be displayed in milliseconds</param>
* if duration 0 (infinite) Message can be canceled with OK or Exit keys
* <param name="message">The message that will be displayed</param>
* <returns>nothing</returns>
*/
void Callback_OSDMessage(uint32_t time_ms, const char *message) {
	ENCODE_TYPE encoding = ENCODE_NONE;
	int offset = 0;

	if(message == NULL)
		return;

	if (message[0] < 0x20) {
		offset = 1;
		switch (message[0]) {
		case 0x01:		encoding = ENCODE_ISO_8859_5;		break;
		case 0x02:		encoding = ENCODE_ISO_8859_6;		break;
		case 0x03:		encoding = ENCODE_ISO_8859_7;		break;
		case 0x04:		encoding = ENCODE_ISO_8859_8;		break;
		case 0x05:		encoding = ENCODE_ISO_8859_9;		break;
		case 0x09:		encoding = ENCODE_ISO_8859_13;		break;
		case 0x10:
			offset = 3;
			switch ((message[1]) << 8 | message[2]) {
			case 0x0001:encoding = ENCODE_ISO_8859_1; break;
			case 0x0002:encoding = ENCODE_ISO_8859_2; break;
			case 0x0003:encoding = ENCODE_ISO_8859_3; break;
			case 0x0004:encoding = ENCODE_ISO_8859_4; break;
			case 0x0005:encoding = ENCODE_ISO_8859_5; break;
			case 0x0006:encoding = ENCODE_ISO_8859_6; break;
			case 0x0007:encoding = ENCODE_ISO_8859_7; break;
			case 0x0008:encoding = ENCODE_ISO_8859_8; break;
			case 0x0009:encoding = ENCODE_ISO_8859_9; break;
			case 0x000A:encoding = ENCODE_ISO_8859_10; break;
			case 0x000B:encoding = ENCODE_ISO_8859_11; break;
			case 0x000C:encoding = ENCODE_ISO_8859_12; break;
			case 0x000D:encoding = ENCODE_ISO_8859_13; break;
			case 0x000E:encoding = ENCODE_ISO_8859_14; break;
			case 0x000F:encoding = ENCODE_ISO_8859_15; break;
				// This should not happen
			default:	encoding = ENCODE_NONE;	break;
			}	break;

			// This is GB2312 Simplified Chinese missing for some
		case 0x13: 	encoding = ENCODE_NONE;	break;

		case 0x15:	encoding = ENCODE_UTF_8; break;

			// All below not used by cryptoguard
		case 0x06:	case 0x07:	case 0x08:	case 0x0A:	case 0x0B:
		case 0x0C:	case 0x0D:	case 0x0E:	case 0x0F:	break;
		case 0x11:	case 0x12:	case 0x14:
		default:
			break;
		}
	}
#ifdef _DEBUG_INFO
    CAS_DBG(CAS, "----------------------------------------\r\n");
    CAS_DBG(CAS, "Display duration(ms):%d\r\n", time_ms);
	CAS_DBG(CAS, "Text: %s\r\n", message + offset);
    CAS_DBG(CAS, "Number for encoding: %d\r\n", encoding);
    CAS_DBG(CAS, "Message: 0x%02x 0x%02x 0x%02x\r\n", message[0], message[1], message[2]);
    CAS_DBG(CAS, "\n");
    CAS_DBG(CAS, "----------------------------------------\r\n");
#endif
    
    GxCas_CGShowOsdMessage  param = {0};
    if ((message[0]) == 0x10)
    {
        param.encoding = encoding;
        param.osd_msg.ms_duration = time_ms;
    	param.osd_msg.length = strlen(message + offset);
        memset(param.osd_msg.textstring, '\0', 128);
    	strcpy((char *)param.osd_msg.textstring, message + offset);
    }
    else
    {
        param.encoding = ENCODE_NONE;
        param.osd_msg.ms_duration = time_ms;
		if(message[0]== 0x13) //GB2312 show broken
		{
			param.osd_msg.length = strlen("broken");
			memset(param.osd_msg.textstring, '\0', 128);
			strcpy((char *)param.osd_msg.textstring, "broken");
		}
		else
		{
			param.osd_msg.length = strlen(message);
			memset(param.osd_msg.textstring, '\0', 128);
			strcpy((char *)param.osd_msg.textstring, message);
		}

    }

	assert(GxCas_QueuePut(GXCAS_CRYPTOGUARD_OSD_MESSAGE, (void *)&param, sizeof(GxCas_CGShowOsdMessage), 0) != -1);

}

/*
* <summary>
* Callback function
* Display PinRequired OSD Message
* </summary>
* <param name="time_ms">The duration the message will be displayed in milliseconds</param>
* if duration 0 (infinite) Message can be canceled with OK or Exit keys
* <param name="message">The message that will be displayed</param>
* <returns>nothing</returns>
*/
void Callback_PinRequired(uint32_t time_ms, const char *message) {
#ifdef _DEBUG_INFO
    if(!time_ms)
        CAS_DBG(CAS, "Pin required (infinite) : %s\r\n", message);
    else
        CAS_DBG(CAS, "Pin required (%d secs) : %s\r\n", (time_ms/1000), message);
#endif

    GxCas_CGPinRequired  param;
    memset(&(param.pin_required),0,sizeof(pin_descriptor_t));
    memcpy(&(param.pin_required.text), message, strlen(message));
	param.pin_required.rating = time_ms/1000;
	printf("lgs::[%s][%d] param.pin_required.rating = %d\n",__FUNCTION__,__LINE__,param.pin_required.rating);
    assert(GxCas_QueuePut(GXCAS_CRYPTOGUARD_PIN_REQUIRED, (void *)&param, sizeof(GxCas_CGPinRequired), 0) != -1);
}

