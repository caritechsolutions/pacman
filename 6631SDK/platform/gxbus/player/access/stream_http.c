#include "gx_demux.h"
#include "stream_m3u8.h"
#include "http.h"
#include "gx_url.h"
#include "avstring.h"
#include "../avutil/dict.h"

#define STREAM_HTTP_PVR_DEBUG 0
#if STREAM_HTTP_PVR_DEBUG
static FILE* stream_http_pvr_fd = NULL;
static char* stream_http_pvr_file = "/media/sda1/net_pvr/1-http.flv";
#endif
#ifndef MIN
#define MIN(a,b) ((a)>(b)?(b):(a))
#endif
extern GxMimeStruct GxMime_Type_Table[];

extern char *av_base64_encode(char *out, int out_size, const uint8_t *in, int in_size);
typedef struct {
	unsigned metaint;
	unsigned metapos;
	int is_ultravox;
} GxScastData;

static int http_interrupt(GxStreamHttp* http_stream)
{
	if(http_stream->parent.interrupt_cbk &&
			http_stream->parent.interrupt_cbk()) {
		return 1;
	}
	return 0;
}

/**
 *  \brief first read any data from sc->buffer then from fd
 *  \param fd file descriptor to read data from
 *  \param buffer buffer to read into
 *  \param len how many bytes to read
 *  \param sc streaming control containing buffer to read from first
 *  \return len unless there is a read error or eof
 */
static unsigned int my_read(int fd, unsigned char* buffer, int len, GxStreamingCtrl * sc)
{
	unsigned pos = 0;
	unsigned cp_len = sc->buffer_size - sc->buffer_pos;
	if (cp_len > len)
		cp_len = len;
	memcpy(buffer, &sc->buffer[sc->buffer_pos], cp_len);
	sc->buffer_pos += cp_len;
	pos += cp_len;
	while (pos < len) {
		int ret = recv(fd, &buffer[pos], len - pos, 0);
		if (ret <= 0)
			break;
		pos += ret;
	}
	return pos;
}

/**
 *  \brief read and process (i.e. discard *g*) a block of ultravox metadata
 *  \param fd file descriptor to read from
 *  \param sc streaming_ctrl_t whose buffer is consumed before reading from fd
 *  \return number of real data before next metadata block starts or 0 on error
 */
static unsigned uvox_meta_read(int fd, GxStreamingCtrl*  sc)
{
	unsigned metaint;
	unsigned char info[6] = { 0, 0, 0, 0, 0, 0 };
	int info_read;

	do {
		info_read = my_read(fd, info, 1, sc);
		if (info[0] == 0x00)
			info_read = my_read(fd, info, 6, sc);
		else
			info_read += my_read(fd, &info[1], 5, sc);
		if (info_read != 6)	// read error or eof
			return 0;
		// sync byte and reserved flags
		if (info[0] != 0x5a || (info[1] & 0xfc) != 0x00) {
			gxlogf("Invalid or unknown uvox metadata\n");
			return 0;
		}
		if (info[1] & 0x01)
			gxlogf("Encrypted ultravox data\n");
		metaint = info[4] << 8 | info[5];
		if ((info[3] & 0xf) < 0x07) {	// discard any metadata nonsense
			unsigned char* metabuf = (unsigned char *)av_malloc(metaint);
			my_read(fd, metabuf, metaint, sc);
			av_free(metabuf);
		}
	} while ((info[3] & 0xf) < 0x07);
	return metaint;
}

/**
 *  \brief read one scast meta data entry and print it
 *  \param fd file descriptor to read from
 *  \param sc streaming_ctrl_t whose buffer is consumed before reading from fd
 */
static void scast_meta_read(int fd, GxStreamingCtrl*  sc)
{
	int i;
	unsigned char tmp = 0;
	unsigned metalen;
	my_read(fd, &tmp, 1, sc);
	metalen = tmp*  16;
	if (metalen > 0) {
		unsigned char* info = (unsigned char *)av_malloc(metalen + 1);
		unsigned int nlen = my_read(fd, info, metalen, sc);
		for (i = 0; i < nlen; i++)
			if (info[i] && info[i] < 32) info[i] = '?';
		info[nlen] = 0;
		gxlogf("\nICY Info: %s\n", info);
		av_free(info);
	}
}

/**
 *  \brief read data from scast/ultravox stream without any metadata
 *  \param fd file descriptor to read from
 *  \param buffer buffer to read data into
 *  \param size number of bytes to read
 *  \param sc streaming_ctrl_t whose buffer is consumed before reading from fd
 */
static int scast_streaming_read(int fd, uint8_t*  buffer, size_t size, GxStreamingCtrl * sc)
{
	GxScastData* sd = (GxScastData *) (sc->data);
	unsigned int block, ret;
	unsigned int done = 0;

	// first read remaining data up to next metadata
	block = sd->metaint - sd->metapos;
	if (block > size)
		block = size;
	ret = my_read(fd, buffer, block, sc);
	sd->metapos += ret;
	done += ret;
	if (ret != block)	// read problems or eof
		size = done;

	while (done < size) {	// now comes the metadata
		if (sd->is_ultravox) {
			sd->metaint = uvox_meta_read(fd, sc);
			if (!sd->metaint)
				size = done;
		} else
			scast_meta_read(fd, sc);	// read and display metadata
		sd->metapos = 0;
		block = size - done;
		if (block > sd->metaint)
			block = sd->metaint;
		ret = my_read(fd, &buffer[done], block, sc);
		sd->metapos += ret;
		done += ret;
		if (ret != block)	// read problems or eof
			size = done;
	}
	return done;
}

static int scast_streaming_start(GxStream*  http_stream)
{
	int metaint, stream_buffer_size = 0;
	GxScastData* scast_data;
	GxStreamHttp* stream = (GxStreamHttp *) http_stream;

	GxHttpHeader* http_hdr = stream->streaming_ctrl->data;
	int is_ultravox = strcasecmp(stream->streaming_ctrl->url->protocol, "unsv") == 0;
	if (!stream || http_stream->fd < 0 || !http_hdr)
		return -1;

	GxPlayer_SystemGet(PSYS_STREAM_BUFFER_SIZE, &stream_buffer_size);

	if (is_ultravox)
		metaint = 0;
	else {
		metaint = atoi(http_get_field(http_hdr, "Icy-MetaInt"));
		if (metaint <= 0)
			return -1;
	}
	stream->streaming_ctrl->buffer = av_malloc(http_hdr->body_size);
	stream->streaming_ctrl->buffer_size = http_hdr->body_size;
	stream->streaming_ctrl->buffer_pos = 0;
	memcpy(stream->streaming_ctrl->buffer, http_hdr->body, http_hdr->body_size);
	scast_data = av_malloc(sizeof(GxScastData));
	scast_data->metaint = metaint;
	scast_data->metapos = 0;
	scast_data->is_ultravox = is_ultravox;
	http_freep(&http_hdr);
	stream->streaming_ctrl->data = scast_data;
	stream->streaming_ctrl->streaming_read = scast_streaming_read;
	stream->streaming_ctrl->streaming_seek = NULL;
	stream->streaming_ctrl->prebuffer_size = stream_buffer_size;
	stream->streaming_ctrl->buffering = 1;
	stream->streaming_ctrl->status = streaming_playing_e;

	return 0;
}

