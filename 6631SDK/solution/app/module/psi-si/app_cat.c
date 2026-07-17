#include "module/app_cat.h"
#if EMM_SUPPORT
#define CAT_DATA_LENGTH    (1024)
#define CAT_DATA_FIFO_NUM_    (1)

#define out_SB_NL(str,hex)       app_log_debug("%s%u (0x%02x)\n",(str),(hex),(hex))
#define out_SW_NL(str,hex)       app_log_debug("%s%u (0x%04x)\n",(str),(hex),(hex))
#define out_SL_NL(str,hex)       app_log_debug("%s%lu (0x%08lx)\n",(str),(hex),(hex))
#define out_S2B_NL(str,hex,str2) app_log_debug("%s%u (0x%02x) [= %s]\n",(str),(hex),(hex),(str2))
#define out_S2W_NL(str,hex,str2) app_log_debug("%s%u (0x%04x)  [= %s]\n",(str),(hex),(hex),(str2))

static struct app_cat s_AppCat =
{
    .subt_id = -1,
    .timeout = 5, //5s
    .ts_src  = 0,
    .demuxid = 0,
    .pid     = 0,
    .req_id  = 0,
    .ver     = 0xff,
};

typedef struct _STR_TABLE
{
    unsigned int  from; /* e.g. from id 1  */
    unsigned int  to;   /*      to   id 3  */
    const char    *str; /*      is   string xxx */
} STR_TABLE;

typedef struct
{
    uint8_t section_data[CAT_DATA_LENGTH];
    uint32_t sub_idx;
}CatInfo;

static handle_t s_cat_thread_handle = -1;
static handle_t s_cat_mutex = -1;
static handle_t s_cat_fifo = -1;



/*
   -- get bits out of buffer  (max 48 bit)
   -- extended bitrange, so it's slower
   -- return: value
*/

static long long getBits48(unsigned char *buf, int byte_offset, int startbit, int bitlen)
{
    unsigned char *b;
    unsigned long long v;
    unsigned long long mask;
    unsigned long long tmp;

    if (bitlen > 48)
    {
        app_log_error(" Error: getBits48() request out of bound!!!! (report!!) \n");
        return 0xFEFEFEFEFEFEFEFELL;
    }

    b = &buf[byte_offset + (startbit / 8)];
    startbit %= 8;

    // -- safe is 48 bitlen
    tmp = (unsigned long long)(
              ((unsigned long long) * (b) << 48) + ((unsigned long long) * (b + 1) << 40) +
              ((unsigned long long) * (b + 2) << 32) + ((unsigned long long) * (b + 3) << 24) +
              (*(b + 4) << 16) + (*(b + 5) << 8) + * (b + 6));

    startbit = 56 - startbit - bitlen;
    tmp      = tmp >> startbit;
    mask     = (1ULL << bitlen) - 1;	// 1ULL !!!
    v        = tmp & mask;

    return v;
}

/*
   -- get bits out of buffer (max 32 bit!!!)
   -- return: value
*/

static unsigned long getBits(unsigned char *buf, int byte_offset, int startbit, int bitlen)
{
    unsigned char *b;
    unsigned long  v;
    unsigned long mask;
    unsigned long tmp_long;
    int           bitHigh;

    b = &buf[byte_offset + (startbit >> 3)];
    startbit %= 8;

    switch ((bitlen - 1) >> 3)
    {
        case -1:	// -- <=0 bits: always 0
            return 0L;
            break;

        case 0:		// -- 1..8 bit
            tmp_long = (unsigned long)(
                           (*(b) << 8) +  * (b + 1));
            bitHigh = 16;
            break;

        case 1:		// -- 9..16 bit
            tmp_long = (unsigned long)(
                           (*(b) << 16) + (*(b + 1) << 8) +  * (b + 2));
            bitHigh = 24;
            break;

        case 2:		// -- 17..24 bit
            tmp_long = (unsigned long)(
                           (*(b) << 24) + (*(b + 1) << 16) +
                           (*(b + 2) << 8) +  * (b + 3));
            bitHigh = 32;
            break;

        case 3:		// -- 25..32 bit
            // -- to be safe, we need 32+8 bit as shift range
            return (unsigned long) getBits48(b, 0, startbit, bitlen);
            break;

        default:	// -- 33.. bits: fail, deliver constant fail value
            app_log_error(" Error: getBits() request out of bound!!!! (report!!) \n");
            return (unsigned long) 0xFEFEFEFE;
            break;
    }

    startbit = bitHigh - startbit - bitlen;
    tmp_long = tmp_long >> startbit;
    mask     = (1ULL << bitlen) - 1;  // 1ULL !!!
    v        = tmp_long & mask;

    return v;
}

