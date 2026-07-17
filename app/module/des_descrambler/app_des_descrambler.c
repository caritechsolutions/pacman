#include "module/app_log.h"
#include "module/app_des_descrambler.h"
#include "gxsecure/gxtfm_api.h"
#include "gxsecure/gxsecure_api.h"


//#define DES_DEBUG_ENABLE
#ifdef  DES_DEBUG_ENABLE
#define DES_PRINTF(...) app_log_debug( __VA_ARGS__ )
#else
#define DES_PRINTF(...) do{}while(0)
#endif

#define MTC_PTHREAD_PRIORITY (GXOS_DEFAULT_PRIORITY-1)
#define MTC_BUFFER_SIZE (16*1024)
#define MTC_KEY_SIZE (16)
#define MAX_PKT_NUM (300)

unsigned char in_mtc_video[MTC_BUFFER_SIZE] __attribute__((aligned(2048))) = {0};
unsigned char in_mtc_audio[MTC_BUFFER_SIZE] __attribute__((aligned(2048))) = {0};
unsigned char out_mtc[MTC_BUFFER_SIZE] __attribute__((aligned(2048))) = {0};
unsigned char video_key[MTC_KEY_SIZE] __attribute__((aligned(8))) = {0};
unsigned char audio_key[MTC_KEY_SIZE] __attribute__((aligned(8))) = {0};

struct mtc
{
    unsigned char *ts;
    unsigned int pktnum;
    unsigned char *out;
    unsigned char update;
    unsigned char oest; //even odd status

    unsigned char *vbuf;
    unsigned int vstart;
    unsigned int vend;
    unsigned char *vkey;
    unsigned char *vpos;
    unsigned int vsize;

    unsigned char *abuf;
    unsigned int astart;
    unsigned int aend;
    unsigned char *akey;
    unsigned char *apos;
    unsigned int asize;
};

static PlayerStreamPortCallback tsbuff_stream_outside;
static int thiz_key_mutex = -1;
static int thiz_key_type = -1;
static unsigned char thiz_video_key[MTC_KEY_SIZE] __attribute__((aligned(8))) = {0};
static unsigned char thiz_audio_key[MTC_KEY_SIZE] __attribute__((aligned(8))) = {0};

static inline int app_tsd_transport_packet_analyse(unsigned char *data, int *pPos, int *pSize,
        unsigned char *pTsc,
        unsigned short *pPid, unsigned char *pPusi) __attribute__((always_inline));
static inline int app_tsd_transport_packet_analyse(unsigned char *data, int *pPos, int *pSize,
        unsigned char *pTsc,
        unsigned short *pPid, unsigned char *pPusi)
{
    unsigned int af_size;
    int payload_size = 0;
    int pos = 0;

    unsigned char sync_byte = data[0];
    unsigned short pid = ((data[1] & 0x1F) << 8) | (data[2] & 0xff);
    unsigned char scrambling_ctrl = (data[3] & 0xC0) >> 6;
    unsigned char adaptation_field = (data[3] & 0x30) >> 4;
    unsigned char payload_start = (data[1] & 0x40) >> 6;

    if (0x47 != sync_byte)
    {
        return -1;
    }

    switch (adaptation_field)
    {
        case M2TS_ADAPTATION_AND_PAYLOAD:
            af_size = data[4];

            if (af_size > 183)
            {
                app_log_error("error in af_size=%d\n", af_size);
                goto exit;
            }

            pos = 5 + af_size;
            payload_size = TS_MAXLEN - pos;
            adaptation_field = 1;
            break;

        case M2TS_ADAPTATION_ONLY:
            af_size = data[4];

            if (af_size > 183)
            {
                app_log_error("error in af_size=%d\n", af_size);
                goto exit;
            }

            pos = TS_MAXLEN;
            payload_size = TS_MAXLEN - pos;
            adaptation_field = 1;
            break;

        case M2TS_ADAPTATION_NONE:
            pos = 4;
            payload_size = TS_MAXLEN - pos;
            adaptation_field = 0;
            goto exit;

        case M2TS_ADAPTATION_RESERVED:
        default:
            pos = 4;
            payload_size = TS_MAXLEN - pos;
            adaptation_field = 0;
            goto exit;
    }

exit:

    *pPos = pos;
    *pSize = payload_size;
    *pTsc = scrambling_ctrl;
    *pPid = pid;
    *pPusi = payload_start;

    return 0;
}

