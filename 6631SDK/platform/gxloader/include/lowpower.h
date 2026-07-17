#ifndef __LOWPOWER_H__
#define __LOWPOWER_H__

/*
 * lowpower_get_wake_flag()
 *
 *  DESCRIPTION
 *      Get wake up flag when wake by low-power
 *
 *  PARAMETER
 *      wakeflag
 *      0: not wake up
 *      1: manual wake up
 *      2: auto wake when waketime was up
 *
 *
 *  RETURN VALUE
 *  0: success
 *  < 0:  failed
 *  NOTES
 *
 * */

int lowpower_get_wake_flag(unsigned int *wakeflag);

#endif