static char *findTableID(STR_TABLE *t, unsigned int id)
{
    while (t->str)
    {
        if (t->from <= id && t->to >= id)
        {
            return (char *) t->str;
        }
        t++;
    }

    return (char *) ">>ERROR: not (yet) defined... Report!<<";
}

/*
   -- current_next_indicator
   -- ISO/IEC13818-1|ITU H.222.0
*/
static char *dvbstrCurrentNextIndicator(unsigned int flag)
{
    STR_TABLE  Table[] =
    {
        {  0x00, 0x00,  "valid next" },
        {  0x01, 0x01,  "valid now" },
        {  0, 0, NULL }
    };

    return findTableID(Table, flag);
}

/*
   --  Table IDs (sections)
   ETSI EN 468   5.2
   EN 301 192
   TR 102 006
   RE 101 202
   ISO 13818-1
   TS 102 323
   TS 101 191
*/

static char *dvbstrTableID(unsigned int id)
{
    STR_TABLE  TableIDs[] =
    {

        // updated -- 2003-11-04
        // -- updated 2004-07-26  from ITU-T Rec H.222.0 | ISO/IEC 13818-1:2000/FDAM 1
        // -- updated 2004-08-12  from ITU-T Rec H.222.0 AMD2
        // ATSC Table IDs could be included...

        {  0x00, 0x00,  "Program Association Table (PAT)" },
        {  0x01, 0x01,  "Conditional Access Table (CAT)" },
        {  0x02, 0x02,  "Program Map Table (PMT)" },
        {  0x03, 0x03,  "Transport Stream Description Table (TSDT)" },
        {  0x04, 0x04,  "ISO_IEC_14496_scene_description_section" },	/* $$$ TODO */
        {  0x05, 0x05,  "ISO_IEC_14496_object_description_section" },	/* $$$ TODO */
        {  0x06, 0x06,  "Metadata Table" },				// $$$ TODO H.222.0 AMD1
        {  0x07, 0x07,  "IPMP_Control_Information_section (ISO 13818-11)" }, // $$$ TODO H.222.0 AMD1
        {  0x08, 0x37,  "ITU-T Rec. H.222.0|ISO/IEC13818 reserved" },
        {  0x38, 0x39,  "ISO/IEC 13818-6 reserved" },
        {  0x3a, 0x3a,  "DSM-CC - multiprotocol encapsulated data" },
        {  0x3b, 0x3b,  "DSM-CC - U-N messages (DSI or DII)" },
        {  0x3c, 0x3c,  "DSM-CC - Download Data Messages (DDB)" },    /* TR 101 202 */
        {  0x3d, 0x3d,  "DSM-CC - stream descriptorlist" },
        {  0x3e, 0x3e,  "DSM-CC - private data section  // DVB datagram" }, /* EN 301 192 // ISO 13818-6 */
        {  0x3f, 0x3f,  "ISO/IEC 13818-6 reserved" },

        {  0x40, 0x40,  "Network Information Table (NIT) - actual network" },
        {  0x41, 0x41,  "Network Information Table (NIT) - other network" },
        {  0x42, 0x42,  "Service Description Table (SDT) - actual transport stream" },
        {  0x43, 0x45,  "reserved" },
        {  0x46, 0x46,  "Service Description Table (SDT) - other transport stream" },
        {  0x47, 0x49,  "reserved" },
        {  0x4A, 0x4A,  "Bouquet Association Table (BAT)" },
        {  0x4B, 0x4B,  "Update Notification Table (UNT)" },	/* TR 102 006 */
        {  0x4C, 0x4C,  "IP/MAC Notification Table (INT) [EN 301 192]" },  /* EN 192 */
        {  0x4D, 0x4D,  "reserved" },

        {  0x4E, 0x4E,  "Event Information Table (EIT) - actual transport stream, present/following" },
        {  0x4F, 0x4F,  "Event Information Table (EIT) - other transport stream, present/following" },
        {  0x50, 0x5F,  "Event Information Table (EIT) - actual transport stream, schedule" },
        {  0x60, 0x6F,  "Event Information Table (EIT) - other transport stream, schedule" },
        {  0x70, 0x70,  "Time Date Table (TDT)" },
        {  0x71, 0x71,  "Running Status Table (RST)" },
        {  0x72, 0x72,  "Stuffing Table (ST)" },
        {  0x73, 0x73,  "Time Offset Table (TOT)" },
        {  0x74, 0x74,  "MHP- Application Information Table (AIT)" }, 	/* MHP */
        {  0x75, 0x75,  "TVA- Container Table (CT)" }, 			/* TS 102 323 */
        {  0x76, 0x76,  "TVA- Related Content Table (RCT)" }, 		/* TS 102 323 */
        {  0x77, 0x77,  "TVA- Content Identifier Table (CIT)" },	 	/* TS 102 323 */
        {  0x78, 0x78,  "MPE-FEC Table (MFT)" }, 				/* EN 301 192 v1.4.1*/
        {  0x79, 0x79,  "TVA- Resolution Notification Table (RNT)" },	/* TS 102 323 */
        {  0x80, 0x7D,  "reserved" },
        {  0x7E, 0x7E,  "Discontinuity Information Table (DIT)" },
        {  0x7F, 0x7F,  "Selection Information Table (SIT)" },
        {  0x80, 0x8F,  "DVB CA message section (EMM/ECM)" },   /* ITU-R BT.1300 ref. */
        {  0x90, 0xBF,  "User private" },
        {  0xC0, 0xFE,  "ATSC reserved" },		/* ETR 211e02 */
        {  0xFF, 0xFF,  "forbidden" },
        {  0, 0, NULL }
    };

    return findTableID(TableIDs, id);
}