static inline void _mtc_copy_to_ts(unsigned char *ts, unsigned char *mtc, unsigned int start,
                                   unsigned int end, unsigned char *pPos)
{
    unsigned char *src = NULL;
    unsigned char *dst = NULL;
    unsigned int i = 0;
    unsigned int need_to_copy_num = 0;

    for (i = start; i < end; i++)
    {
        if (i == start)
        {
            src = (unsigned char *)(mtc);
        }

        need_to_copy_num = ((TS_MAXLEN - pPos[i]) / 8) * 8;

        if (pPos[i] == 188)
        {
            dst = (unsigned char *)(ts + (i * TS_MAXLEN));
        }
        else
        {
            dst = (unsigned char *)(ts + (i * TS_MAXLEN) + pPos[i]);
        }

        if (need_to_copy_num > 0)
        {
            memcpy(dst, src, need_to_copy_num);
        }

        src += need_to_copy_num;
    }
}

static inline int _mtc_decrypt_data(struct mtc *pmtc, unsigned char oe, unsigned char va)
{
    GxTfmCrypto mtc_data = {0};

    int ret = -1;
    memset(&mtc_data, 0, sizeof(GxTfmCrypto));
    mtc_data.module = TFM_MOD_M2M;
    mtc_data.output.buf = (unsigned char *)pmtc->out;
    mtc_data.output.length = MTC_BUFFER_SIZE;
    mtc_data.even_key.id = TFM_KEY_ACPU_SOFT;
    mtc_data.alg = TFM_ALG_DES;
    mtc_data.opt = TFM_OPT_ECB;
    mtc_data.flags = TFM_FLAG_CRYPT_EVEN_KEY_VALID;


    if (va == 0) //video
    {
        mtc_data.input.buf = (unsigned char *)pmtc->vbuf;
        mtc_data.input.length = pmtc->vsize;
        mtc_data.soft_key_len = 8;
        if (oe == 0) //even
        {
            memcpy(&mtc_data.soft_key[0], (unsigned char *)(pmtc->vkey), mtc_data.soft_key_len);
            ret = GxTfm_Decrypt(&mtc_data);
        }
        else
        {
            memcpy(&mtc_data.soft_key[0], (unsigned char *)(pmtc->vkey+8), mtc_data.soft_key_len);
            ret = GxTfm_Decrypt(&mtc_data);
        }

        if (ret == 0)
        {
            _mtc_copy_to_ts(pmtc->ts, pmtc->out, pmtc->vstart, pmtc->vend, pmtc->vpos);
        }else {
            app_log_error("%s:%d GxTfm_Decrypt return err\n", __func__, __LINE__);
        }
    }
    else //audio
    {
        mtc_data.input.buf = (unsigned char *)pmtc->abuf;
        mtc_data.input.length = pmtc->asize;
        mtc_data.soft_key_len = 8;
        if (oe == 0) //even
        {
            memcpy(&mtc_data.soft_key[0], (unsigned char *)(pmtc->akey), mtc_data.soft_key_len);
            ret = GxTfm_Decrypt(&mtc_data);
        }
        else
        {
            memcpy(&mtc_data.soft_key[0], (unsigned char *)(pmtc->akey+8), mtc_data.soft_key_len);
            ret = GxTfm_Decrypt(&mtc_data);
        }

        if (ret == 0)
        {
            _mtc_copy_to_ts(pmtc->ts, pmtc->out, pmtc->astart, pmtc->aend, pmtc->apos);
        }else {
            app_log_error("%s:%d GxTfm_Decrypt return err\n", __func__, __LINE__);
        }
    }

    return ret;
}

