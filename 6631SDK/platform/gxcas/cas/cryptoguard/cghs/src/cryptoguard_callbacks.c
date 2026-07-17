#include <assert.h>
#include "gxcore.h"
#include "autoconf.h"
#include "gxcas_demux.h"
#include "gxcas_dbg.h"
#include "gxcas_queue.h"
#include "cg_cas.h"
#include "cg_callbacks.h"
#include "cryptoguard_callbacks.h"
#include "cryptoguard_api.h"
#define _DEMO

void Callback_STB_DisplayScrollMessage(scroll_descriptor_t *scroll) {
	ENCODE_TYPE encoding = ENCODE_NONE;
	int offset = 0;

	if(scroll == NULL)
		return;

	if (scroll->Text[0] < 0x20) {
		offset = 1;
		switch (scroll->Text[0]) {
		case 0x01:		encoding = ENCODE_ISO_8859_5;		break;
		case 0x02:		encoding = ENCODE_ISO_8859_6;		break;
		case 0x03:		encoding = ENCODE_ISO_8859_7;		break;
		case 0x04:		encoding = ENCODE_ISO_8859_8;		break;
		case 0x05:		encoding = ENCODE_ISO_8859_9;		break;
		case 0x09:		encoding = ENCODE_ISO_8859_13;		break;
		case 0x10:
			offset = 3;
			switch ((scroll->Text[1]) << 8 | scroll->Text[2]) {
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
#ifdef _DEMO
	printf("----------------------------------------\r\n");
	printf("%s message to be displayed\r\n", (scroll->Forced) ? "Forced" : "Normal");
	printf("Display duration(sec):%d\r\n", scroll->Duration);
	printf("Scroll speed(ms):%d\r\n", scroll->ScrollSpeed);
	printf("Text: %s\r\n", scroll->Text + offset);
	printf("Number for encoding: %d\r\n", encoding);
	printf("Y position %02d\r\n", scroll->Y);
	printf("Font color %08X\r\n", scroll->FontColor);
	printf("Background color %08X\r\n", scroll->BackgroundColor);
	printf("----------------------------------------\r\n");
#endif

	GxCas_CGScrollMessage  param = {0};
	param.encoding = encoding;
	param.scroll_msg.Duration = scroll->Duration;
	param.scroll_msg.Y = scroll->Y;
	param.scroll_msg.FontColor = scroll->FontColor;
	param.scroll_msg.BackgroundColor = scroll->BackgroundColor;
	param.scroll_msg.ScrollSpeed = scroll->ScrollSpeed;
	param.scroll_msg.Forced = scroll->Forced;
	param.scroll_msg.FontSize = scroll->FontSize;
	strcpy(param.scroll_msg.Text,scroll->Text + offset);
	assert(GxCas_QueuePut(GXCAS_CRYPTOGUARD_SCROLL_MESSAGE, (void *)&param, sizeof(GxCas_CGScrollMessage), 0) != -1);

}

void Callback_STB_DisplayMessage(message_descriptor_t *message) {
	ENCODE_TYPE encoding = ENCODE_NONE;
	int offset = 0;

	if(message == NULL)
		return;

	if (message->Text[0] < 0x20) {
		offset = 1;
		switch (message->Text[0]) {
		case 0x01:		encoding = ENCODE_ISO_8859_5;		break;
		case 0x02:		encoding = ENCODE_ISO_8859_6;		break;
		case 0x03:		encoding = ENCODE_ISO_8859_7;		break;
		case 0x04:		encoding = ENCODE_ISO_8859_8;		break;
		case 0x05:		encoding = ENCODE_ISO_8859_9;		break;
		case 0x09:		encoding = ENCODE_ISO_8859_13;		break;
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
#ifdef _DEMO
	printf("----------------------------------------\r\n");
	printf("%s message to be displayed\r\n", (message->Forced) ? "Forced" : "Normal");
	printf("Display duration(sec):%d\r\n", message->Duration);
    printf("Number for encoding: %d\r\n", encoding);
    printf("Text: ----------------------------------------\r\n");
	printf("%s\n", message->Text + offset);
#if 0
	int i=0;
    while (message->Text[i+offset]) {
		printf("%02X ", message->Text[offset + i++]);
    printf("\n");
	}
#endif
	printf("----------------------------------------\r\n");
#endif

	GxCas_CGShowLongMessage  param = {0};
	param.encoding = encoding;
	param.long_msg.Forced = message->Forced;
	param.long_msg.Duration = message->Duration;
	strcpy((char *)param.long_msg.Text, (const char *)(message->Text + offset));
	assert(GxCas_QueuePut(GXCAS_CRYPTOGUARD_LONG_MESSAGE, (void *)&param, sizeof(GxCas_CGShowLongMessage), 0) != -1);

}


/*
* <summary>
* Callback function
* Videorules has changed
* </summary>
* <param name="videorules->CGMSA">Copy Generation Management System - Analog</param>
* <param name="videorules->HDCP">High-bandwidth Digital Content Protection</param>
* <param name="videorules->Macrovision">Macrovision</param>
* <param name="videorules->AnaHD">AnaHD</param>
* <param name="videorules->DigHD">DigHD</param>
* <param name="videorules->CH">Session number</param>
* <returns>nothing</returns>
*/
void Callback_STB_VideoRulesChanged(ecm_stb_videorules_t *videorules) {
	if(videorules == NULL)
		return;

#ifdef _DEMO
	printf("-------------------------\r\n");
	printf("Video rules change CH : %d\r\n", videorules->CH);
	printf("CGMSA                 : %d \r\n", videorules->CGMSA);
	printf("HDCP                  : %d \r\n", videorules->HDCP);
	printf("Macrovision           : %d \r\n", videorules->Macrovision);
	printf("AnaHD                 : %d \r\n", videorules->AnaHD);
	printf("DigHD                 : %d \r\n", videorules->DigHD);
	printf("-------------------------\r\n");
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

void Callback_STB_OtaTriggerReceived(ota_trigger_t *Ota) {
	if(Ota == NULL)
		return;

#ifdef _DEMO
	printf("--------------------------\r\n");
	printf("Ota trigger has received \r\n");
	printf("Manufacturer : %d 0x%08X\r\n", Ota->Manufacturer, Ota->Manufacturer);
	printf("Model        : %d\r\n", Ota->Model);
	printf("Version      : %X\r\n", Ota->Version);
	printf("PID          : %d\r\n", Ota->PID);
	printf("Delivery     : (%d) ", Ota->Delivery);

	switch (Ota->Delivery) { // 1 = Cable delivery, 2 = Terrestial delivery, 3 = Satellite
	case 1:
		printf("Cable delivery\r\n");
		break;
	case 2:
		printf("Terrestial delivery\r\n");
		break;
	case 3:
		printf("Satellite delivery\r\n");
		break;
	default:
		printf("Undefined delivery\r\n");
		break;
	}

	printf("Frequency    : %d MHz\r\n", Ota->Frequency);
	printf("Symbolrate   : %d\r\n", Ota->Symbolrate);
	printf("Fec          : %d\r\n", Ota->Fec);
	printf("Polarity     : %d\r\n", Ota->Polarity);
	printf("Modulation   : (%d) ", Ota->Modulation);

	switch (Ota->Delivery) { // 1 = Cable delivery, 2 = Terrestial delivery, 3 = Satellite
	case 2:
		printf("64 QAM\r\n");
		break;
	case 3:
		printf("128 QAM\r\n");
		break;
	case 4:
		printf("256 QAM\r\n");
		break;
	default:
		printf("16 QAM\r\n");
		break;
	}

	printf("Bandwith     : %d MHz\r\n", Ota->Bandwith);
	printf("--------------------------\r\n");
#endif

	GxCas_CGOtaTrigger  param;
	memset(&(param.ota_trigger),0,sizeof(ota_trigger_t));
	memcpy(&(param.ota_trigger), Ota, sizeof(ota_trigger_t));
	assert(GxCas_QueuePut(GXCAS_CRYPTOGUARD_OTA_TRIGGER, (void *)&param, sizeof(GxCas_CGOtaTrigger), 0) != -1);

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
#ifdef _DEMO
	printf("----------------------------------------\r\n");
	printf("Display duration(ms):%d\r\n", time_ms);
	printf("Text: %s\r\n", message + offset);
	printf("Number for encoding: %d\r\n", encoding);
	printf("\n");
	printf("----------------------------------------\r\n");
#endif

	GxCas_CGShowOsdMessage  param = {0};
	param.encoding = encoding;
	param.osd_msg.ms_duration = time_ms;
	param.osd_msg.length = strlen(message)-offset;
	strcpy((char *)param.osd_msg.textstring, message + offset);
	assert(GxCas_QueuePut(GXCAS_CRYPTOGUARD_OSD_MESSAGE, (void *)&param, sizeof(GxCas_CGShowOsdMessage), 0) != -1);

}

/*
* <summary>
* Callback function
* Display PinRequired OSD Message
* </summary>
* <param name="rating">The rating that is requiered to watch channel</param>
* <param name="text">informative text for future use</param>
* <returns>nothing</returns>
*/
void Callback_PinRequired(pin_descriptor_t *pin) {
	if(pin == NULL)
		return;

#ifdef _DEMO
	printf("-------------------------------------------------------------------------------------------\r\n");
	printf("Here you should call a function to let user enter pin to unlock! NOT display this message!!\r\n");
	printf("Pin required (rating %d ) : %s\r\n", pin->rating, pin->text);
	printf("-------------------------------------------------------------------------------------------\r\n");
#endif

	GxCas_CGPinRequired  param;
	memset(&(param.pin_required),0,sizeof(pin_descriptor_t));
	memcpy(&(param.pin_required), pin, sizeof(pin_descriptor_t));
	assert(GxCas_QueuePut(GXCAS_CRYPTOGUARD_PIN_REQUIRED, (void *)&param, sizeof(GxCas_CGPinRequired), 0) != -1);

}

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
void Callback_STB_DisplayFingerPrint(fingerprint_descriptor_t *Fingerprint) {
	if(Fingerprint == NULL)
		return;

#ifdef _DEMO
	printf("-----------------------------\r\n");
	printf("%s     %d\r\n", Fingerprint->DisplayText, Fingerprint->CH);
	printf("X:%02d Y:%02d Duration:%07d ms\r\n", Fingerprint->X, Fingerprint->Y, Fingerprint->DurationMs);
	printf("Font:%08X  Back::%08X \r\n", Fingerprint->FontColor, Fingerprint->BackgroundColor);
	printf("-----------------------------\r\n");
#endif

	GxCas_CGFingerInfo  param;
	memset(&(param.finger_info),0,sizeof(fingerprint_descriptor_t));
	memcpy(&(param.finger_info), Fingerprint, sizeof(fingerprint_descriptor_t));
	assert(GxCas_QueuePut(GXCAS_CRYPTOGUARD_FINGER_INFO, (void *)&param, sizeof(GxCas_CGFingerInfo), 0) != -1);

}