/*
   -- ISO Descriptor table tags
   -- ISO 13818-1, etc.
   -- 2004-08-11 Updated H.222.0 AMD1
   -- 2004-08-12 Updated H.222.0 AMD3
   -- 2005-11-10 Updated H.222.0 (2000)/Cor.4 (09/2005)
*/
static char *dvbstrMPEGDescriptorTAG(unsigned int tag)
{
    STR_TABLE  Tags[] =
    {
        {  0x00, 0x01,  "Reserved" },
        {  0x02, 0x02,  "video_stream_descriptor" },
        {  0x03, 0x03,  "audio_stream_descriptor" },
        {  0x04, 0x04,  "hierarchy_descriptor" },
        {  0x05, 0x05,  "registration_descriptor" },
        {  0x06, 0x06,  "data_stream_alignment_descriptor" },
        {  0x07, 0x07,  "target_background_grid_descriptor" },
        {  0x08, 0x08,  "video_window_descriptor" },
        {  0x09, 0x09,  "CA_descriptor" },
        {  0x0A, 0x0A,  "ISO_639_language_descriptor" },
        {  0x0B, 0x0B,  "system_clock_descriptor" },
        {  0x0C, 0x0C,  "multiplex_buffer_utilization_descriptor" },
        {  0x0D, 0x0D,  "copyright_descriptor" },
        {  0x0E, 0x0E,  "maximum_bitrate_descriptor" },
        {  0x0F, 0x0F,  "private_data_indicator_descriptor" },
        {  0x10, 0x10,  "smoothing_buffer_descriptor" },
        {  0x11, 0x11,  "STD_descriptor" },
        {  0x12, 0x12,  "IBP_descriptor" },
        /* MPEG DSM-CC */
        {  0x13, 0x13,  "carousel_identifier_descriptor" },
        {  0x14, 0x14,  "association_tag_descriptor" },
        {  0x15, 0x15,  "deferred_association_tag_descriptor" },
        {  0x16, 0x16,  "ISO/IEC13818-6 Reserved" },
        /* DSM-CC stream descriptors */
        {  0x17, 0x17,  "NPT_reference_descriptor" },
        {  0x18, 0x18,  "NPT_endpoint_descriptor" },
        {  0x19, 0x19,  "stream_mode_descriptor" },
        {  0x1A, 0x1A,  "stream_event_descriptor" },
        /* MPEG-4 descriptors */
        {  0x1B, 0x1B,  "MPEG4_video_descriptor" },
        {  0x1C, 0x1C,  "MPEG4_audio_descriptor" },
        {  0x1D, 0x1D,  "IOD_descriptor" },
        {  0x1E, 0x1E,  "SL_descriptor" },
        {  0x1F, 0x1F,  "FMC_descriptor" },
        {  0x20, 0x20,  "External_ES_ID_descriptor" },
        {  0x21, 0x21,  "MuxCode_descriptor" },
        {  0x22, 0x22,  "FMXBufferSize_descriptor" },
        {  0x23, 0x23,  "MultiplexBuffer_descriptor" },
        {  0x24, 0x24,  "Content_labeling_descriptor" },
        /* TV ANYTIME TS 102 323 descriptors, ISO 13818-1 */
        {  0x25, 0x25,  "metadata_pointer_descriptor" },
        {  0x26, 0x26,  "metadata_descriptor" },
        {  0x27, 0x27,  "metadata_STD_descriptor" },
        {  0x28, 0x28,  "AVC_video_descriptor" },
        {  0x29, 0x29,  "IPMP_descriptor (MPEG-2 IPMP, ISO 13818-11)" },
        {  0x2A, 0x2A,  "AVC_timing_and_HRD_descriptor" },

        {  0x2B, 0x2B,  "MPEG-2_AAC_audio_descriptor" },
        {  0x2C, 0x2C,  "FlexMuxTiming_descriptor" },
        {  0x2D, 0x3F,  "ITU-T.Rec.H.222.0|ISO/IEC13818-1 Reserved" },

        {  0x40, 0xFF,  "Forbidden descriptor in MPEG context" },	// DVB Context
        {  0, 0, NULL }
    };

    return findTableID(Tags, tag);
}

