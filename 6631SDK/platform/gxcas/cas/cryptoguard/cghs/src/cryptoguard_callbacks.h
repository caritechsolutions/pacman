#include "gxcore.h"
#include "autoconf.h"
#include "gxcas_demux.h"
#include "gxcas_dbg.h"
#include "cg_cas.h"
#include "cg_callbacks.h"

void Callback_STB_DisplayScrollMessage(scroll_descriptor_t *scroll);
void Callback_STB_DisplayMessage(message_descriptor_t *message);
void Callback_STB_VideoRulesChanged(ecm_stb_videorules_t *videorules);
void Callback_STB_OtaTriggerReceived(ota_trigger_t *Ota);
void Callback_OSDMessage(uint32_t time_ms, const char *message);
void Callback_PinRequired(pin_descriptor_t *pin);
void Callback_STB_DisplayFingerPrint(fingerprint_descriptor_t *Fingerprint);

