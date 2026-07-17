#ifndef __DEMUX_DUMP_H__
#define __DEMUX_DUMP_H__

extern void* GxDemuxDump_Open(GxDemuxer *demuxer);
extern int   GxDemuxDump_Read (void *priv, GxDemuxStream *ds, unsigned char *buf, unsigned int size);
extern int   GxDemuxDump_Write(void *priv, GxDemuxStream *ds, unsigned char *buf, unsigned int size);
extern void  GxDemuxDump_Close(void *priv);

#endif