static int nop_streaming_start(GxStream*  http_stream)
{
	GxHttpHeader* http_hdr = NULL;
	char* next_url = NULL;
	GxURL* rd_url = NULL;
	int fd, ret, stream_buffer_size= 0;
	GxStreamHttp* stream = (GxStreamHttp *) http_stream;

	if (stream == NULL)
		return -1;

	fd = http_stream->fd;
	if (fd < 0) {
		fd = http_send_request(stream, stream->streaming_ctrl->url, 0, stream->parent.interrupt_cbk);
		if (fd < 0)
			return -1;
		http_freep(&http_hdr);
		http_hdr = http_read_response(fd, stream->parent.interrupt_cbk);
		if (http_hdr == NULL)
			return -1;

		switch (http_hdr->status_code) {
		case 200:	// OK
		case 206:	// OK
			gxlogf("Content-Type: [%s]\n", http_get_field(http_hdr, "Content-Type"));
			gxlogf("Content-Length: [%s]\n", http_get_field(http_hdr, "Content-Length"));
			if (http_hdr->body_size > 0) {
				if (streaming_bufferize(stream->streaming_ctrl, http_hdr->body, http_hdr->body_size) <
						0) {
					http_freep(&http_hdr);
					return -1;
				}
			}
			break;
			// Redirect
		case 301:	// Permanently
		case 302:	// Temporarily
		case 303:	// See Other
		case 307: // Temporarily (since HTTP/1.1)
			ret = -1;
			next_url = http_get_field(http_hdr, "Location");

			if (next_url != NULL)
				rd_url = url_new(next_url);

			if (next_url != NULL && rd_url != NULL) {
				gxlogf("Redirected: Using this url instead %s\n", next_url);
				stream->streaming_ctrl->url = check4proxies(rd_url);

				ret = nop_streaming_start(http_stream);	//recursively get streaming started
			} else {
				gxlogf("Redirection failed\n");
				closesocket(fd);
				fd = -1;
			}
			return ret;
			break;
		case 401:	//Authorization required
		case 403:	//Forbidden
		case 404:	//Not found
		case 500:	//Server Error
		default:
			gxlogf("Server returned code %d: %s\n", http_hdr->status_code, http_hdr->reason_phrase);
			closesocket(fd);
			fd = -1;
			return -1;
			break;
		}
		http_stream->fd = fd;
	} else {
		http_hdr = (GxHttpHeader* ) stream->streaming_ctrl->data;
		if (http_hdr->body_size > 0) {
			if (streaming_bufferize(stream->streaming_ctrl, http_hdr->body, http_hdr->body_size) < 0) {
				http_freep(&http_hdr);
				stream->streaming_ctrl->data = NULL;
				return -1;
			}
		}
	}

	if (http_hdr) {
		http_freep(&http_hdr);
		stream->streaming_ctrl->data = NULL;
	}

	GxPlayer_SystemGet(PSYS_STREAM_BUFFER_SIZE, &stream_buffer_size);

	stream->streaming_ctrl->streaming_read = nop_streaming_read;
	stream->streaming_ctrl->streaming_seek = NULL;
	stream->streaming_ctrl->prebuffer_size = stream_buffer_size;
	stream->streaming_ctrl->buffering = 1;
	stream->streaming_ctrl->status = streaming_playing_e;
	return 0;
}

GxHttpHeader* http_new_header(void)
{
	GxHttpHeader* http_hdr;

	http_hdr = av_mallocz(sizeof(GxHttpHeader));
	if (http_hdr == NULL)
		return NULL;

	return http_hdr;
}

void http_freep(GxHttpHeader** http_hdr)
{
	http_free(*http_hdr);
	*http_hdr = NULL;
}

void http_free(GxHttpHeader* http_hdr)
{
	GxHttpField* field, *field2free;
	if (http_hdr == NULL)
		return;
	if (http_hdr->protocol != NULL)
		av_free(http_hdr->protocol);
	if (http_hdr->uri != NULL)
		av_free(http_hdr->uri);
	if (http_hdr->reason_phrase != NULL)
		av_free(http_hdr->reason_phrase);
	if (http_hdr->field_search != NULL)
		av_free(http_hdr->field_search);
	if (http_hdr->method != NULL)
		av_free(http_hdr->method);
	if (http_hdr->buffer != NULL)
		av_free(http_hdr->buffer);
	field = http_hdr->first_field;
	while (field != NULL) {
		field2free = field;
		if (field->field_name)
			av_free(field->field_name);
		field = field->next;
		av_free(field2free);
	}
	av_free(http_hdr);
}

int http_response_append(GxHttpHeader*  http_hdr, char *response, int length)
{
	if (http_hdr == NULL || response == NULL || length < 0)
		return -1;

	if ((unsigned)length > SIZE_MAX - http_hdr->buffer_size - 1) {
		gxlogf("Bad size in memory (re)allocation\n");
		return -1;
	}
	http_hdr->buffer = (char* )av_realloc(http_hdr->buffer, http_hdr->buffer_size + length + 1);
	if (http_hdr->buffer == NULL) {
		gxlogf("Memory (re)allocation failed\n");
		return -1;
	}
	memcpy(http_hdr->buffer + http_hdr->buffer_size, response, length);
	http_hdr->buffer_size += length;
	http_hdr->buffer[http_hdr->buffer_size] = '\0';	// close the string!
	return http_hdr->buffer_size;
}

int http_is_header_entire(GxHttpHeader*  http_hdr)
{
	if (http_hdr == NULL)
		return -1;
	if (http_hdr->buffer == NULL)
		return 0;	// empty

	if (strstr(http_hdr->buffer, "\r\n\r\n") == NULL && strstr(http_hdr->buffer, "\n\n") == NULL)
		return 0;
	return 1;
}

int http_response_parse(GxHttpHeader*  http_hdr)
{
	char* hdr_ptr, *ptr;
	char* field = NULL;
	int pos_hdr_sep, hdr_sep_len;
	size_t len;
	if (!http_hdr || !http_hdr->buffer || !http_hdr->buffer_size)
		return -1;
	if (http_hdr->is_parsed)
		return 0;

	// Get the protocol
	hdr_ptr = strstr(http_hdr->buffer, " ");
	if (hdr_ptr == NULL) {
		gxlogf("Malformed answer. No space separator found.\n");
		return -1;
	}

	len = hdr_ptr - http_hdr->buffer;
	if(len == 0) {
		gxlogf("http protocol get failed\n");
		return -1;
	}

	http_hdr->protocol = av_malloc(len + 1);
	if (http_hdr->protocol == NULL) {
		gxlogf("Memory allocation failed\n");
		return -1;
	}
	strncpy(http_hdr->protocol, http_hdr->buffer, len);
	http_hdr->protocol[len] = '\0';
	if (!strncasecmp(http_hdr->protocol, "HTTP", 4)) {
		if (sscanf(http_hdr->protocol + 5, "1.%d", &(http_hdr->http_minor_version)) != 1) {
			gxlogf("Malformed answer. Unable to get HTTP minor version.\n");
			return -1;
		}
	}
	// Get the status code
	if (strlen(hdr_ptr) > 0 && sscanf(++hdr_ptr, "%d", &(http_hdr->status_code)) != 1) {
		gxlogf("Malformed answer. Unable to get status code.\n");
		return -1;
	}
	hdr_ptr += 4;

	// Get the reason phrase
	ptr = strstr(hdr_ptr, "\n");
	if (hdr_ptr == NULL) {
		gxlogf("Malformed answer. Unable to get the reason phrase.\n");
		return -1;
	}
	len = ptr - hdr_ptr;
	http_hdr->reason_phrase = av_malloc(len + 1);
	if (http_hdr->reason_phrase == NULL) {
		gxlogf("Memory allocation failed\n");
		return -1;
	}
	strncpy(http_hdr->reason_phrase, hdr_ptr, len);
	if (http_hdr->reason_phrase[len - 1] == '\r') {
		len--;
	}
	http_hdr->reason_phrase[len] = '\0';

	// Set the position of the header separator: \r\n\r\n
	hdr_sep_len = 4;
	ptr = strstr(http_hdr->buffer, "\r\n\r\n");
	if (ptr == NULL) {
		ptr = strstr(http_hdr->buffer, "\n\n");
		if (ptr == NULL) {
			gxlogf("Header may be incomplete. No CRLF CRLF found.\n");
			return -1;
		}
		hdr_sep_len = 2;
	}
	pos_hdr_sep = ptr - http_hdr->buffer;

	// Point to the first line after the method line.
	hdr_ptr = strstr(http_hdr->buffer, "\n") + 1;
	do {
		ptr = hdr_ptr;
		while (*ptr != '\r' &&* ptr != '\n')
			ptr++;
		len = ptr - hdr_ptr;
		if (len == 0)
			break;
		field = (char* )av_realloc(field, len + 1);
		if (field == NULL) {
			gxlogf("Memory allocation failed\n");
			return -1;
		}
		strncpy(field, hdr_ptr, len);
		field[len] = '\0';
		http_set_field(http_hdr, field);
		hdr_ptr = ptr + ((*ptr == '\r') ? 2 : 1);
	} while (hdr_ptr < (http_hdr->buffer + pos_hdr_sep));

	if (field != NULL)
		av_free(field);

	if (pos_hdr_sep + hdr_sep_len < http_hdr->buffer_size) {
		// Response has data!
		if(!strcasecmp(http_hdr->protocol, "ICY")) {
			http_hdr->body = http_hdr->buffer;
			http_hdr->body_size = http_hdr->buffer_size;
		}else{
			http_hdr->body = http_hdr->buffer + pos_hdr_sep + hdr_sep_len;
			http_hdr->body_size = http_hdr->buffer_size - (pos_hdr_sep + hdr_sep_len);
		}
	}

	http_hdr->is_parsed = 1;
	return 0;
}