/*
   -- CA-System Identifier  (ETSI ETR 162)
*/
static char *dvbstrCASystem_ID(unsigned int id)
{
    STR_TABLE  Table[] =
    {
        // -- updated from dvb.org 2003-10-16
        {  0x0000, 0x0000,  "Reserved" },
        {  0x0001, 0x00FF,  "Standardized Systems" },
        {  0x0100, 0x01FF,  "Canal Plus (Seca/MediaGuard)" },
        {  0x0200, 0x02FF,  "CCETT" },
        {  0x0300, 0x03FF,  "MSG MediaServices GmbH" },
        {  0x0400, 0x04FF,  "Eurodec" },
        {  0x0500, 0x05FF,  "France Telecom (Viaccess)" },
        {  0x0600, 0x06FF,  "Irdeto" },
        {  0x0700, 0x07FF,  "Jerrold/GI/Motorola" },
        {  0x0800, 0x08FF,  "Matra Communication" },
        {  0x0900, 0x09FF,  "News Datacom (Videoguard)" },
        {  0x0A00, 0x0AFF,  "Nokia" },
        {  0x0B00, 0x0BFF,  "Norwegian Telekom (Conax)" },
        {  0x0C00, 0x0CFF,  "NTL" },
        {  0x0D00, 0x0DFF,  "Philips (Cryptoworks)" },
        {  0x0E00, 0x0EFF,  "Scientific Atlanta (Power VU)" },
        {  0x0F00, 0x0FFF,  "Sony" },
        {  0x1000, 0x10FF,  "Tandberg Television" },
        {  0x1100, 0x11FF,  "Thompson" },
        {  0x1200, 0x12FF,  "TV/COM" },
        {  0x1300, 0x13FF,  "HPT - Croatian Post and Telecommunications" },
        {  0x1400, 0x14FF,  "HRT - Croatian Radio and Television" },
        {  0x1500, 0x15FF,  "IBM" },
        {  0x1600, 0x16FF,  "Nera" },
        {  0x1700, 0x17FF,  "Beta Technik (Betacrypt)" },
        {  0x1800, 0x18FF,  "Kudelski SA"},
        {  0x1900, 0x19FF,  "Titan Information Systems"},
        {  0x2000, 0x20FF,  "Telef?nica Servicios Audiovisuales"},
        {  0x2100, 0x21FF,  "STENTOR (France Telecom, CNES and DGA)"},
        {  0x2200, 0x22FF,  "Scopus Network Technologies"},
        {  0x2300, 0x23FF,  "BARCO AS"},
        {  0x2400, 0x24FF,  "StarGuide Digital Networks  "},
        {  0x2500, 0x25FF,  "Mentor Data System, Inc."},
        {  0x2600, 0x26FF,  "European Broadcasting Union"},
        {  0x4700, 0x47FF,  "General Instrument"},
        {  0x4800, 0x48FF,  "Telemann"},
        {  0x4900, 0x49FF,  "Digital TV Industry Alliance of China"},
        {  0x4A00, 0x4A0F,  "Tsinghua TongFang"},
        {  0x4A10, 0x4A1F,  "Easycas"},
        {  0x4A20, 0x4A2F,  "AlphaCrypt"},
        {  0x4A30, 0x4A3F,  "DVN Holdings"},
        {  0x4A40, 0x4A4F,  "Shanghai Advanced Digital Technology Co. Ltd. (ADT)"},
        {  0x4A50, 0x4A5F,  "Shenzhen Kingsky Company (China) Ltd"},
        {  0x4A60, 0x4A6F,  "@SKY"},
        {  0x4A70, 0x4A7F,  "DreamCrypt"},
        {  0x4A80, 0x4A8F,  "THALESCrypt"},
        {  0x4A90, 0x4A9F,  "Runcom Technologies"},
        {  0x4AA0, 0x4AAF,  "SIDSA"},
        {  0x4AB0, 0x4ABF,  "Beijing Compunicate Technology Inc."},
        {  0x4AC0, 0x4ACF,  "Latens Systems Ltd"},
        {  0, 0, NULL }
    };

    return findTableID(Table, id);
}

