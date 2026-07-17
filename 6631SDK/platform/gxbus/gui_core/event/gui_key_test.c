#include "gui_key_test.h"
#include "gui_core.h"

#define KEY_BUF_SIZE 8

extern GuiCore gui;
struct kqueue {
	unsigned int elem[KEY_BUF_SIZE];
	int front;
	int rear;
};

static int queue_isfull(struct kqueue *q)
{
	return q->front == (q->rear + 1) % KEY_BUF_SIZE;
}

static int queue_isempty(struct kqueue *q)
{
	return q->front == q->rear;
}

static int queue_add(struct kqueue *q, unsigned int elm)
{
	if (queue_isfull(q))
		return -1;

	q->elem[q->rear] = elm;
	q->rear = (q->rear + 1) % KEY_BUF_SIZE;

	return 0;
}

static int queue_get(struct kqueue *q, unsigned int *elm)
{
	if (queue_isempty(q))
		return -1;

	*elm = q->elem[q->front];

	q->front = (q->front + 1) % KEY_BUF_SIZE;
	return (*elm);

}

static struct kqueue key_buf = {
	.front = 0,
	.rear = 0
};

int Gui_WriteKeyBuf(unsigned int key)
{
	return queue_add(&key_buf, key);
}

unsigned int Gui_ReadKeyBuf(void)
{
	unsigned int key = 0;

	if (queue_get(&key_buf, &key))
		return key;
	else
		return 0;
}

void Gui_IrrKeyMute(int flag)
{
	gui.config.irr_key_mute = flag;
}

static int _check_key_len(unsigned int *keys)
{
	unsigned int key = 0, time = 0, len = 0;
	unsigned int *key_ptr = keys;

	while(GUIK_NULL != *key_ptr) {
		key = *key_ptr;
		key_ptr++;
		time = *key_ptr;
		key_ptr++;
		len += 2;
	}

	return (len + 1);
}

static int s_key_exit;
static int *sys_keys = NULL;

static void _key_send(void *data)
{
	unsigned int *keys = (unsigned int *)data;
	unsigned int key = 0, time = 0;

	s_key_exit = false;
	Gui_IrrKeyMute(true);

	while(!s_key_exit) {
		key = *keys;
		if(GUIK_NULL == key) {
			keys = (unsigned int *)data;
			key = *keys;
		}
		keys++;
		time = *keys;
		keys++;

		Gui_WriteKeyBuf(key);
		gui_delay(time * 1000);
	}

	Gui_IrrKeyMute(false);
	GUI_FREE(sys_keys);
}

void Gui_SendLoopKey(unsigned int *keys)
{
	int len = 0;
	int key_thread = 0;

	if(NULL == keys) {
		return;
	}

	len = _check_key_len(keys);
	sys_keys = GUI_MALLOCZ(len * sizeof(int unsigned));
	if(NULL == sys_keys) {
		return;
	}
	memcpy(sys_keys, keys, len * sizeof(unsigned int));

	GxCore_ThreadCreate("key_test", &key_thread, _key_send, (void *)sys_keys, 10 * 1024, GXOS_DEFAULT_PRIORITY - 1);
}

int Gui_ExitLoopKey(void)
{
	s_key_exit = true;
	while(NULL != sys_keys) {
		gui_delay(10);
	}

	return (0);
}