static void _mtc_data(struct mtc *pmtc)
{
    unsigned char need_update = pmtc->update;

    if (need_update != DES_NO_DATA)
    {
        if (0 == need_update % 2) //even
        {
            if ((need_update == DES_VIDEO_EVEN_DATA || need_update == DES_ALL_EVEN_DATA) && (pmtc->vsize != 0))
            {
                _mtc_decrypt_data(pmtc, 0, 0);
            }

            if ((need_update == DES_AUDIO_EVEN_DATA || need_update == DES_ALL_EVEN_DATA) && (pmtc->asize != 0))
            {
                _mtc_decrypt_data(pmtc, 0, 1);
            }
        }
        else if (1 == need_update % 2) //odd
        {
            if ((need_update == DES_VIDEO_ODD_DATA || need_update == DES_ALL_ODD_DATA) && (pmtc->vsize != 0))
            {
                _mtc_decrypt_data(pmtc, 1, 0);
            }

            if ((need_update == DES_AUDIO_ODD_DATA || need_update == DES_ALL_ODD_DATA) && (pmtc->asize != 0))
            {
                _mtc_decrypt_data(pmtc, 1, 1);
            }
        }

        if (need_update == DES_VIDEO_ODD_DATA
                || need_update == DES_VIDEO_EVEN_DATA
                || need_update == DES_ALL_ODD_DATA
                || need_update == DES_ALL_EVEN_DATA)
        {
            pmtc->vstart = pmtc->vend;
            pmtc->vsize = 0;
        }

        if (need_update == DES_AUDIO_ODD_DATA
                || need_update == DES_AUDIO_EVEN_DATA
                || need_update == DES_ALL_ODD_DATA
                || need_update == DES_ALL_EVEN_DATA)
        {
            pmtc->astart = pmtc->aend;
            pmtc->asize = 0;
        }

        pmtc->update = DES_NO_DATA;
    }
}

