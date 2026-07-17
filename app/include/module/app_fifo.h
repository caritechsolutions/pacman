#ifndef __APP_FIFO_H__
#define __APP_FIFO_H__

status_t app_fifo_create(handle_t *fifo, unsigned int block_size, unsigned int block_num);
status_t app_fifo_destroy(handle_t fifo);
unsigned int app_fifo_write(handle_t fifo, char *write_data,  unsigned int write_size);
unsigned int app_fifo_read(handle_t fifo, char *read_data,  unsigned int read_size);

#endif