char* http_build_request(GxHttpHeader * http_hdr)
{
	char* ptr, *uri = NULL;
	int len;
	GxHttpField* field;
	if (http_hdr == NULL)
		return NULL;

	if (http_hdr->method == NULL)
		http_set_method(http_hdr, "GET");
	if (http_hdr->uri == NULL)
		http_set_uri(http_hdr, "/");
	else {
		uri = av_mallocz(strlen(http_hdr->uri) + 2);
		if (uri == NULL) {
			gxlogf("Memory allocation failed\n");
			return NULL;
		}
		av_strlcpy(uri, http_hdr->uri, strlen(http_hdr->uri)+1);
	}

	//**** Compute the request length
	// Add the Method line
	len = strlen(http_hdr->method) + strlen(uri) + 12;
	// Add the fields
	field = http_hdr->first_field;
	while (field != NULL) {
		len += strlen(field->field_name) + 2;
		field = field->next;
	}
	// Add the CRLF
	len += 2;
	// Add the body
	if (http_hdr->body != NULL) {
		len += http_hdr->body_size;
	}
	// Free the buffer if it was previously used
	if (http_hdr->buffer != NULL) {
		av_free(http_hdr->buffer);
		http_hdr->buffer = NULL;
	}
	http_hdr->buffer = av_malloc(len + 1);
	if (http_hdr->buffer == NULL) {
		gxlogf("Memory allocation failed\n");
		return NULL;
	}
	http_hdr->buffer_size = len;

	//*** Building the request
	ptr = http_hdr->buffer;
	// Add the method line
	ptr += snprintf(ptr, len, "%s %s HTTP/1.1\r\n", http_hdr->method, uri);//http_hdr->http_minor_version
	field = http_hdr->first_field;
	// Add the field
	while (field != NULL) {
		ptr += snprintf(ptr, len, "%s\r\n", field->field_name);
		field = field->next;
	}
	ptr += snprintf(ptr, len, "\r\n");
	// Add the body
	if (http_hdr->body != NULL) {
		memcpy(ptr, http_hdr->body, http_hdr->body_size);
	}

	if (uri)
		av_free(uri);
	return http_hdr->buffer;
}

char* http_get_field(GxHttpHeader * http_hdr, const char *field_name)
{
	if (http_hdr == NULL || field_name == NULL)
		return NULL;
	http_hdr->field_search_pos = http_hdr->first_field;
	http_hdr->field_search = (char* )av_realloc(http_hdr->field_search, strlen(field_name) + 2);
	if (http_hdr->field_search == NULL) {
		gxlogf("Memory allocation failed\n");
		return NULL;
	}
	http_hdr->field_search[0] = '\0';
	av_strlcpy(http_hdr->field_search, field_name, strlen(field_name)+1);
	if(strlen(http_hdr->field_search) == 0) {
		gxlogf("field search len eq 0\n");
		return NULL;
	}
	return http_get_next_field(http_hdr);
}

char* http_get_next_field(GxHttpHeader * http_hdr)
{
	char* ptr;
	GxHttpField* field;
	if (http_hdr == NULL)
		return NULL;

	memset (http_hdr->acSavCkBuf, 0, sizeof(http_hdr->acSavCkBuf)/sizeof(char));
	http_hdr->iCookieTotal = 0;
	field = http_hdr->field_search_pos;
	while (field != NULL) {
		ptr = strstr(field->field_name, ":");
		if (ptr == NULL)
			return NULL;
		if (!strncasecmp(field->field_name, http_hdr->field_search, strlen(http_hdr->field_search))) {
			ptr++;	// Skip the column
			while (ptr[0] == ' ')
				ptr++;	// Skip the spaces if there is some
			http_hdr->field_search_pos = field->next;
			if (0 == strcasecmp(http_hdr->field_search, "Set-Cookie")) {
				http_hdr->iCookieTotal += 1;
				snprintf (http_hdr->acSavCkBuf + strlen(http_hdr->acSavCkBuf), sizeof(http_hdr->acSavCkBuf)/sizeof(char), "%s\n", ptr);
			} else {
				return ptr;	// return the value without the field name
			}
		}
		field = field->next;
	}

	return NULL;
}

void http_set_fields(GxHttpHeader* http_hdr, const char **field_name, unsigned int count)
{
	GxHttpField* new_field;
	unsigned int field_len = 0, i = 0;

	if (http_hdr == NULL || field_name == NULL)
		return;

	for (i = 0; i < count; i++)
		field_len += field_name[i]?strlen(field_name[i]):0;

	if (field_len == 0)
		return;

	new_field = av_malloc(sizeof(GxHttpField));
	if (new_field == NULL) {
		gxlogf("Memory allocation failed\n");
		return;
	}
	new_field->next = NULL;
	new_field->field_name = av_mallocz(field_len + 1);
	if (new_field->field_name == NULL) {
		gxlogf("Memory allocation failed\n");
		av_free(new_field);
		return;
	}

	for (i = 0; i < count; i++) {
		if (field_name[i])
			strncat(new_field->field_name, field_name[i], field_len);
	}

	if (http_hdr->last_field == NULL) {
		http_hdr->first_field = new_field;
	} else {
		http_hdr->last_field->next = new_field;
	}
	http_hdr->last_field = new_field;
	http_hdr->field_nb++;
}

void http_set_field(GxHttpHeader*  http_hdr, const char *field_name)
{
	GxHttpField* new_field;
	if (http_hdr == NULL || field_name == NULL)
		return;

	new_field = av_malloc(sizeof(GxHttpField));
	if (new_field == NULL) {
		gxlogf("Memory allocation failed\n");
		return;
	}
	new_field->next = NULL;
	new_field->field_name = av_malloc(strlen(field_name) + 2);
	if (new_field->field_name == NULL) {
		gxlogf("Memory allocation failed\n");
		av_free(new_field);
		return;
	}
	av_strlcpy(new_field->field_name, field_name, strlen(field_name)+1);

	if (http_hdr->last_field == NULL) {
		http_hdr->first_field = new_field;
	} else {
		http_hdr->last_field->next = new_field;
	}
	http_hdr->last_field = new_field;
	http_hdr->field_nb++;
}

void http_set_method(GxHttpHeader*  http_hdr, const char *method)
{
	if (http_hdr == NULL || method == NULL)
		return;

	http_hdr->method = av_mallocz(strlen(method) + 2);
	if (http_hdr->method == NULL) {
		gxlogf("Memory allocation failed\n");
		return;
	}
	av_strlcpy(http_hdr->method, method, strlen(method)+1);

}