static void app_tsd_des_descrambler(unsigned char *pData, unsigned int size, unsigned short vpid,
                                    unsigned short apid)
{
    static struct mtc mtcst = {0};
    static unsigned int s_odd_even = 0;

    unsigned int i = 0;
    int payload_pos = 0;
    int payload_size = 0;
    unsigned char transport_scrambling_control = 0;
    unsigned short pid = 0;
    unsigned char pusi = 0;

    unsigned char video_pos[MAX_PKT_NUM] = {0};
    unsigned char audio_pos[MAX_PKT_NUM] = {0};

    memset(&mtcst, 0, sizeof(struct mtc));

    mtcst.ts = pData;
    mtcst.pktnum = size / TS_MAXLEN;
    mtcst.out = out_mtc;
    mtcst.update = DES_NO_DATA;
    mtcst.oest = s_odd_even;

    mtcst.vbuf = in_mtc_video;
    mtcst.vstart = mtcst.vend = 0;
    mtcst.vkey = video_key;
    mtcst.vpos = video_pos;
    mtcst.vsize = 0;

    mtcst.abuf = in_mtc_audio;
    mtcst.astart = mtcst.aend = 0;
    mtcst.akey = audio_key;
    mtcst.apos = audio_pos;
    mtcst.asize = 0;

    for (i = 0; i < mtcst.pktnum; i++)
    {
        app_tsd_transport_packet_analyse((unsigned char *)(pData + (i * TS_MAXLEN)),
                                         &payload_pos,
                                         &payload_size,
                                         &transport_scrambling_control,
                                         &pid,
                                         &pusi);

        if (!((pid == vpid || pid == apid) && (transport_scrambling_control != 0)))
        {
            //Just deal with encrypted audio and video data
            mtcst.vpos[i] = TS_MAXLEN;
            mtcst.apos[i] = TS_MAXLEN;
            continue;
        }

        (pData + (i * TS_MAXLEN))[3] &= 0x3F;

        if (payload_size != 0)
        {
            GxCore_MutexLock(thiz_key_mutex);

            if (thiz_key_type == DES_KEY_EVEN)
            {
                memcpy(&video_key[0], &thiz_video_key[0], sizeof(video_key) / 2);
                memcpy(&audio_key[0], &thiz_audio_key[0], sizeof(audio_key) / 2);
            }
            else
            {
                memcpy(&video_key[0] + 8, &thiz_video_key[0] + 8, sizeof(video_key) / 2);
                memcpy(&audio_key[0] + 8, &thiz_audio_key[0] + 8, sizeof(audio_key) / 2);
            }

            GxCore_MutexUnlock(thiz_key_mutex);

            if ((s_odd_even != transport_scrambling_control))
            {
                if (mtcst.vsize != 0)
                {
                    mtcst.update = (s_odd_even == 0x2) ? DES_VIDEO_EVEN_DATA : DES_VIDEO_ODD_DATA;
                    mtcst.vend = i;
                }

                if (mtcst.asize != 0)
                {
                    if (mtcst.update == 0)
                    {
                        mtcst.update = (s_odd_even == 0x2) ? DES_AUDIO_EVEN_DATA : DES_AUDIO_ODD_DATA;
                    }
                    else
                    {
                        mtcst.update += 2;
                    }

                    mtcst.aend = i;
                }

                if (mtcst.update != DES_NO_DATA)
                {
                    _mtc_data(&mtcst);
                }

                s_odd_even = transport_scrambling_control;
                mtcst.oest = s_odd_even;
            }

            unsigned int need_to_copy = (payload_size / 8) * 8;

            if (vpid == pid) //video
            {
                unsigned char *vdst = (unsigned char *)(mtcst.vbuf + mtcst.vsize);
                unsigned char *vsrc = (unsigned char *)(mtcst.ts + (i * TS_MAXLEN) + (payload_pos));

                memcpy(vdst, vsrc, need_to_copy);
                mtcst.vsize += need_to_copy;
                mtcst.vpos[i] = payload_pos;
                mtcst.apos[i] = TS_MAXLEN;

                if (MTC_BUFFER_SIZE - mtcst.vsize < TS_MAXLEN)
                {
                    mtcst.update = (s_odd_even == 0x2) ? DES_VIDEO_EVEN_DATA : DES_VIDEO_ODD_DATA;
                    mtcst.vend = i + 1;
                }
            }
            else if (apid == pid) //audio
            {
                unsigned char *adst = (unsigned char *)(mtcst.abuf + mtcst.asize);
                unsigned char *asrc = (unsigned char *)(mtcst.ts + (i * TS_MAXLEN) + (payload_pos));

                memcpy(adst, asrc, need_to_copy);
                mtcst.asize += need_to_copy;
                mtcst.apos[i] = payload_pos;
                mtcst.vpos[i] = TS_MAXLEN;

                if (MTC_BUFFER_SIZE - mtcst.asize < TS_MAXLEN)
                {
                    mtcst.update = (s_odd_even == 0x2) ? DES_AUDIO_EVEN_DATA : DES_AUDIO_ODD_DATA;
                    mtcst.aend = i + 1;
                }
            }
            else
            {
                mtcst.vpos[i] = TS_MAXLEN;
                mtcst.apos[i] = TS_MAXLEN;
            }

            if (mtcst.update != DES_NO_DATA)
            {
                _mtc_data(&mtcst);
            }
        }
        else
        {
            mtcst.vpos[i] = TS_MAXLEN;
            mtcst.apos[i] = TS_MAXLEN;
        }
    }

    if (mtcst.vstart != mtcst.pktnum)
    {
        mtcst.update = (s_odd_even == 0x2) ? DES_VIDEO_EVEN_DATA : DES_VIDEO_ODD_DATA;
        mtcst.vend = i;
    }

    if (mtcst.astart != mtcst.pktnum)
    {
        if (mtcst.update == 0)
        {
            mtcst.update = (s_odd_even == 0x2) ? DES_AUDIO_EVEN_DATA : DES_AUDIO_ODD_DATA;
        }
        else
        {
            mtcst.update += 2;
        }

        mtcst.aend = i;
    }

    if (mtcst.update != DES_NO_DATA)
    {
        _mtc_data(&mtcst);
    }

    return;
}

