/* Imported from the dvbstream project
 *
 * Modified for use with MPlayer, for details see the changelog at
 * http://svn.mplayerhq.hu/mplayer/trunk/
 * $Id: rtp.h 23709 2007-07-02 22:34:45Z diego $
 */

#ifndef RTP_H
#define RTP_H

#define MAX_RTP_PACKET_COUNT 128	// The number of max packets being reordered
#define MAX_RTP_PACKET_SIZE 1590
#define STREAM_RTP_MAX_PKT_SIZE 7*188

int read_rtp_from_server(int fd, char *buffer, int length, int time_over);
void init_rtp_cache(void);
void uninit_rtp_cache(void);

#endif