/*
   0x09 CA descriptor  (condition access)
*/
static void descriptorMPEG_CA(unsigned char *b)
{
    /* IS13818-1  2.6.1 */
    typedef struct  _descCA
    {
        unsigned int      descriptor_tag;
        unsigned int      descriptor_length;
        unsigned int      CA_system_ID;
        unsigned int      reserved;
        unsigned int      CA_PID;
        // private data bytes
    } descCA;

    descCA  d;
    int     len;

    d.descriptor_tag    = b[0];
    d.descriptor_length = b[1];
    d.CA_system_ID      = getBits(b, 0, 16, 16);
    d.reserved          = getBits(b, 0, 32, 3);
    d.CA_PID            = getBits(b, 0, 35, 13);

    //store_PidToMem (d.CA_PID);

    out_S2W_NL("CA_system_ID: ", d.CA_system_ID, dvbstrCASystem_ID(d.CA_system_ID));
    out_SB_NL("reserved: ", d.reserved);
    out_SW_NL("CA_PID: ", d.CA_PID);

    len = d.descriptor_length - 4;
    if (len > 0)
    {
        app_log_debug("Private Data Len: %d\n", len);
    }

    if (d.CA_system_ID >= 0x0E00 && d.CA_system_ID <= 0x0EFF)
    {
        app_emm_start(s_AppCat.ts_src, s_AppCat.demuxid, d.CA_PID, d.CA_system_ID, 5);
    }
}

/*
   determine MPEG descriptor type and print it...
   return byte length
   2004-08-11  updated H.222.0 AMD1
*/
static int  descriptorMPEG(unsigned char *b)
{
    int len;
    int tag;

    tag = getBits(b, 0, 0, 8);
    len = getBits(b, 0, 8, 8);
    out_S2B_NL("MPEG-DescriptorTag: ", tag, dvbstrMPEGDescriptorTAG(tag));

    if (len == 0)
    {
        return len;
    }

    switch (tag)
    {

#if 0
        case 0x02:
            descriptorMPEG_VideoStream(b);
            break;
        case 0x03:
            descriptorMPEG_AudioStream(b);
            break;
        case 0x04:
            descriptorMPEG_Hierarchy(b);
            break;
        case 0x05:
            descriptorMPEG_Registration(b);
            break;
        case 0x06:
            descriptorMPEG_DataStreamAlignment(b);
            break;
        case 0x07:
            descriptorMPEG_TargetBackgroundGrid(b);
            break;
        case 0x08:
            descriptorMPEG_VideoWindow(b);
            break;
        case 0x09:
            descriptorMPEG_CA(b);
            break;
        case 0x0A:
            descriptorMPEG_ISO639_Lang(b);
            break;
        case 0x0B:
            descriptorMPEG_SystemClock(b);
            break;
        case 0x0C:
            descriptorMPEG_MultiplexBufUtil(b);
            break;
        case 0x0D:
            descriptorMPEG_Copyright(b);
            break;
        case 0x0E:
            descriptorMPEG_MaxBitrate(b);
            break;
        case 0x0F:
            descriptorMPEG_PrivateDataIndicator(b);
            break;
        case 0x10:
            descriptorMPEG_SmoothingBuf(b);
            break;
        case 0x11:
            descriptorMPEG_STD(b);
            break;
        case 0x12:
            descriptorMPEG_IBP(b);
            break;

        /* 0x13 - 0x1A  DSM-CC ISO13818-6,  TR 102 006 */
        case 0x13:
            descriptorMPEG_Carousel_Identifier(b);
            break;
        case 0x14:
            descriptorMPEG_Association_tag(b);
            break;
        case 0x15:
            descriptorMPEG_Deferred_Association_tags(b);
            break;

        /* DSM-CC stream descriptors */
        // case 0x16: reserved....
        case 0x17:
            descriptorMPEG_NPT_reference(b);
            break;
        case 0x18:
            descriptorMPEG_NPT_endpoint(b);
            break;
        case 0x19:
            descriptorMPEG_stream_mode(b);
            break;
        case 0x1A:
            descriptorMPEG_stream_event(b);
            break;

        /* MPEG 4 */
        case 0x1B:
            descriptorMPEG_MPEG4_video(b);
            break;
        case 0x1C:
            descriptorMPEG_MPEG4_audio(b);
            break;
        case 0x1D:
            descriptorMPEG_IOD(b);
            break;
        case 0x1E:
            descriptorMPEG_SL(b);
            break;
        case 0x1F:
            descriptorMPEG_FMC(b);
            break;
        case 0x20:
            descriptorMPEG_External_ES_ID(b);
            break;
        case 0x21:
            descriptorMPEG_MuxCode(b);
            break;
        case 0x22:
            descriptorMPEG_FMXBufferSize(b);
            break;
        case 0x23:
            descriptorMPEG_MultiplexBuffer(b);
            break;
        case 0x24:
            descriptorMPEG_ContentLabeling(b);
            break;

        /* TV ANYTIME, TS 102 323 */
        case 0x25:
            descriptorMPEG_TVA_metadata_pointer(b);
            break;
        case 0x26:
            descriptorMPEG_TVA_metadata(b);
            break;
        case 0x27:
            descriptorMPEG_TVA_metadata_STD(b);
            break;

        /* H.222.0 AMD 3 */
        case 0x28:
            descriptorMPEG_AVC_video(b);
            break;
        case 0x29:
            descriptorMPEG_IPMP(b);
            break;
        case 0x2A:
            descriptorMPEG_AVC_timing_and_HRD(b);
            break;

        /* H.222.0 Corr 4 */
        case 0x2B:
            descriptorMPEG_MPEG2_AAC_audio(b);
            break;
        case 0x2C:
            descriptorMPEG_FlexMuxTiming(b);
            break;
#endif
        case 0x09:
            descriptorMPEG_CA(b);
            break;

        default:
            if (tag < 0x80)
            {
                app_log_error("  ----> ERROR: unimplemented descriptor (mpeg context), Report!\n");
            }
            //descriptor_PRIVATE (b,MPEG);
            break;
    }

    return len + 2; // (descriptor total length)
}

