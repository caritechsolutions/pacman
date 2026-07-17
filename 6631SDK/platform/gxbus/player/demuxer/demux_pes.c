#include "demux_pes.h"

static int read_nBytes(MpegPes* pes, unsigned char* buffer, int bytes)
{
	int data_len = (pes->write_pos + PES_BLOCK_SIZE - pes->read_pos)%PES_BLOCK_SIZE;
	int copy_len = 0;

	if(bytes > data_len)
		return -1;

	copy_len = PES_BLOCK_SIZE - pes->read_pos;
	if(copy_len > bytes){
		memcpy(buffer, pes->buffer + pes->read_pos, bytes);
	}else{
		memcpy(buffer, pes->buffer + pes->read_pos, copy_len);
		memcpy(buffer + copy_len, pes->buffer, bytes - copy_len);
	}
	pes->read_pos = (pes->read_pos + bytes)%PES_BLOCK_SIZE;
	return bytes;
}

static int back_nByte(MpegPes* pes, int bytes)
{
	int data_len = (pes->write_pos + PES_BLOCK_SIZE - pes->read_pos)%PES_BLOCK_SIZE;
	int free_len = PES_BLOCK_SIZE - data_len;

	if(bytes > free_len)
		return -1;
	pes->read_pos = (pes->read_pos + PES_BLOCK_SIZE - bytes)%PES_BLOCK_SIZE;
	return 0;
}

static int check_nBytes(MpegPes* pes)
{
	int data_len = (pes->write_pos + PES_BLOCK_SIZE - pes->read_pos)%PES_BLOCK_SIZE;

	return data_len;
}

handle_t MpegPesCreate(void)
{
	MpegPes* pes;

	pes = (MpegPes*)av_malloc(sizeof(MpegPes));
	pes->write_pos = pes->read_pos = 0;

	return (handle_t)pes;
}

void MpegPesDestory(handle_t handle)
{
	MpegPes* pes = (MpegPes*)handle;

	if(pes)
		av_free(pes);
	return;
}

int MpegPesInsertBuffer(handle_t handle, uint8_t* buffer, int size)
{
	MpegPes* pes = (MpegPes*) handle;

	int data_len = (pes->write_pos + PES_BLOCK_SIZE - pes->read_pos)%PES_BLOCK_SIZE;
	int free_len = PES_BLOCK_SIZE - data_len;
	int copy_len = 0;

	if(free_len <= size)
		return -1;

	copy_len = PES_BLOCK_SIZE - pes->write_pos;
	if(copy_len >= size){
		memcpy(pes->buffer + pes->write_pos, buffer, size);
	}else{
		memcpy(pes->buffer + pes->write_pos, buffer, copy_len);
		memcpy(pes->buffer, buffer+copy_len, size - copy_len);
	}
	pes->write_pos = (pes->write_pos + size)%PES_BLOCK_SIZE;
	return 0;
}

void MpegPesClear(handle_t handle)
{
	MpegPes* pes = (MpegPes*) handle;

	pes->write_pos = pes->read_pos = 0;
	return;
}

int MpegPesReadEs(handle_t handle, uint8_t* buffer, int size, int64_t* pts, struct gx_ctrl_info* ctrlinfo)
{
	int read_len = 0, ret;
	unsigned char p[256];
	unsigned char stream_id;
	uint32_t header_len;
	uint32_t is_aligned;
	uint32_t payload_size;
	uint32_t pts_flags;
	MpegPes* pes = (MpegPes*)handle;

	ret = read_nBytes(pes, p, 3);
	if(ret < 0)
		goto es_need_data;
	read_len += 3;

	if (p[0] || p[1] || (p[2] != 1))
		return 0;

	ret = read_nBytes(pes, p, 3);
	if(ret < 0)
		goto es_need_data;

	read_len += 3;
	stream_id = p[0];
	payload_size = (p[1] << 8 | p[2]);
	if(payload_size > check_nBytes(pes))
		goto es_need_data;

	read_nBytes(pes, p, 3);
	is_aligned = (p[0] & 4);
	pts_flags  = p[1]&0x80;
	header_len = p[2];
	if((header_len + 3) >= payload_size)
		return 0;

	if(header_len > 0){
		read_nBytes(pes, p, header_len);
		if (pts_flags) {
			*pts = (int64_t) (p[0] & 0x0E) << 29;
			*pts |= p[1] << 22;
			*pts |= (p[2] & 0xFE) << 14;
			*pts |= p[3] << 7;
			*pts |= (p[4] & 0xFE) >> 1;
		}else
			*pts = GX_NOPTS_VALUE;

		if(header_len > 15){
			unsigned char* ad = p + 6;
			int len = ad[0]&0xf;
			if((len >=8) &&
					(ad[1] == 0x44) && (ad[2] == 0x54) &&
					(ad[3] == 0x47) && (ad[4] == 0x41) &&
					(ad[5] == 0x44))
			{
				if (ctrlinfo) {
					ctrlinfo->ad_tags = 1;
					ctrlinfo->ad_fade_byte = ad[7];
					ctrlinfo->ad_pan_byte  = ad[8];
				}
			}
		}
	}

	payload_size -= (header_len + 3);

	ret = read_nBytes(pes, buffer, payload_size);
	if((ret == -1 ) || (ret != payload_size)){
		gxlogd("waring: payload_size %d, ret %d\n", payload_size, ret);
		return 0;
	}
	return payload_size;

es_need_data:
	if(read_len > 0)
		back_nByte(pes, read_len);
	return -1;
}