void http_set_uri(GxHttpHeader*  http_hdr, const char *uri)
{
	if (http_hdr == NULL || uri == NULL)
		return;

	http_hdr->uri = av_malloc(strlen(uri) + 2);
	if (http_hdr->uri == NULL) {
		gxlogf("Memory allocation failed\n");
		return;
	}
	av_strlcpy(http_hdr->uri, uri, strlen(uri)+1);
}

#define AV_BASE64_SIZE(x)  (((x)+2) / 3 * 4 + 1)

static int http_add_authentication( GxHttpHeader *http_hdr, const char *username, const char *password, const char *auth_str )
{
	char *auth = NULL, *usr_pass = NULL, *b64_usr_pass = NULL;
	int encoded_len, pass_len=0;
	size_t auth_len, usr_pass_len;
	int res = -1;
	if( http_hdr==NULL || username==NULL ) return -1;

	if( password!=NULL ) {
		pass_len = strlen(password);
	}

	usr_pass_len = strlen(username) + 1 + pass_len;
	usr_pass = av_malloc(usr_pass_len + 1);
	if( usr_pass==NULL ) {
		gxlogd("MSGT_NETWORK,MSGL_FATAL,MSGTR_MemAllocFailed\n");
		goto out;
	}

	snprintf( usr_pass, usr_pass_len, "%s:%s", username, (password==NULL)?"":password );

	encoded_len = AV_BASE64_SIZE(usr_pass_len);
	b64_usr_pass = av_malloc(encoded_len);
	if( b64_usr_pass==NULL ) {
		gxlogd("MSGT_NETWORK,MSGL_FATAL,MSGTR_MemAllocFailed\n");
		goto out;
	}
	av_base64_encode(b64_usr_pass, encoded_len, (const uint8_t*)usr_pass, usr_pass_len);

	auth_len = encoded_len + 100;
	auth = av_malloc(auth_len);
	if( auth==NULL ) {
		gxlogd("MSGT_NETWORK,MSGL_FATAL,MSGTR_MemAllocFailed\n");
		goto out;
	}

	snprintf(auth, auth_len, "%s: Basic %s", auth_str, b64_usr_pass);
	http_set_field( http_hdr, auth );
	res = 0;

out:
	av_free( usr_pass );
	av_free( b64_usr_pass );
	av_free( auth );

	return res;
}

int http_add_basic_authentication( GxHttpHeader *http_hdr, const char *username, const char *password )
{
	return http_add_authentication(http_hdr, username, password, "Authorization");
}

int http_add_basic_proxy_authentication( GxHttpHeader *http_hdr, const char *username, const char *password )
{
	return http_add_authentication(http_hdr, username, password, "Proxy-Authorization");
}

void http_debug_hdr(GxHttpHeader*  http_hdr)
{
	GxHttpField* field;
	int i = 0;
	if (http_hdr == NULL)
		return;

	gxlogf("--- HTTP DEBUG HEADER --- START ---\n");
	gxlogf("protocol:           [%s]\n", http_hdr->protocol);
	gxlogf("http minor version: [%d]\n", http_hdr->http_minor_version);
	gxlogf("uri:                [%s]\n", http_hdr->uri);
	gxlogf("method:             [%s]\n", http_hdr->method);
	gxlogf("status code:        [%d]\n", http_hdr->status_code);
	gxlogf("reason phrase:      [%s]\n", http_hdr->reason_phrase);
	gxlogf("body size:          [%d]\n", http_hdr->body_size);

	gxlogf("Fields:\n");
	field = http_hdr->first_field;
	while (field != NULL) {
		gxlogf(" %d - %s\n", i, field->field_name);
		field = field->next;
		i++;
	}
	gxlogf("--- HTTP DEBUG HEADER --- END ---\n");
}

static void print_icy_metadata(GxHttpHeader*  http_hdr)
{
	char* field_data;
	// note: I skip icy-notice1 and 2, as they contain html <BR>
	// and are IMHO useless info ::atmos
	if ((field_data = http_get_field(http_hdr, "icy-name")) != NULL)
		gxlogf("Name   : %s\n", field_data);
	if ((field_data = http_get_field(http_hdr, "icy-genre")) != NULL)
		gxlogf("Genre  : %s\n", field_data);
	if ((field_data = http_get_field(http_hdr, "icy-url")) != NULL)
		gxlogf("Website: %s\n", field_data);
	// XXX: does this really mean public server? ::atmos
	if ((field_data = http_get_field(http_hdr, "icy-pub")) != NULL)
		gxlogf("Public : %s\n", atoi(field_data) ? "yes" : "no");
	if ((field_data = http_get_field(http_hdr, "icy-br")) != NULL)
		gxlogf("Bitrate: %skbit/s\n", field_data);
}

static int parse_set_cookie(const char *set_cookie, AVDictionary **dict)
{
	char *param = NULL, *next_param = NULL, *cstr = NULL, *back = NULL;

	if (!set_cookie[0])
		return 0;

	if (!(cstr = av_strdup(set_cookie)))
		return 0;

	// strip any trailing whitespace
	back = &cstr[strlen(cstr)-1];
	while (strchr(WHITESPACES, *back)) {
		*back='\0';
		if (back == cstr)
			break;
		back--;
	}

	next_param = cstr;
	while ((param = av_strtok(next_param, ";", &next_param))) {
		char *name = NULL, *value = NULL;
		param += strspn(param, WHITESPACES);
		if ((name = av_strtok(param, "=", &value))) {
			if (value && av_dict_set(dict, name, value, 0) < 0) {
				if (cstr)
					av_free(cstr);
				return -1;
			}
		}
	}

	if (cstr)
		av_free(cstr);
	return 0;
}

/**
 * Create a string containing cookie values for use as a HTTP cookie header
 * field value for a particular path and domain from the cookie values stored in
 * the HTTP protocol context. The cookie string is stored in *cookies, and may
 * be NULL if there are no valid cookies.
 *
 * @return a negative value if an error condition occurred, 0 otherwise
 */
static int get_cookies(char *pcCookiesBuff, char *cookies, int iCookiesSize)
{
	char *cookie = NULL, *set_cookies = NULL, *next = NULL;

	if (!pcCookiesBuff) {
		gxlogd ("cookies buffer null, [%s, %d]\n", __func__, __LINE__);
		return 0;
	}

	next = set_cookies = av_strdup(pcCookiesBuff);

	if (!next) {
		gxlogd ("malloc cookies buff fail, [%s, %d]\n", __func__, __LINE__);
		return 0;
	}


	while ((cookie = av_strtok(next, "\n", &next))) {
		AVDictionary *cookie_params = NULL;
		AVDictionaryEntry *cookie_entry, *e=NULL;

		// continue on to the next cookie if this one cannot be parsed
		if (parse_set_cookie(cookie, &cookie_params)) {
			gxlogd ("parse set cokie fail, [%s, %d]\n", __func__, __LINE__);
			goto skip_cookie;
		}

		// if the cookie has no value, skip it
		cookie_entry = av_dict_get(cookie_params, "", NULL, AV_DICT_IGNORE_SUFFIX);
		if (!cookie_entry || !cookie_entry->value) {
			gxlogd ("cookies fail, [%s, %d]\n", __func__, __LINE__);
			goto skip_cookie;
		}

		// if no domain in the cookie assume it appied to this request
		e = av_dict_get(cookie_params, "domain", NULL, 0);

		if (e && e->value) {
			snprintf (cookies, iCookiesSize, "%s=%s;%s=%s", cookie_entry->key, cookie_entry->value, "domain", e->value);
		} else {
			snprintf (cookies, iCookiesSize, "%s=%s", cookie_entry->key, cookie_entry->value);
		}
skip_cookie:
		if (cookie_params)
			av_dict_free(&cookie_params);
	}

	av_free(set_cookies);
	return 1;
}