static int des_vpid = -1;
static int des_apid = -1;

static unsigned short app_des_get_apid(void)
{
    return des_apid;
}

static unsigned short app_des_get_vpid(void)
{
    return des_vpid;
}

static unsigned int app_tsd_current_timems(void)
{
    GxTime Time;

    GxCore_GetTickTime(&Time);

    return Time.seconds * 1000 + Time.microsecs / 1000;
}

static handle_t app_tsd_stream_init(size_t blksize, unsigned int delayms)
{
    int ret;
    TSBuffStreamPortEx *priv = GxCore_Mallocz(sizeof(TSBuffStreamPortEx));

    if (priv == NULL)
    {
        return 0;
    }

    priv->blksize = blksize;
    priv->delayms = delayms;
    priv->cache2mem = 0x1;
    priv->cachesize = 0x400000;

    app_log_debug("\n\033[31m[Des Init]: 0x%x, %d, 0x%x\n\033[0m", priv->cachesize, priv->cache2mem,
           priv->blksize);

    if (priv->cache2mem)
    {
        ret = app_des_cache_init(&priv->cache, priv->blksize,
                                 priv->cachesize / priv->blksize, priv->cachesize * 3 / priv->blksize / 4);
        TSD_CHECK_RET(ret);
    }

    priv->running = 1;

    GxCore_MutexCreate(&thiz_key_mutex);

    return (handle_t)priv;
ERR:
    GxCore_Free(priv);
    return 0;
}

static void app_tsd_stream_uninit(handle_t handle)
{
    TSBuffStreamPortEx *priv = (TSBuffStreamPortEx *)handle;

    if (priv->cache2mem)
    {
        app_des_cache_uninit(&priv->cache);
    }

    GxCore_MutexDelete(thiz_key_mutex);
}

static void app_tsd_stream_exit(handle_t handle)
{
    TSBuffStreamPortEx *priv = (TSBuffStreamPortEx *)handle;

    priv->running = 0;

    if (priv->cache2mem)
    {
        app_des_cache_exit(&priv->cache);
    }
}

static int app_tsd_stream_new_packet(handle_t handle, GxPlayerDataPacket *pkt, ssize_t size)
{
    TSBuffStreamPortEx *priv = (TSBuffStreamPortEx *)handle;

    if (priv->cache2mem)
    {
        struct cachepage *page = app_des_cache_alloc_page(&priv->cache);

        if (page == NULL)
        {
            return -1;
        }

        priv->cur_page_r = page;
        pkt->data = page->buffer;
        pkt->size = page->size;
    }

    return 0;
}

static void app_tsd_stream_del_packet(handle_t handle, GxPlayerDataPacket *pkt)
{
    TSBuffStreamPortEx *priv = (TSBuffStreamPortEx *)handle;

    if (pkt && pkt->data)
    {
        if (priv->cache2mem)
        {
            if (priv->cur_page_r && pkt->data == priv->cur_page_r->buffer)
            {
                app_des_cache_free_page(&priv->cache, priv->cur_page_r);
                priv->cur_page_r = NULL;
            }
            else if (priv->cur_page_w && pkt->data == priv->cur_page_w->buffer)
            {
                app_des_cache_free_page(&priv->cache, priv->cur_page_w);
                priv->cur_page_w = NULL;
            }
        }
    }

    memset(pkt, 0, sizeof(GxPlayerDataPacket));
}

