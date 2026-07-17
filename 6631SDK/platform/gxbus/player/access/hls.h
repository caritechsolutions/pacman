#include "gx_common.h"
#include "gx_stream.h"

#define GX_HLS_PROBE_LEN 7
#define PROTOCOL_LEN 32

enum KeyType {
	KEY_NONE,
	KEY_AES_128,
};

enum PlaylistType {
	PLS_TYPE_UNSPECIFIED,
	PLS_TYPE_EVENT,
	PLS_TYPE_VOD
};


struct segment {
	uint8_t iv[16];
	int new_starttime;
	int duration;
	int64_t size;
	int64_t url_offset;
	enum KeyType key_type;
	char *url;
	char *key;
};

struct variant {
	int bandwidth;
	char *url;

	int finished;
	int target_duration;
	int totle_duration;
	int start_seq_no;
	int n_segments;
	int totle_segments;
	struct segment *segments;
	int needed, cur_needed;
	int cur_seq_no;

	char *key_url;
	uint8_t key[16];
	uint8_t iv[16];
	int  acodec_type;
	int  vcodec_type;
	int playlist_type;
};

typedef struct HLSContext {
	int n_variants;
	struct variant **variants;
	int cur_seq_no;
	int end_of_segment;
} HLSContext;


struct variant_info {
	char bandwidth[20];
	char codec_type[25];
};

struct key_info {
	char *uri;
	char method[10];
	char iv[35];
};

typedef void (*hls_parse_key_val_cb)(void* context, char *key, int key_len, char **dest, int *dest_len);

int hls_get_probe_len(void);
int hls_probe(const char* buf);
int hls_get_interval_byseq(HLSContext* c, struct variant* var, int seq, int only_cur);
int hls_get_time_byseq(HLSContext*c, struct variant* var, int seq);
int hls_get_seq_bytime(HLSContext*c, struct variant* var, int time);
int hls_get_base_time(HLSContext* c, struct variant *var, int seq);
int hls_get_playurl(GxStream* stream, HLSContext *c, struct variant *var, char* url, int64_t ms);
int hls_get_varurl(HLSContext* c, struct variant *var, char* url);
int hls_parse_playlist(HLSContext* hls, struct variant* var, char* url, char *hls_buf);
int stream_parse_playlist(HLSContext* hls, struct variant* var, char* url, char *hls_buf);
int hls_is_finished(HLSContext *c, struct variant *var);
int hls_is_new_data(HLSContext *c, struct variant *var);
HLSContext* hls_create(void);
void hls_destory(HLSContext* hls);
