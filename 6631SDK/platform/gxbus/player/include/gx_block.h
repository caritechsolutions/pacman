#ifndef GX_BLOCK_H
#define GX_BLOCK_H 1

/**
 * \file
 * This file implements functions and structures to handle blocks of data in vlc
 *
 */

/****************************************************************************
 * block:
 ****************************************************************************
 * - flags may not always be set (ie could be 0, even for a key frame
 *      it depends where you receive the buffer (before/after a packetizer
 *      and the demux/packetizer implementations.
 * - dts/pts could be 0, it means no pts
 * - length: length in microseond of the packet, can be null except in the
 *      sout where it is mandatory.
 * - rate 0 or a valid input rate, look at vlc_input.h
 *
 * - buffer_size number of valid data pointed by buffer
 *      you can freely decrease it but never increase it yourself
 *      (use GxBlock_Realloc)
 * - buffer: pointer over datas. You should never overwrite it, you can
 *   only incremment it to skip datas, in others cases use GxBlock_Realloc
 *   (don't duplicate yourself in a bigger buffer, GxBlock_Realloc is
 *   optimised for prehader/postdatas increase)
 ****************************************************************************/

/** The content doesn't follow the last block, or is probably broken */
#define BLOCK_FLAG_DISCONTINUITY 0x0001
/** Intra frame */
#define BLOCK_FLAG_TYPE_I        0x0002
/** Inter frame with backward reference only */
#define BLOCK_FLAG_TYPE_P        0x0004
/** Inter frame with backward and forward reference */
#define BLOCK_FLAG_TYPE_B        0x0008
/** For inter frame when you don't know the real type */
#define BLOCK_FLAG_TYPE_PB       0x0010
/** Warn that this block is a header one */
#define BLOCK_FLAG_HEADER        0x0020
/** This is the last block of the frame */
#define BLOCK_FLAG_END_OF_FRAME  0x0040
/** This is not a key frame for bitrate shaping */
#define BLOCK_FLAG_NO_KEYFRAME   0x0080
/** This block contains the last part of a sequence  */
#define BLOCK_FLAG_END_OF_SEQUENCE 0x0100
/** This block contains a clock reference */
#define BLOCK_FLAG_CLOCK         0x0200
/** This block is scrambled */
#define BLOCK_FLAG_SCRAMBLED     0x0400
/** This block has to be decoded but not be displayed */
#define BLOCK_FLAG_PREROLL       0x0800
/** This block is corrupted and/or there is data loss  */
#define BLOCK_FLAG_CORRUPTED     0x1000

/* These are for input core private usage only */
#define BLOCK_FLAG_CORE_PRIVATE_MASK  0x00ff0000
#define BLOCK_FLAG_CORE_PRIVATE_SHIFT 16

/* These are for module private usage only */
#define BLOCK_FLAG_PRIVATE_MASK  0xff000000
#define BLOCK_FLAG_PRIVATE_SHIFT 24

typedef void (*GxBlockFree) (GxBlock *);

struct GxBlock
{
    GxBlock     *next;
    GxBlock     *prev;

    uint32_t    flags;

    mtime_t     pts;
    mtime_t     dts;
    mtime_t     length;

    int         samples; /* Used for audio */
    int         rate;

    size_t      buffer_size;
    uint8_t     *buffer;

    /* Rudimentary support for overloading block (de)allocation. */
    GxBlockFree pf_release;
};

/****************************************************************************
 * Blocks functions:
 ****************************************************************************
 * - GxBlock_Alloc : create a new block with the requested size ( >= 0 ), return
 *      NULL for failure.
 * - GxBlock_Release : release a block allocated with GxBlock_Alloc.
 * - GxBlock_Realloc : realloc a block,
 *      pre: how many bytes to insert before body if > 0, else how many
 *      bytes of body to skip (the latter can be done without using
 *      GxBlock_Realloc buffer_size -= -i_pre, buffer += -i_pre as i_pre < 0)
 *      body (>= 0): the final size of the body (decreasing it can directly
 *      be done with buffer_size = body).
 *      with preheader and or body (increase
 *      and decrease are supported). Use it as it is optimised.
 * - GxBlock_Duplicate : create a copy of a block.
 ****************************************************************************/
void     GxBlock_Init    (GxBlock *block, void *data, size_t size);
GxBlock* GxBlock_Alloc   (size_t size);
GxBlock* GxBlock_Realloc (GxBlock *block, ssize_t pre, size_t body);

#define GxBlock_Create( dummy, size ) GxBlock_Alloc(size)

static inline GxBlock *GxBlock_Duplicate( GxBlock *p_block )
{
    GxBlock *clone = GxBlock_Alloc( p_block->buffer_size );
    if( clone == NULL )
        return NULL;

    clone->dts     = p_block->dts;
    clone->pts     = p_block->pts;
    clone->flags   = p_block->flags;
    clone->length  = p_block->length;
    clone->rate    = p_block->rate;
    clone->samples = p_block->samples;
    memcpy( clone->buffer, p_block->buffer, p_block->buffer_size );

    return clone;
}