static int app_tsd_stream_get_packet(handle_t handle, GxPlayerDataPacket *pkt)
{
    unsigned int pts = 0;
    int32_t delayms;
    TSBuffStreamPortEx *priv = (TSBuffStreamPortEx *)handle;

    if (priv->cache2mem)
    {
        struct cachepage *page = app_des_cache_get_page(&priv->cache);

        if (page == NULL)
        {
            return -1;
        }

        priv->cur_page_w = page;
        pkt->data = page->buffer;
        pkt->size = page->size;
        pts = page->pts;
    }

    while (priv->running)
    {
        delayms = app_tsd_current_timems() - pts - priv->delayms;

        if (delayms >= -10)
        {
            break;
        }

        GxCore_ThreadDelay(10);
    }

    return 0;
}

static int app_tsd_stream_put_packet(handle_t handle, GxPlayerDataPacket *pkt)
{
    TSBuffStreamPortEx *priv = (TSBuffStreamPortEx *)handle;

    if (priv->cache2mem)
    {
        priv->cur_page_r->size = pkt->size;
        priv->cur_page_r->pts = app_tsd_current_timems();
        app_tsd_des_descrambler(pkt->data, pkt->size, app_des_get_vpid(), app_des_get_apid());
        app_des_cache_put_page(&priv->cache, priv->cur_page_r);
    }

    return 0;
}

static PlayerStreamPortCallback tsbuff_stream_outside =
{
    .Init = app_tsd_stream_init,
    .Uninit = app_tsd_stream_uninit,
    .Exit = app_tsd_stream_exit,
    .NewPacket = app_tsd_stream_new_packet,
    .DelPacket = app_tsd_stream_del_packet,
    .GetPacket = app_tsd_stream_get_packet,
    .PutPacket = app_tsd_stream_put_packet,
};

int app_des_set_key(unsigned char *cw, unsigned char type)
{
    GxCore_MutexLock(thiz_key_mutex);
    thiz_key_type = type;
    memcpy(&thiz_video_key[0], (unsigned char *)cw, sizeof(thiz_video_key));
    memcpy(&thiz_audio_key[0], (unsigned char *)(cw + 16), sizeof(thiz_audio_key));
    GxCore_MutexUnlock(thiz_key_mutex);

    return 0;
}

int app_des_set_pid(unsigned short vpid, unsigned short apid)
{
    des_vpid = vpid;
    des_apid = apid;
    return 0;
}

int app_des_get_pid(unsigned short *vpid, unsigned short *apid)
{
    if((NULL == vpid) || (NULL == apid))
    {
        return -1;
    }
    *vpid = des_vpid;
    *apid = des_apid;
    return 0;
}

int app_des_enable(void)
{
    tsbuff_stream_outside.Init = app_tsd_stream_init;
    tsbuff_stream_outside.Uninit = app_tsd_stream_uninit;
    tsbuff_stream_outside.Exit = app_tsd_stream_exit;
    tsbuff_stream_outside.NewPacket = app_tsd_stream_new_packet;
    tsbuff_stream_outside.DelPacket = app_tsd_stream_del_packet;
    tsbuff_stream_outside.GetPacket = app_tsd_stream_get_packet;
    tsbuff_stream_outside.PutPacket = app_tsd_stream_put_packet;

    GxPlayer_SetStreamCallback(&tsbuff_stream_outside);
    return 0;
}

int app_des_disable(void)
{
    des_vpid = -1;
    des_apid = -1;

    tsbuff_stream_outside.Init = NULL;
    tsbuff_stream_outside.Uninit = NULL;
    tsbuff_stream_outside.Exit = NULL;
    tsbuff_stream_outside.NewPacket = NULL;
    tsbuff_stream_outside.DelPacket = NULL;
    tsbuff_stream_outside.GetPacket = NULL;
    tsbuff_stream_outside.PutPacket = NULL;

    GxPlayer_SetStreamCallback(&tsbuff_stream_outside);
    return 0;
}
