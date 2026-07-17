#ifndef CALLBACKS_H_INCLUDED
#define CALLBACKS_H_INCLUDED

#include "IntCAM.h"
#include <stdint.h> // used for uint32_t

// Callback functions needed to be implemented by Integrator
// -------------------
// CALLBACK FUNCTIONS
// -------------------

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
void Callback_STB_DisplayFingerPrint(unsigned char CH, fingerprint_descriptor *Fingerprint);

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
void Callback_STB_DoForcetune(delivery_descriptor *in_forcetune);

/*
* <summary>
* Callback function
* Display EMM message on screen
* </summary>
* <param name="message->Duration">duration to display message on screen</param>
* <param name="message->Forced">0 Message can be canceled with OK or Exit keys, 1 Message has to be shown full duration</param>
* <param name="message->Text">Text string to be displayed on screen, including linefeed and CR for multiple line support and character encoding</param>
* <returns>nothing</returns>
*/
void Callback_STB_DisplayMessage(message_descriptor *message);

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
void Callback_STB_DisplayScrollMessage(scroll_descriptor *scroll);

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
void Callback_STB_VideoRulesChanged(unsigned char CH, ecm_STB_videorules *videorules);

/*
* <summary>
* Callback function
* OTA Trigger received
* </summary>
* <param name="Ota">Ota trigger data</param>
* <returns>nothing</returns>
*/
void Callback_STB_OtaTriggerReceived(ota_trigger *Ota);

/*
* <summary>
* Callback function
* Store structure to NV memmory
* </summary>
* <param name="nvmem">pointer to a nvmem_storage structure to be stored</param>
* nvmem_storage is of size sizeof(nvmem_storage)
* <returns>nothing</returns>
*/
void Callback_LoadMemory(nvmem_storage *nvmem);

/*
* <summary>
* Callback function
* Read structure from NV memmory
* </summary>
* <param name="nvmem">pointer to a nvmem_storage structure to be stored</param>
* nvmem_storage is of size sizeof(nvmem_storage)
* <returns>nothing</returns>
*/
void Callback_StoreMemory(nvmem_storage *nvmem);

/*
* <summary>
* Callback function
* Transmits commands to the SMC
* </summary>
* <param name="header">Header information to the SMC</param>
* <param name="data">The data that should be sent to the SMC</param>
* <returns>Returns the response of the SendCommand</returns>
*/
unsigned short Callback_ISO7816_TransmitCommand(unsigned char header[5], unsigned char *data);

/*
* <summary>
* Callback function
* Receives data from the SMC
* </summary>
* <param name="header">Header information to the SMC</param>
* <param name="data">Pointer to a buffer that will be filled with information from the ReceiveCommand.</param>
* <returns>Returns the response of the ReceiveCommand</returns>
*/
unsigned short Callback_ISO7816_ReceiveCommand(unsigned char header[5], unsigned char *data);

/*
* <summary>
* Callback function
* Display OSD Message
* </summary>
* <param name="time_ms">The duration the message will be displayed in milliseconds</param>
* if duration 0 (infinite) Message can be canceled with OK or Exit keys
* <param name="message">The message that will be displayed including character encoding if any</param>
* <returns>nothing</returns>
*/
void Callback_OSDMessage(uint32_t time_ms, const char *message);


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
void Callback_PinRequired(uint32_t time_ms, const char *message);

#endif // CALLBACKS_H_INCLUDED