static inline void GxBlock_Release( GxBlock *p_block )
{
    p_block->pf_release( p_block );
}

GxBlock* GxBlock_MmapAlloc(void *addr, size_t length);
GxBlock* GxBlock_File     (int fd);

/****************************************************************************
 * Chains of blocks functions helper
 ****************************************************************************
 * - GxBlock_ChainAppend : append a block to the last block of a chain. Try to
 *      avoid using with a lot of data as it's really slow, prefer
 *      GxBlock_ChainLastAppend
 * - GxBlock_ChainLastAppend : use a pointer over a pointer to the next blocks,
 *      and update it.
 * - GxBlock_ChainRelease : release a chain of block
 * - GxBlock_ChainExtract : extract data from a chain, return real bytes counts
 * - GxBlock_ChainGather : gather a chain, free it and return one block.
 ****************************************************************************/
static inline void GxBlock_ChainAppend( GxBlock **pp_list, GxBlock *p_block )
{
    if( *pp_list == NULL )
    {
        *pp_list = p_block;
    }
    else
    {
        GxBlock *p = *pp_list;

        while( p->p_next ) p = p->p_next;
        p->p_next = p_block;
    }
}

static inline void GxBlock_ChainLastAppend( GxBlock ***ppp_last, GxBlock *p_block )
{
    GxBlock *p_last = p_block;

    **ppp_last = p_block;

    while( p_last->p_next ) p_last = p_last->p_next;
    *ppp_last = &p_last->p_next;
}

static inline void GxBlock_ChainRelease( GxBlock *p_block )
{
    while( p_block )
    {
        GxBlock *p_next = p_block->p_next;
        GxBlock_Release( p_block );
        p_block = p_next;
    }
}

static size_t GxBlock_ChainExtract( GxBlock *p_list, void *p_data, size_t i_max )
{
    size_t  i_total = 0;
    uint8_t *p = (uint8_t*)p_data;

    while( p_list && i_max )
    {
        size_t i_copy = __MIN( i_max, p_list->buffer_size );
        memcpy( p, p_list->buffer, i_copy );
        i_max   -= i_copy;
        i_total += i_copy;
        p       += i_copy;

        p_list = p_list->p_next;
    }
    return i_total;
}

static inline GxBlock *GxBlock_ChainGather( GxBlock *p_list )
{
    size_t  i_total = 0;
    mtime_t length = 0;
    GxBlock *b, *g;

    if( p_list->p_next == NULL )
        return p_list;  /* Already gathered */

    for( b = p_list; b != NULL; b = b->p_next )
    {
        i_total += b->buffer_size;
        length += b->length;
    }

    g = GxBlock_Alloc( i_total );
    GxBlock_ChainExtract( p_list, g->buffer, g->buffer_size );

    g->flags = p_list->flags;
    g->pts   = p_list->pts;
    g->dts   = p_list->dts;
    g->length = length;

    /* free p_list */
    GxBlock_ChainRelease( p_list );
    return g;
}

/****************************************************************************
 * Fifos of blocks.
 ****************************************************************************
 * - GxBlockFifo_Create : create and init a new fifo
 * - GxBlockFifo_Release : destroy a fifo and free all blocks in it.
 * - GxBlockFifo_Empty : free all blocks in a fifo
 * - GxBlockFifo_Put : put a block
 * - GxBlockFifo_Get : get a packet from the fifo (and wait if it is empty)
 * - GxBlockFifo_Show : show the first packet of the fifo (and wait if
 *      needed), be carefull, you can use it ONLY if you are sure to be the
 *      only one getting data from the fifo.
 * - GxBlockFifo_Count : how many packets are waiting in the fifo
 * - GxBlockFifo_Size : how many cumulated bytes are waiting in the fifo
 * - GxBlockFifo_Wake : wake ups a thread with GxBlockFifo_Get() = NULL
 *   (this is used to wakeup a thread when there is no data to queue)
 ****************************************************************************/

GxBlockFifo* GxBlockFifo_Create      (void );
void         GxBlockFifo_Release  (GxBlockFifo *fifo);
void         GxBlockFifo_Empty    (GxBlockFifo *fifo);
size_t       GxBlockFifo_Put      (GxBlockFifo *fifo, GxBlock *block);
void         GxBlockFifo_Wake     (GxBlockFifo *fifo);
GxBlock*     GxBlockFifo_Get      (GxBlockFifo *fifo);
GxBlock*     GxBlockFifo_Show     (GxBlockFifo *fifo);
size_t       GxBlockFifo_Size     (const GxBlockFifo *fifo );
size_t       GxBlockFifo_Count    (const GxBlockFifo *fifo );

#endif /* GX_BLOCK_H */