/*
   determine descriptor type and print it...
   return byte length
*/
static int descriptor(unsigned char *b)
{
    int len;
    int id;

    id  = (int)b[0];
    len = ((int)b[1]) + 2;   	// total length

    if (id < 0x40)
    {
        descriptorMPEG(b);
    }
    //else
    //    descriptorDVB (b);

    return len;   // (descriptor total length)
}
#if 0
/*
 * @return PRIVATE_SECTION_OK   :GXMSG_SI_SECTION_OK
 *         PRIVATE_SUBTABLE_OK  :GXMSG_SI_SUBTABLE_OK
 */
static private_parse_status _cat_filter_callback(uint8_t *p_section_data, size_t section_size)
{
    /*  IS13818-1  S. 63 */
    /*  see also: ETS 468, ETR 289 */
    int len = 0;
    unsigned long CRC;

    if (!p_section_data)
    {
        return PRIVATE_SECTION_OK;
    }

    unsigned char *b = p_section_data;

    unsigned int table_id                 = b[0];
    unsigned int section_syntax_indicator = getBits(b, 0, 8, 1);
    unsigned int reserved_1               = getBits(b, 0, 10, 2);
    unsigned int section_length           = getBits(b, 0, 12, 12);
    unsigned int reserved_2               = getBits(b, 0, 24, 18);
    unsigned int version_number           = getBits(b, 0, 42, 5);
    unsigned int current_next_indicator   = getBits(b, 0, 47, 1);
    unsigned int section_number           = getBits(b, 0, 48, 8);
    unsigned int last_section_number      = getBits(b, 0, 56, 8);

    if (table_id != 0x01)
    {
        app_log_error("wrong Table ID");
        return PRIVATE_SECTION_OK;
    }

    app_log_info("CAT-decoding....\n");
    out_S2B_NL("Table_ID: ", table_id, dvbstrTableID(table_id));
    out_SB_NL("section_syntax_indicator: ", section_syntax_indicator);
    out_SB_NL("(fixed): ", 0);
    out_SB_NL("reserved_1: ", reserved_1);
    out_SW_NL("Section_length: ", section_length);
    out_SB_NL("reserved_2: ", reserved_2);
    out_SB_NL("Version_number: ", version_number);
    out_S2B_NL("current_next_indicator: ", current_next_indicator, dvbstrCurrentNextIndicator(current_next_indicator));
    out_SB_NL("Section_number: ", section_number);
    out_SB_NL("Last_Section_number: ", last_section_number);

    // buffer + header, len = len - header - CRC
    // Descriptor ISO 13818 - 2.6.1
    // - header - CRC
    len = section_length - 5;
    b  += 8;

    app_log_debug("\033[31m\n");
    while (len > 4)
    {
        int i = descriptor(b);
        app_log_debug("\n");
        len -= i;
        b += i;

    }
    app_log_debug("\033[0m\n");

    CRC = getBits(b, 0, 0, 32);
    out_SL_NL("CRC: ", CRC);

    return PRIVATE_SUBTABLE_OK;
}
#endif

static private_parse_status _cat_private_function(uint8_t *p_section_data, uint32_t len)
{
    unsigned char *data = p_section_data;
    int length = 0;
    if((NULL == data) || (((((data[1] & 0x0f) << 8) | data[2] ) + 3) != len))
    {
        app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
        return 0;
    }
    if(GxCore_MutexTrylock(s_cat_mutex) == GXCORE_SUCCESS)
    {
        if((data[0] == CAT_TID)
             && (s_AppCat.subt_id != -1))
        {
            app_log_info("\033[33m CAT, get section data!!\033[0m\n");
            length = ((data[1] & 0x0f) << 8) | data[2];
            length += 3;
            if (length <= CAT_DATA_LENGTH)
            {
                uint8_t *section_data = GxCore_Mallocz(sizeof(CatInfo));
                if(NULL != section_data)
                {
                    memcpy(section_data, data, length);
                    app_fifo_write(s_cat_fifo, (char *)section_data, length);
                    GxCore_Free(section_data);
                }
            }
        }
        GxCore_MutexUnlock(s_cat_mutex);
    }
    return PRIVATE_SUBTABLE_OK;
}

