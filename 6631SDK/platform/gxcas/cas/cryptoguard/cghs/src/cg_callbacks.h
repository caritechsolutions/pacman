#ifndef CALLBACKS_H_INCLUDED
#define CALLBACKS_H_INCLUDED
#ifdef __cplusplus
extern "C"
{
#endif

// functions needed to be implemented and registered by STBM
typedef void(*cg_debugprint_callback_func)(const char *fmt, ...);
void cg_debugprint_register_callback_func(cg_debugprint_callback_func callback);

typedef int32_cg(*cg_otp_mem_callback_func)(uint8_cg write, uint32_cg offset, uint8_cg*data, int32_cg length);
void cg_otp_mem_register_callback_func(cg_otp_mem_callback_func callback);

typedef void(*cg_display_fingerprint_callback_func)(fingerprint_descriptor_t *Fingerprint);
void cg_fingerprint_register_callback_func(cg_display_fingerprint_callback_func callback);

typedef void(*cg_do_force_tuning_callback_func)(delivery_descriptor_t *in_forcetune);
void cg_forcetune_register_callback_func(cg_do_force_tuning_callback_func callback);

typedef void(*cg_display_text_message_callback_func)(message_descriptor_t *message);
void cg_textmessage_register_callback_func(cg_display_text_message_callback_func callback);

typedef void(*cg_display_scroll_message_callback_func)(scroll_descriptor_t *message);
void cg_scroll_message_register_callback_func(cg_display_scroll_message_callback_func callback);

typedef void(*cg_request_pin_callback_func)(pin_descriptor_t *pin);
void cg_request_pin_register_callback_func(cg_request_pin_callback_func callback);

/*
* <summary>
* Callback function
* OTA Trigger received
* </summary>
* <param name="Ota">Ota trigger information</param>
* <returns>nothing</returns>
*/
typedef void(*cg_OTA_trigger_callback_func)(ota_trigger_t *Ota);
void cg_OTA_trigger_register_callback_func(cg_OTA_trigger_callback_func callback);


/*
* <summary>
* Callback function
* Read and write data NV memmory
* </summary>
* <param name="write">read write select, 1 write, 0 read</param>
* <param name="data">pointer to data</param>
* <param name="length">number of bytes to read or written</param>
* <returns>nothing</returns>
*/
typedef void(*cg_flash_mem_callback_func)(uint8_cg write, uint8_cg *data, uint32_cg length);
void cg_flash_mem_register_callback_func(cg_flash_mem_callback_func callback);

/*
* <summary>
* Callback function
* Display OSD Message
* </summary>
* <param name="time_ms">The duration the message will be displayed in milliseconds</param>
* if duration 0 (infinite) Message can be canceled with OK or Exit keys
* <param name="message">The message that will be displayed this message also holds encoding, if any</param>
* <returns>nothing</returns>
*/
typedef void(*cg_osd_message_callback_func)(uint32_cg time_ms, const char *message);
void cg_osdmessage_register_callback_func(cg_osd_message_callback_func callback);

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
* <returns>nothing</returns>
*/
typedef void(*cg_video_rules_changed_callback_func)(ecm_stb_videorules_t *videorules);
void cg_videorules_register_callback_func(cg_video_rules_changed_callback_func callback);

typedef enum
{
	ENCODE_NONE = 0,
	ENCODE_ASCII,
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

#ifdef __cplusplus
}
#endif
#endif // CALLBACKS_H_INCLUDED