//! If this function succeeds you must closesocket stream->fd
static int http_streaming_start(GxStreamHttp*  stream, int64_t pos, int *file_format)
{
	GxHttpHeader* http_hdr = NULL;
	unsigned int i = 0;
	int fd = stream->parent.fd;
	int res = -1;
	int redirect = 0;
	int auth_retry = 0;
	int seekable = 0;
	int no_response_count = 0;
	char* next_url;
	GxURL* url = stream->streaming_ctrl->url;

	memset(stream->parent.mime_type, 0, sizeof(stream->parent.mime_type));

	do {
		if(http_interrupt(stream)) {
			goto err_out;
		}
		redirect = 0;
		if (fd > 0) {
			closesocket(fd);
			fd = -1;
		}
		fd = http_send_request2(stream, url, pos, stream->parent.interrupt_cbk);
		if (fd < 0)
			goto err_out;

		http_freep(&http_hdr);
		http_hdr = http_read_response2(stream, fd, stream->parent.interrupt_cbk);
		if (http_hdr == NULL) {
			no_response_count++;
			if(no_response_count <= 2) {
				gxlogd("http server no respone cont %d\n", no_response_count);
				redirect = 1;
				continue;
			}
			goto err_out;
		}
		http_debug_hdr(http_hdr);

		// Check if we can make partial content requests and thus seek in http-streams
		if( http_hdr!=NULL && (http_hdr->status_code==200||http_hdr->status_code==206)) {
			const char *accept_ranges  = http_get_field(http_hdr,"Accept-Ranges");
			const char *content_ranges = http_get_field(http_hdr,"Content-Range");
			const char *server = http_get_field(http_hdr, "Server");
			if (accept_ranges || content_ranges) {
				if (accept_ranges)
					seekable |= (strncmp(accept_ranges, "bytes", 5)==0);
				if (content_ranges)
					seekable |= (strncmp(content_ranges, "bytes", 5)==0);
			}else if (server && (strcmp(server, "gvs 1.0") == 0 ||
						strncmp(server, "MakeMKV", 7) == 0)) {
				// HACK for youtube and MakeMKV incorrectly claiming not to support seeking
				gx_msg(MSGT_NETWORK, MSGL_WARN, "Broken webserver, incorrectly claims to not support Accept-Ranges\n");
				seekable = 1;
			}
		}

		print_icy_metadata(http_hdr);

		// Check if the response is an ICY status_code reason_phrase
		if (!strcasecmp(http_hdr->protocol, "ICY") || http_get_field(http_hdr, "Icy-MetaInt")) {
			switch (http_hdr->status_code) {
			case 200:	// OK
			case 206:{	// OK
				char* field_data = NULL;
				// If content-type == video/nsv we most likely have a winamp video stream
				// otherwise it should be mp3. if there are more types consider adding mime type
				// handling like later
				if ((field_data = http_get_field(http_hdr, "content-type")) != NULL
						&& (!strcmp(field_data, "video/nsv")
							|| !strcmp(field_data, "misc/ultravox")))
					*file_format = GX_DEMUXER_TYPE_NSV;
				else if ((field_data = http_get_field(http_hdr, "content-type")) != NULL
						&& (!strcmp(field_data, "audio/aacp")
							|| !strcmp(field_data, "audio/aac"))) {
					*file_format = GX_DEMUXER_TYPE_AAC;
				} else
					*file_format = GX_DEMUXER_TYPE_AUDIO;
				if (NULL != field_data) {
					strncpy(stream->parent.mime_type, field_data, PLAYER_MIME_TYPE_LONG);
				}
				stream->streaming_ctrl->con_len = -1;
				stream->streaming_ctrl->res_len = -1;
				stream->streaming_ctrl->chk_size = -1;
				res = 0;
				goto out;
			}
			case 400:	// Server Full
				gxlogf("Error: ICY-Server is full, skipping!\n");
				goto err_out;
			case 401:	// Service Unavailable
				gxlogf("Error: ICY-Server return service unavailable, skipping!\n");
				goto err_out;
			case 403:	// Service Forbidden
				gxlogf("Error: ICY-Server return 'Service Forbidden'\n");
				goto err_out;
			case 404:	// Resource Not Found
				gxlogf("Error: ICY-Server couldn't find requested stream, skipping!\n");
				goto err_out;
			default:
				gxlogf("Error: unhandled ICY-Errorcode, contact MPlayer developers!\n");
				goto err_out;
			}
		}
		// Assume standard http if not ICY
		switch (http_hdr->status_code) {
		case 200:	// OK
		case 206:	// OK
			{
				// Look if we can use the Content-Type
				char* content_args = NULL;
				off_t filesize = 0;

				content_args = http_get_field(http_hdr, "Transfer-Encoding");
				if((content_args != NULL) && !strncasecmp(content_args, "chunked", 7))
					stream->streaming_ctrl->chk_size = 0;
				else
					stream->streaming_ctrl->chk_size = -1;

				content_args = http_get_field(http_hdr, "Time-Range");
				if (content_args != NULL) {
					int s1=0, s2=0;
					sscanf(content_args, "npt=%d-%d", &s1, &s2);
					stream->streaming_ctrl->duration = s2;
				}

				content_args = http_get_field(http_hdr, "Content-Length");
				if (content_args != NULL && ((filesize = atoll(content_args)) != 0)) {
					stream->streaming_ctrl->res_len = filesize;
					stream->parent.end_pos = stream->streaming_ctrl->con_len = pos + stream->streaming_ctrl->res_len;
				} else {
					stream->streaming_ctrl->res_len = stream->streaming_ctrl->con_len = -1;
					content_args = http_get_field(http_hdr, "Content-Range");
					if (content_args) {
						if (!strncmp(content_args, "bytes ", 6)) {
							const char *slash = NULL;
							content_args += 6;
							if ((slash = strchr(content_args, '/')) && strlen(slash) > 0) {
								filesize = atoll(slash + 1);
								if (filesize > pos) {
									stream->parent.end_pos = stream->streaming_ctrl->con_len = filesize;
									stream->streaming_ctrl->res_len = filesize - pos;
								}
							}
						}
					}
				}

				content_args = http_get_field(http_hdr, "Content-Encoding");
				if ((content_args != NULL) &&
						(!strncasecmp(content_args, "gzip", 4) || !strncasecmp(content_args, "deflate", 7))) {
					stream->streaming_ctrl->compress = 1;
					inflateEnd(&stream->streaming_ctrl->inflate_stream);
					if (inflateInit2(&stream->streaming_ctrl->inflate_stream, 32 + 15) != Z_OK) {
						gxlogf("Error zlib initialisation: %s\n", stream->streaming_ctrl->inflate_stream.msg);
						goto err_out;
					}
					if (zlibCompileFlags() & (1 << 17)) {
						gxlogf("Your zlib was compiled without gzip support.\n");
						goto err_out;
					}
				}

				content_args = http_get_field(http_hdr, "Content-Type");
				if (content_args != NULL) {
					// Check in the mime type table for a demuxer type
					i = 0;
					while (GxMime_Type_Table[i].mime_type != NULL) {
						if (!strcasecmp(content_args, GxMime_Type_Table[i].mime_type)) {
							*file_format = GxMime_Type_Table[i].demuxer_type;
							strncpy(stream->parent.mime_type, GxMime_Type_Table[i].mime_type, PLAYER_MIME_TYPE_LONG);
							break;
						}
						i++;
					}
				}

				http_get_field(http_hdr, "Set-Cookie");
				if (http_hdr->iCookieTotal >= 1 && strlen(http_hdr->acSavCkBuf)) {
					char *pcBuffer = NULL;
					pcBuffer = av_malloc(strlen(http_hdr->acSavCkBuf));
					if (pcBuffer) {
						memset (pcBuffer, 0, strlen(http_hdr->acSavCkBuf));
						get_cookies(http_hdr->acSavCkBuf, pcBuffer, strlen(http_hdr->acSavCkBuf)-1);
						if (stream->streaming_ctrl->cookies) {
							av_free(stream->streaming_ctrl->cookies);
							stream->streaming_ctrl->cookies = NULL;
						}
						stream->streaming_ctrl->cookies = av_strdup(pcBuffer);
						if (pcBuffer) {
							av_free(pcBuffer);
							pcBuffer = NULL;
						}
					} else {
						goto err_out;
					}
				} else {
					if (content_args) {
						if (strlen(http_hdr->acSavCkBuf))
							stream->streaming_ctrl->cookies = av_strdup(http_hdr->acSavCkBuf);
					}
				}


				if(*file_format == GX_DEMUXER_TYPE_UNKNOWN) {
					if(strstr(stream->parent.url,".flv.ts") != NULL) {
						*file_format = GX_DEMUXER_TYPE_MPEG_TS;
						strncpy(stream->parent.mime_type, "flv.ts", PLAYER_MIME_TYPE_LONG);
					}else if(strstr(stream->parent.url,".ts") != NULL){
						*file_format = GX_DEMUXER_TYPE_MPEG_TS;
						strncpy(stream->parent.mime_type, "ts", PLAYER_MIME_TYPE_LONG);
					}else if(strstr(stream->parent.url,".flv") != NULL){
						*file_format = GX_DEMUXER_TYPE_LAVF;
						strncpy(stream->parent.mime_type, "flv", PLAYER_MIME_TYPE_LONG);
					}else if(strstr(stream->parent.url,".mp3") != NULL){
						*file_format = GX_DEMUXER_TYPE_AUDIO;
						strncpy(stream->parent.mime_type, "mp3", PLAYER_MIME_TYPE_LONG);
					}
				}
				// Not found in the mime type table, don't fail,
				// we should try raw HTTP
				res = seekable;
				goto out;
			}
			// Redirect
		case 301: // Permanently
		case 302: // Temporarily
		case 303: // See Other
		case 307: // Temporarily (since HTTP/1.1)
			// TODO: RFC 2616, recommand to detect infinite redirection loops
			next_url = http_get_field( http_hdr, "Location" );
			if( next_url!=NULL ) {
				char* cookies = NULL;
				int is_ultravox = strcasecmp(stream->streaming_ctrl->url->protocol, "unsv") == 0;
				stream->streaming_ctrl->url = url_redirect( &url, next_url );
				if (!strcasecmp(url->protocol, "mms")) {
					res = GX_STREAM_REDIRECTED;
					goto err_out;
				}
				if (strcasecmp(url->protocol, "http") &&
						strcasecmp(url->protocol, "https")) {
					gxlogd("Unsupported http %d redirect to %s protocol\n", http_hdr->status_code, url->protocol);
					goto err_out;
				}
				if (!strcasecmp(url->protocol, "https") && !stream->https_protocol && !stream->https) {
					stream->https = av_mallocz(sizeof(GxStreamHttps));
					if(stream->https == NULL)
						goto err_out;
					https_init();
					stream->https_protocol = 1;
					stream->streaming_ctrl->priv = stream;
					stream->https->interrupt_cbk = stream->parent.interrupt_cbk;
				} else if (!strcasecmp(url->protocol, "http") && stream->https_protocol && stream->https) {
					https_close(stream->https);
					av_free(stream->https);
					https_deinit();
					stream->https = NULL;
					stream->https_protocol = 0;
					stream->streaming_ctrl->priv = NULL;
				}

				if (is_ultravox)  {
					av_free(url->protocol);
					url->protocol = av_strdup("unsv");
				}
				if (stream->parent.redirect_url) {
					av_free(stream->parent.redirect_url);
					stream->parent.redirect_url = NULL;
				}
				if (stream->streaming_ctrl->url)
					stream->parent.redirect_url = av_strdup(stream->streaming_ctrl->url->url);

				if (stream->streaming_ctrl->cookies) {
					av_free(stream->streaming_ctrl->cookies);
					stream->streaming_ctrl->cookies = NULL;
				}
				cookies = http_get_field(http_hdr, "Set-Cookie");


				if (http_hdr->iCookieTotal > 1 && strlen(http_hdr->acSavCkBuf)) {
					char *pcBuffer = NULL;
					pcBuffer = av_malloc(strlen(http_hdr->acSavCkBuf));
					if (pcBuffer) {
						memset (pcBuffer, 0, strlen(http_hdr->acSavCkBuf));
						get_cookies(http_hdr->acSavCkBuf, pcBuffer, strlen(http_hdr->acSavCkBuf)-1);
						stream->streaming_ctrl->cookies = av_strdup(pcBuffer);
						if (pcBuffer) {
							av_free(pcBuffer);
							pcBuffer = NULL;
						}
					} else {
						gxlogd ("cookies buffer malloc fail\n");
					}
				} else {
					if (cookies)
						stream->streaming_ctrl->cookies = av_strdup(http_hdr->acSavCkBuf);
				}

				redirect = 1;
			}
			break;
		case 401: // Authentication required
			if( http_authenticate(http_hdr, url, &auth_retry)<0 )
				goto err_out;
			redirect = 1;
		case 404:
			no_response_count++;
			redirect = 1;
			break;
		default:
			gxlogd("server response code[%d]\n", http_hdr->status_code);
			goto err_out;
		}
	} while((no_response_count<4) && redirect);

err_out:
	if (fd > 0)
		closesocket(fd);
	fd = res = -1;
	http_freep(&http_hdr);
out:
	stream->streaming_ctrl->data = (void* )http_hdr;
	stream->parent.fd = fd;
	return res;
}