static int _cat_record(struct app_cat *cat)
{
    memcpy(&s_AppCat, cat, sizeof(struct app_cat));
    return 0;
}

static int _cat_config(struct app_cat *cat)
{
    GxSubTableDetail subt_detail = {0};
    GxMsgProperty_SiCreate  params_create = {0};
    GxMsgProperty_SiStart params_start = {0};

    if (s_AppCat.subt_id != -1)
    {
        //app_log_error("[%s] Cat has been started!\n", __func__);
        //return -2;
        app_cat_release();
    }

    if (cat == NULL || cat->pid != CAT_PID)
    {
        app_log_error("[%s] No params!\n", __func__);
        return -1;
    }

    subt_detail.ts_src = cat->ts_src;
    subt_detail.demux_id = cat->demuxid;
    app_demux_param_adjust(&subt_detail.ts_src, &subt_detail.demux_id, 0);

    app_emm_release_all();
    //app_cat_release();

    subt_detail.si_filter.pid = cat->pid;//0x1
    subt_detail.si_filter.match_depth = 1;
    subt_detail.si_filter.eq_or_neq = EQ_MATCH;
    subt_detail.si_filter.match[0] = CAT_TID;
    subt_detail.si_filter.mask[0] = 0xff;
    subt_detail.si_filter.crc = CRC_OFF;
    subt_detail.time_out = cat->timeout * 1000000;

    subt_detail.table_parse_cfg.mode = PARSE_PRIVATE_ONLY;//PARSE_SECTION_ONLY;
    subt_detail.table_parse_cfg.table_parse_fun = _cat_private_function;// NULL;

    params_create = &subt_detail;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void *)&params_create);

    cat->subt_id = subt_detail.si_subtable_id;
    cat->req_id = subt_detail.request_id;

    params_start = cat->subt_id;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void *)&params_start);

    APP_PRINT("---[app_cat_start][start]---\n");
    APP_PRINT("---[ts_src = %d][pid = 0x%x][subt_id = %d][req_id = %d]---\n", subt_detail.ts_src, subt_detail.si_filter.pid, cat->subt_id, cat->req_id);
    APP_PRINT("---[app_cat_start][end]---\n");

    cat->ver = 0xff;
    return 0;
}

int app_cat_start(struct app_cat *cat)
{
    if (0 == _cat_config(cat))
    {
        _cat_record(cat);
    }
    return 0;
}

int app_cat_restart(void)
{
    struct app_cat *cat = &s_AppCat;

    if (0 == _cat_config(cat))
    {
        _cat_record(cat);
    }
    return 0;
}

static int _cat_stop(void)
{
    struct app_cat *cat = &s_AppCat;
    GxSubTableDetail subt_detail = {0};
    GxMsgProperty_SiGet si_get = {0};
    GxMsgProperty_SiRelease params_release = (GxMsgProperty_SiRelease)cat->subt_id;

    if (cat->subt_id == -1)
    {
        app_log_error("[%s] Cat has been released!\n", __func__);
        return -1;
    }

    cat->ver = 0xff;

    APP_PRINT("---[app_cat_releasei id :%d]---\n", cat->subt_id);
    subt_detail.si_subtable_id = cat->subt_id;
    si_get = &subt_detail;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void *)&si_get);

    app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void *)&params_release);
    cat->subt_id = -1;

    return subt_detail.si_filter.pid;

}

int app_cat_release(void)
{
    return _cat_stop();
}
#if 0
int app_cat_get(GxMsgProperty_SiSubtableOk *data)
{
	int length = 0;
    if (data != NULL)
    {
        if (data->table_id == CAT_TID
                && data->si_subtable_id == s_AppCat.subt_id
                && data->request_id == s_AppCat.req_id)
        {
            app_log_info("\033[33m Get GXMSG_SI_SECTION_OK, Stop CAT!!\033[0m\n");
			length = ((data->parsed_data[1] & 0x0f) << 8) | data->parsed_data[2];
			length += 3;
			if (length <= CAT_DATA_LENGTH)
			{
				uint8_t *section_data = GxCore_Mallocz(sizeof(CatInfo));
				if(NULL != section_data)
				{
					memcpy(section_data, data->parsed_data, length);
					app_fifo_write(s_cat_fifo, (char *)section_data, length);
					GxCore_Free(section_data);
				}
			}
			GxBus_SiParserBufFree(data->parsed_data);
            return _cat_stop();
        }
    }

    return -1;
}
#endif
int app_cat_subtable_ok(GxMsgProperty_SiSubtableOk *data)
{
    if (data != NULL)
    {
        if (data->table_id == CAT_TID
                && data->si_subtable_id == s_AppCat.subt_id
                && data->request_id == s_AppCat.req_id)
        {
            app_log_info("\033[33m CAT, subtable ok, Stop CAT!!\033[0m\n");
            return _cat_stop();
        }
    }

    return -1;
}

