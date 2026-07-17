/*
 * net_ipv4_tftp.c
 *
 * TFTP client
 * Based on RFC 1350 "The TFTP Protocol (Revision 2)"
 * Based on RFC 1782 "TFTP Option Extension"
 *
 * Written by Ho Lee 11/20/2002
 */

#include "config.h"
#include "net_priv.h"
#include "net_ipv4.h"

#include <net.h>

static const char *blkstr = "blksize";

/*
   TFTP file transfer example (from RFC 1782)

   Read Request

   client                                           server
   -------------------------------------------------------
   |1|foofile|0|octet|0|blksize|0|1432|0|  -->               RRQ
   <--  |6|blksize|0|1432|0|   OACK
   |4|0|  -->                                                ACK
   <--  |3|1| 1432 octets of data |   DATA
   |4|1|  -->                                                ACK
   <--  |3|2| 1432 octets of data |   DATA
   |4|2|  -->                                                ACK
   <--  |3|3|<1432 octets of data |   DATA
   |4|3|  -->                                                ACK
   */

int tftp_parsepacket(struct sk_buff *skb)
{
	skb->tftp = (struct tftphdr *) ((char *) skb->udp + sizeof(struct udphdr));
	skb->tftp->opcode = ntohs(skb->tftp->opcode);

	switch (skb->tftp->opcode) {
		case TFTPOP_DATA :
			skb->tftp->u.data.block = ntohs(skb->tftp->u.data.block);
			break;
		case TFTPOP_ACK :
			skb->tftp->u.ack.block = ntohs(skb->tftp->u.ack.block);
			break;
		case TFTPOP_ERROR :
			skb->tftp->u.err.errcode = ntohs(skb->tftp->u.err.errcode);
			break;
	}

	return 0;
}

void tftp_dump_packet(struct sk_buff *skb, int need_parsing)
{
	printf("TFTP : \n");
}

int ipv4_tftp(in_addr_t ipaddr, int port, int opcode, char *filename, unsigned int addr, unsigned int *datalen)
{
	tftpip_t packet;
	int len, nretry;
	int nloop = 0, nloop2 = 0;
	struct sk_buff *skb = NULL;
	unsigned char *dataptr;
	unsigned int iport = ipv4_alloc_port();
	unsigned int blksize = TFTP_MAX_PACKET;
	unsigned int last_block = 0;

	dataptr = (unsigned char *) addr;
	if(TFTPOP_RRQ == opcode)
		*datalen = 0;

	// send RRQ
	packet.tftp.opcode = htons(opcode);
	len = sizeof packet.tftp.opcode;
	len += sprintf((char *) packet.tftp.u.rrq, "%s%coctet%c%s%c%d",
			filename, 0, 0, blkstr, 0, TFTP_MAX_PACKET);
	len += 1;

	for (nretry = 1; nretry <= MAX_TFTP_RETRIES; ++nretry) {
		printf("%s (%s)...\n", (TFTPOP_RRQ == opcode)?"Getting":"Dumpping into", filename);
		udp_transmit(ipaddr, iport, port, len, &packet, 0);
		if ((skb = udp_receive(0, 0, iport, TIMEOUT)) != NULL)
			break;

		if (nretry == MAX_TFTP_RETRIES) {
			printf("Timeout. Connection failed.\n");
			return TFTPRET_NETWORKERR;
		} else {
			printf("Timeout. Retrying...\n");
		}
	}
	while (skb) {
		tftp_parsepacket(skb);

		switch (skb->tftp->opcode)
		{
			case TFTPOP_RRQ :
			case TFTPOP_WRQ :
				// invalid op code for client
				// just ignore them
				break;
			case TFTPOP_ACK :
				if(*datalen > skb->tftp->u.ack.block * blksize){
					if(skb->tftp->u.ack.block != last_block)
						continue;
					packet.tftp.opcode = htons(TFTPOP_DATA);
					packet.tftp.u.data.block = htons(skb->tftp->u.ack.block + 1);
					len = ((*datalen < (skb->tftp->u.ack.block + 1) * blksize)?*datalen - skb->tftp->u.ack.block * blksize : blksize);
					memcpy(packet.tftp.u.data.data, dataptr + skb->tftp->u.ack.block * blksize, len);
					udp_transmit(skb->ip->saddr, iport, skb->udp->src, len + 4, &packet, 0);
					last_block++;
				}else{
					skb_free(skb);
					return 0;
				}
				break;
			case TFTPOP_DATA :
				// print progress
				if ((nloop++ & 0x3f) == 0) {
					if ((nloop2++ & 0x3f) == 0) {
						if (nloop2 == 1)
							printf("Receiving");
						else
							printf("\n            ");
					}
					putc('.');
				}

				/* Save the last block# so if ack is lost we won't copy the
				   same packet more than once */
				if (last_block != skb->tftp->u.data.block) {
					// Append to data buffer
					len = skb->udpdata_len - 4;
					memcpy(dataptr, skb->tftp->u.data.data, len);
					dataptr += len;
					*datalen += len;
					last_block = skb->tftp->u.data.block;
				}

				// reply ACK
				packet.tftp.opcode = htons(TFTPOP_ACK);
				packet.tftp.u.ack.block = htons(skb->tftp->u.data.block);
				udp_transmit(skb->ip->saddr, iport, skb->udp->src, 4, &packet, 0);

				if (len < blksize) {
					printf("\n");
					//printf("Received %d (0x%x) bytes\n", *datalen, *datalen);
					skb_free(skb);
					return 0;
				}
				break;
			case TFTPOP_ERROR :
				switch (skb->tftp->u.err.errcode) {
					case TFTPERR_FILENOTFOUND :
						printf("File not found\n");
						skb_free(skb);
						return TFTPRET_FILENOTFOUND;
					case TFTPERR_ACCESSDENIED :
						printf("Access denied\n");
						skb_free(skb);
						return TFTPRET_ACCESSDENIED;
					default :
						printf("TFTP error %d : %s\n",
								skb->tftp->u.err.errcode, skb->tftp->u.err.errmsg);
						skb_free(skb);
						return TFTPRET_OTHER;
				}
				break;
			case TFTPOP_OACK :
				// reply with ACK
				if (strcmp((char*)skb->tftp->u.oack.data, (char*)blkstr) == 0) {
					blksize = atoi((char*)&(skb->tftp->u.oack.data[strlen(blkstr) + 1]));
					//printf("Block size set to %d.\n", blksize);
				}
				packet.tftp.opcode = htons(TFTPOP_ACK);
				packet.tftp.u.ack.block = 0;
				udp_transmit(skb->ip->saddr, iport, skb->udp->src, 4, &packet, 0);
				break;
			default:
				break;
		}

		skb_free(skb);

		for (nretry = 1; nretry <= MAX_TFTP_RETRIES; ++nretry) {
			if ((skb = udp_receive(0, 0, iport, TIMEOUT)) != NULL)
				break;
			if (nretry == MAX_TFTP_RETRIES) {
				printf("Timeout. Transfer failed.\n");
				return TFTPRET_NETWORKERR;
			}
		}
	}

	return 0;
}