static int http_streaming_seek(GxStream* stream, int64_t pos)
{
	GxStreamHttp* http_stream = (GxStreamHttp*)stream;
	GxHttpHeader* http_hdr = http_stream->streaming_ctrl->data;

	if(http_stream->https_protocol && http_stream->https)
		https_shutdown(http_stream->https);

	http_freep(&http_hdr);
	streaming_buffer_free(http_stream->streaming_ctrl);
	if(http_streaming_start(http_stream, pos, &http_stream->parent.demuxer_type) < 0)
		return GX_PLAYER_ERROR;

	http_hdr = http_stream->streaming_ctrl->data;
	if (http_hdr && http_hdr->body_size > 0) {
		if (streaming_bufferize(http_stream->streaming_ctrl, http_hdr->body, http_hdr->body_size) < 0) {
			http_freep(&http_hdr);
			http_stream->streaming_ctrl->data = NULL;
			return GX_PLAYER_ERROR;
		}
	}

	if (http_hdr) {
		http_freep(&http_hdr);
		http_stream->streaming_ctrl->data = NULL;
	}
	return GX_PLAYER_OK;
}

static int fixup_open(GxStream*  stream, int seekable)
{
	GxStreamHttp* http_stream = (GxStreamHttp *) stream;
	GxHttpHeader* http_hdr = http_stream->streaming_ctrl->data;

	int is_icy = http_hdr && http_get_field(http_hdr, "Icy-MetaInt");
	int is_ultravox = strcasecmp(http_stream->streaming_ctrl->url->protocol, "unsv") == 0;

	stream->file_format = GX_STREAMTYPE_STREAM;
	if (!is_icy && !is_ultravox && seekable) {
		stream->flags |= GX_STREAM_SEEK;
	}

	if ((!is_icy && !is_ultravox) || scast_streaming_start(stream)) {
		if (nop_streaming_start(stream)) {
			gxlogf("nop_streaming_start failed\n");
			if (stream->fd >= 0)
				closesocket(stream->fd);
			stream->fd = -1;

			return GX_PLAYER_ERROR;
		}
	}

	stream->streaming_ctrl = http_stream->streaming_ctrl;
	stream->file_format = GX_STREAMTYPE_STREAM;
#if STREAM_HTTP_PVR_DEBUG
	if(stream_http_pvr_fd)
		fclose(stream_http_pvr_fd);
	stream_http_pvr_fd = fopen(stream_http_pvr_file, "w+");
#endif
	return GX_PLAYER_OK;
}