static void app_cat_thread(void* arg)
{
    uint8_t *section_data = GxCore_Mallocz(sizeof(CatInfo));
	if(NULL == section_data)
	{
		app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
		return ;
	}
	GxCore_ThreadDetach();
    while(1)
    {
        if(app_fifo_read(s_cat_fifo, (char*)section_data, CAT_DATA_LENGTH) > 0)
        {
            g_AppCat.analyse(&g_AppCat, section_data);
        }
    }
	GxCore_Free(section_data);
}


// ts_src TS1-0 TS2-1 TS3-2...
static void app_cat_start_one(AppCat *cat)
{
    GxSubTableDetail subt_detail = {0};
    //GxMsgProperty_NodeByPosGet node;
    //GxMsgProperty_SiCreate  params_create;
    //GxMsgProperty_SiStart params_start;

    if(s_cat_mutex == -1)
    {
        GxCore_MutexCreate(&s_cat_mutex);
    }

    if(s_cat_fifo == -1)
    {
        app_fifo_create(&s_cat_fifo, sizeof(CatInfo),CAT_DATA_FIFO_NUM_);
        if(s_cat_thread_handle == -1)
        {
            GxCore_ThreadCreate("app_cat_proc", &s_cat_thread_handle, app_cat_thread, NULL, 10*1024, GXOS_DEFAULT_PRIORITY);
        }
    }

	GxCore_MutexLock(s_cat_mutex);
	app_cat_start(cat);
	GxCore_MutexUnlock(s_cat_mutex);




// recoder
    cat->demuxid= subt_detail.demux_id;
}


static status_t app_cat_analyse(AppCat *cat, uint8_t *section_data)
{

	/*  IS13818-1  S. 63 */
    /*  see also: ETS 468, ETR 289 */
    int len = 0;
    unsigned long CRC;

    if (!section_data)
    {
        return PRIVATE_SECTION_OK;
    }

    unsigned char *b = section_data;

    unsigned int table_id                 = b[0];
    unsigned int section_syntax_indicator = getBits(b, 0, 8, 1);
    unsigned int reserved_1               = getBits(b, 0, 10, 2);
    unsigned int section_length           = getBits(b, 0, 12, 12);
    unsigned int reserved_2               = getBits(b, 0, 24, 18);
    unsigned int version_number           = getBits(b, 0, 42, 5);
    unsigned int current_next_indicator   = getBits(b, 0, 47, 1);
    unsigned int section_number           = getBits(b, 0, 48, 8);
    unsigned int last_section_number      = getBits(b, 0, 56, 8);

    if (table_id != 0x01)
    {
        app_log_info("wrong Table ID");
        return PRIVATE_SECTION_OK;
    }

    app_log_info("CAT-decoding....\n");
    out_S2B_NL("Table_ID: ", table_id, dvbstrTableID(table_id));
    out_SB_NL("section_syntax_indicator: ", section_syntax_indicator);
    out_SB_NL("(fixed): ", 0);
    out_SB_NL("reserved_1: ", reserved_1);
    out_SW_NL("Section_length: ", section_length);
    out_SB_NL("reserved_2: ", reserved_2);
    out_SB_NL("Version_number: ", version_number);
    out_S2B_NL("current_next_indicator: ", current_next_indicator, dvbstrCurrentNextIndicator(current_next_indicator));
    out_SB_NL("Section_number: ", section_number);
    out_SB_NL("Last_Section_number: ", last_section_number);

    // buffer + header, len = len - header - CRC
    // Descriptor ISO 13818 - 2.6.1
    // - header - CRC
    len = section_length - 5;
    b  += 8;

    app_log_debug("\033[31m\n");
    while (len > 4)
    {
        int i = descriptor(b);
        app_log_debug("\n");
        len -= i;
        b += i;

    }
    app_log_debug("\033[0m\n");

    CRC = getBits(b, 0, 0, 32);
    out_SL_NL("CRC: ", CRC);

    return GXCORE_SUCCESS;
}


static uint32_t app_cat_stop(AppCat *cat)
{

    GxCore_MutexLock(s_cat_mutex);
    cat->ver = 0xff;
   	app_cat_release();
    GxCore_MutexUnlock(s_cat_mutex);
    return 0;
}



AppCat g_AppCat =
{
    .subt_id    = -1,
    .req_id     = 0,
    .ver        = 0xff,
    .start      = app_cat_start_one,
    .stop       = app_cat_stop,
    .analyse    = app_cat_analyse,
};



#endif
