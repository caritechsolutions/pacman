
#ifndef __WATCHDOG_H__
#define __WATCHDOG_H__

void gx_wdt_init(int clk);
void gx_wdt_start(void);
void gx_wdt_stop(void);
void gx_wdt_settimeout(int ms);
void gx_wdt_ping(void);
void gx_wdt_reset_soon(int ms);

#endif