static int open_http1(GxStream*  stream, int mode)
{
	GxStreamHttp* http_stream = (GxStreamHttp *) (stream);
	int seekable = 0;
	GxURL* url = NULL;
	char *url_cache_value = NULL;

	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT,   &stream->interrupt_cbk);
	GxPlayer_SystemGet(PSYS_NETWORK_TIMEOUT, &http_stream->timeout);
	http_stream->streaming_ctrl = streaming_ctrl_new();
	if (http_stream->streaming_ctrl == NULL)
		return GX_PLAYER_ERROR;

	url = url_new(http_stream->parent.url);
	if (url == NULL) {
		streaming_ctrl_free(http_stream->streaming_ctrl);
		http_stream->streaming_ctrl = NULL;
		return GX_PLAYER_ERROR;
	}

	if ((url_cache_value = GxOptions_Get_By_Name(((GxStream*)stream)->options, " -H", "no_cache_flag:"))) {
		http_stream->parent.nobuffer = atoi(url_cache_value);
	} else {
		http_stream->parent.nobuffer = 0;
	}

	http_stream->streaming_ctrl->url = check4proxies(url);
	if (http_stream->streaming_ctrl->url == NULL) {
		url_free(url);
		streaming_ctrl_free(http_stream->streaming_ctrl);
		http_stream->streaming_ctrl = NULL;
		return GX_PLAYER_ERROR;
	}
	url_free(url);

	if(http_stream->streaming_ctrl->url &&
			(strcasecmp(http_stream->streaming_ctrl->url->protocol, "https") == 0)) {
		http_stream->https = av_mallocz(sizeof(GxStreamHttps));
		if(http_stream->https == NULL)
			goto http1_fail;

		https_init();
		http_stream->https_protocol = 1;
		http_stream->streaming_ctrl->priv = http_stream;
		http_stream->https->interrupt_cbk = stream->interrupt_cbk;
	}

	gxlogf("STREAM_HTTP(1), URL: %s\n", http_stream->parent.url);
	seekable = http_streaming_start(http_stream, 0, &http_stream->parent.demuxer_type);
	if (seekable < 0)
		goto http1_fail;

	seekable = fixup_open(stream, seekable);
	if(seekable == GX_PLAYER_ERROR)
		goto http1_fail;

	return GX_PLAYER_OK;

http1_fail:
	if(http_stream->streaming_ctrl->url &&
			(strcasecmp(http_stream->streaming_ctrl->url->protocol, "https") == 0)) {
		if(http_stream->https) {
			https_close(http_stream->https);
			av_free(http_stream->https);
			http_stream->https = NULL;
			https_deinit();
		}
	}
	streaming_ctrl_free(http_stream->streaming_ctrl);
	stream->streaming_ctrl = http_stream->streaming_ctrl = NULL;
	return GX_PLAYER_ERROR;
}

static int http_get_line(GxStreamHttp* s, char* line, int line_size)
{
	int retry = 15;
	char* q = line;
	char ch;

	for(;;) {
		while(1) {
			int ret;
			if(http_interrupt(s)) {
				return -1;
			}
			ret = s->streaming_ctrl->streaming_read(s->parent.fd, (uint8_t*)(&ch), 1, s->streaming_ctrl);
			if(ret < 0 || retry==0) {
				return -1;
			}else if(ret == 0) {
				retry--;
			}else
				break;
		}
		retry = 15;
		if(ch < 0) {
			return ch;
		}
		if(ch == '\n') {
			if(q > line && q[-1] == '\r')
				q--;
			*q = '\0';
			return 0;
		}else{
			if((q - line) < line_size - 1)
				*q++ = ch;
		}
	}
}

static int http_get_chunk_size(GxStreamHttp*s)
{
	char line[32]={0};
	int size;

	do{
		if(http_get_line(s, line, sizeof(line)) < 0) {
			gxlogf("get chunk_size error\n");
			return 0;
		}
	}while(!*line);
	size =  strtoll(line, NULL, 16);
	return size;
}

static size_t _get_min_read_data(GxStreamHttp* http_stream, size_t max_len)
{
	GxStreamingCtrl* ctrl = http_stream->streaming_ctrl;

	if(ctrl->res_len >= 0)
		max_len = MIN(ctrl->res_len, max_len);
	if(ctrl->chk_size >= 0) {
		if(ctrl->chk_size == 0)
			ctrl->chk_size = http_get_chunk_size(http_stream);
		if(ctrl->chk_size <= 0) {
			http_stream->parent.err.need_restart = 1;
			return -1;
		}
		max_len = MIN(ctrl->chk_size, max_len);
	}
	return max_len;
}

static int _check_restart(GxStreamHttp* http_stream, int len)
{
	GxStreamingCtrl* ctrl = http_stream->streaming_ctrl;
	int retry_count = http_stream->timeout/100;

	if(http_stream->parent.demuxer_type == GX_DEMUXER_TYPE_AAC)
		retry_count = 500;

	if(len < 0) {
		if((ctrl->chk_size > 0) || ctrl->res_len)
			http_stream->parent.err.need_restart = 1; //read data not over, must restart
		len = -1;
	}else if(len == 0) {
		if((ctrl->chk_size > 0) || ctrl->res_len)
			http_stream->parent.err.recv_err++;       //read data zero and not over, recv error
		else if(!ctrl->res_len)
			len = -1;
		if(http_stream->parent.err.recv_err > retry_count) {
			http_stream->parent.err.recv_err = 0;
			if ((!http_stream->parent.err.need_length) ||
					(http_stream->parent.err.need_length != ctrl->res_len)) {
				http_stream->parent.err.need_restart = 1;
				len = -1;
			} else {
				http_stream->parent.err.need_restart = 1;
				len = -2;
			}
		}
	}else{
		http_stream->parent.err.recv_err = 0;
		if(ctrl->res_len > 0)
			ctrl->res_len -= len;
		if(ctrl->chk_size > 0)
			ctrl->chk_size -= len;
	}

	if(len < 0)
		gxlogf("con_len %lld, res_len %lld, chunk_size %lld\n", ctrl->con_len, ctrl->res_len, ctrl->chk_size);

	return len;
}

static int net_seek(GxStream*  s, off_t newpos)
{
	GxStreamHttp*  http_stream = (GxStreamHttp *) (s);

	if (http_stream && http_stream->streaming_ctrl) {
		if (http_stream->streaming_ctrl->con_len == -1)
			newpos = 0;
	}

	if(http_streaming_seek(s, newpos)!=GX_PLAYER_OK)
		return GX_PLAYER_ERROR;

	return GX_PLAYER_OK;
}

static int net_read(GxStream*  s, uint8_t * buffer, size_t max_len)
{
	int len = 0;

	GxStreamHttp* obj = (GxStreamHttp *) (s);
	if (obj != NULL && obj->streaming_ctrl != NULL) {
		//calc really read data len
		max_len=_get_min_read_data(obj, max_len);
		if(((int)max_len) <= 0)
			return -1;

		len = obj->streaming_ctrl->streaming_read(obj->parent.fd, buffer, max_len,
				obj->streaming_ctrl);
		len = _check_restart(obj, len);

		if(len == -1 && obj->streaming_ctrl->res_len && obj->parent.err.need_restart) {
			off_t newpos  = 0;
			if(obj->streaming_ctrl->res_len > 0) {
				newpos = obj->streaming_ctrl->con_len - obj->streaming_ctrl->res_len;
			}else{
				if((s->flags&GX_STREAM_NO_SEEK) == GX_STREAM_NO_SEEK) {
					obj->parent.err.need_restart = 0;
					return -1;
				}else{
					newpos = 0;
				}
			}
			if (net_seek(s, newpos) == GX_PLAYER_OK) {
				obj->parent.err.need_restart = 0;
				obj->parent.err.need_length  = obj->streaming_ctrl->res_len;
				return 0;
			}
		}
	}
#if STREAM_HTTP_PVR_DEBUG
	if(stream_http_pvr_fd && len > 0) {
		fwrite(buffer, len, 1, stream_http_pvr_fd);
	}
#endif

	return (len < 0) ? -1 : len;
}

