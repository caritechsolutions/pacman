#define FF_PAD_STRUCTURE(name, size, ...) \
struct ff_pad_helper_##name { __VA_ARGS__ }; \
typedef struct name { \
    __VA_ARGS__ \
    char reserved_padding[size - sizeof(struct ff_pad_helper_##name)]; \
} name;

FF_PAD_STRUCTURE(AVBPrint, 1024,
    char *str;         /**< string so far */
    unsigned len;      /**< length so far */
    unsigned size;     /**< allocated memory */
    unsigned size_max; /**< maximum allocated memory */
    char reserved_internal_buffer[1];
)

/**
 * Do not write anything to the buffer, only calculate the total length.
 *
 * The write operations can then possibly be repeated in a buffer with
 * exactly the necessary size (using `size_init = size_max = AVBPrint.len + 1`).
 */
#define AV_BPRINT_SIZE_COUNT_ONLY 0

int av_bprint_is_complete(const AVBPrint *buf);
int av_bprint_alloc(AVBPrint *buf, unsigned room);
void av_bprint_append_data(AVBPrint *buf, const char *data, unsigned size);
void av_bprint_init(AVBPrint *buf, unsigned size_init, unsigned size_max);
void av_bprint_init_for_buffer(AVBPrint *buf, char *buffer, unsigned size);
int av_bprint_finalize(AVBPrint *buf, char **ret_str);
void av_bprintf(AVBPrint *buf, const char *fmt, ...);