static int net_compress_read(GxStream*  s, uint8_t * buffer, size_t max_len)
{
#define DECOMPRESS_BUF_SIZE (256 * 1024)
	int ret;
	GxStreamHttp* obj = (GxStreamHttp *) (s);

	if (!obj || !obj->streaming_ctrl) return -1;

	if (obj->streaming_ctrl->uncompress_buffer) {
		int uncompress_size = obj->streaming_ctrl->uncompress_buffer_size - obj->streaming_ctrl->uncompress_buffer_pos;
		max_len = (max_len > uncompress_size)?uncompress_size:max_len;
		memcpy(buffer, obj->streaming_ctrl->uncompress_buffer, max_len);
		obj->streaming_ctrl->uncompress_buffer_pos += max_len;
		if (obj->streaming_ctrl->uncompress_buffer_pos >= obj->streaming_ctrl->uncompress_buffer_size) {
			av_free(obj->streaming_ctrl->uncompress_buffer);
			obj->streaming_ctrl->uncompress_buffer      = NULL;
			obj->streaming_ctrl->uncompress_buffer_pos  = 0;
			obj->streaming_ctrl->uncompress_buffer_size = 0;
		}
		return max_len;
	}

	if (!obj->streaming_ctrl->inflate_buffer) {
		obj->streaming_ctrl->inflate_buffer = av_malloc(DECOMPRESS_BUF_SIZE);
		if (!obj->streaming_ctrl->inflate_buffer)
			return -1;
	}

	if (obj->streaming_ctrl->inflate_stream.avail_in == 0) {
		int read = net_read(s, obj->streaming_ctrl->inflate_buffer, DECOMPRESS_BUF_SIZE);
		if (read <= 0)
			return read;
		obj->streaming_ctrl->inflate_stream.next_in  = obj->streaming_ctrl->inflate_buffer;
		obj->streaming_ctrl->inflate_stream.avail_in = read;
	}

	obj->streaming_ctrl->inflate_stream.avail_out = max_len;
	obj->streaming_ctrl->inflate_stream.next_out  = buffer;

	ret = inflate(&obj->streaming_ctrl->inflate_stream, Z_SYNC_FLUSH);
	if (ret != Z_OK && ret != Z_STREAM_END)
		gxlogf("inflate return value: %d, %s\n", ret, obj->streaming_ctrl->inflate_stream.msg);

	return max_len - obj->streaming_ctrl->inflate_stream.avail_out;
}

static int http_read(GxStream*  s, uint8_t * buffer, size_t max_len)
{
	GxStreamHttp* obj = (GxStreamHttp *) (s);

	if (obj->streaming_ctrl->compress)
		return net_compress_read(s, buffer, max_len);

	return net_read(s, buffer, max_len);
}

void http_close(GxStream*  stream)
{
#if STREAM_HTTP_PVR_DEBUG
	if(stream_http_pvr_fd)
		fclose(stream_http_pvr_fd);
	stream_http_pvr_fd = NULL;
#endif
	GxStreamHttp* s = (GxStreamHttp *) (stream);

	if(s->streaming_ctrl) {
		if(s->streaming_ctrl->url &&
				(strcasecmp(s->streaming_ctrl->url->protocol, "https") == 0)) {
			if(s->https) {
				https_close(s->https);
				av_free(s->https);
				s->https = NULL;
				https_deinit();
			}
		}
		if(s->streaming_ctrl) {
			if (s->streaming_ctrl->compress) {
				if (s->streaming_ctrl->uncompress_buffer) {
					av_free(s->streaming_ctrl->uncompress_buffer);
					s->streaming_ctrl->uncompress_buffer = NULL;
				}
				inflateEnd(&s->streaming_ctrl->inflate_stream);
				av_freep(&s->streaming_ctrl->inflate_buffer);
			}
			streaming_ctrl_free(s->streaming_ctrl);
			stream->streaming_ctrl = s->streaming_ctrl = NULL;
		}
	}

	if(stream->fd >= 0) {
		closesocket(stream->fd);
		stream->fd = -1;
	}
	stream->interrupt_cbk = NULL;
}

void http_closesocket(GxStream* stream)
{
	if(stream->fd >= 0) {
		closesocket(stream->fd);
		stream->fd = -1;
	}
}

int http_rollback(GxStream* stream, char* buffer, int size)
{
	GxStreamingCtrl* streaming_ctrl = stream->streaming_ctrl;

	if (!streaming_ctrl) {
		return GX_PLAYER_ERROR;
	}

	if (streaming_ctrl->compress) {
		if (streaming_ctrl->uncompress_buffer)
			av_free(streaming_ctrl->uncompress_buffer);
		streaming_ctrl->uncompress_buffer = av_malloc(size);
		memcpy(streaming_ctrl->uncompress_buffer, buffer, size);
		streaming_ctrl->uncompress_buffer_pos  = 0;
		streaming_ctrl->uncompress_buffer_size = size;
		return GX_PLAYER_OK;
	}

	if (streaming_ctrl->res_len >= 0)
		streaming_ctrl->res_len += size;
	if (streaming_ctrl->chk_size >= 0)
		streaming_ctrl->chk_size += size;
	if ((size <= streaming_ctrl->buffer_pos)&&(streaming_ctrl->buffer)) {
		streaming_ctrl->buffer_pos -= size;
	} else
		streaming_bufferize(streaming_ctrl, buffer, size);

#if STREAM_HTTP_PVR_DEBUG
	if(stream_http_pvr_fd)
		fclose(stream_http_pvr_fd);
	stream_http_pvr_fd = fopen(stream_http_pvr_file, "w+");
#endif
	return GX_PLAYER_OK;
}

static int http_control(GxStream *s, int cmd, void *arg)
{
	switch (cmd) {
	case GX_STREAM_CTRL_GET_SIZE:
		{
			if (s->streaming_ctrl->compress)
				*(off_t*)arg = -1;
			else if (s->streaming_ctrl->con_len >= 0)
				*(off_t*)arg = s->streaming_ctrl->con_len;
			else if (s->streaming_ctrl->chk_size > 0)
				*(off_t*)arg = s->streaming_ctrl->chk_size;
			else if (s->streaming_ctrl->con_len < 0)
				*(off_t*)arg = -1;
			else
				*(off_t*)arg = 0;
			return GX_PLAYER_OK;
		}
	case GX_STREAM_CTRL_GET_RES_SIZE:
		{
			if (s->streaming_ctrl->compress)
				*(off_t*)arg = -1;
			else if (s->streaming_ctrl->res_len >= 0)
				*(off_t*)arg = s->streaming_ctrl->res_len;
			else if (s->streaming_ctrl->chk_size > 0)
				*(off_t*)arg = s->streaming_ctrl->chk_size;
			else if (s->streaming_ctrl->con_len < 0)
				*(off_t*)arg = -1;
			else
				*(off_t*)arg = 0;
			return GX_PLAYER_OK;
		}
	case GX_STREAM_CTRL_ROLL_BACK:
		{
			GxStream_RollBack* rol = arg;
			if(!rol)
				return GX_PLAYER_ERROR;
			http_rollback(s, rol->buffer, rol->size);
			return GX_PLAYER_OK;
		}
	case GX_STREAM_CTRL_GET_TOTAL_TIME:
		{
			if(s->streaming_ctrl->duration > 0) {
				*(int*)arg = s->streaming_ctrl->duration;
				return GX_PLAYER_OK;
			}
		}
	default:
		break;
	}

	return GX_PLAYER_ERROR;
}

GxStreamClass gx_stream_http1 = {
	._inherit = {
		._inherit = {
			.name   = "StreamHttp1",
			.parent = &gx_streambase,
			.size   = sizeof(GxStreamHttp),
		},
	},
	.protocols = {"http", "http_proxy", "unsv", "https", NULL},
	DEF_AUTHOR("Stream","http", "No description","L.F","No comment"),

	.flags   = GX_STREAM_NEED_PROBE,
	.open    = open_http1,
	.read    = http_read,
	.write   = NULL,
	.seek    = net_seek,
	.control = http_control,
	.close   = http_close,
};
