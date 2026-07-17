#include "speed_detect.h"

SpeedDetect* speed_detect_init()
{
	SpeedDetect *speed_detect = (SpeedDetect*)av_malloc(sizeof(SpeedDetect));
	if (speed_detect == NULL)
		return NULL;

	memset(speed_detect, 0, sizeof(SpeedDetect));

	return speed_detect;
}

int speed_detect_set(SpeedDetect *speed_detect, int count, SpeedDetectMode mode)
{
	if (speed_detect == NULL)
		return -1;

	if ((mode & MODE_FUNC) == MODE_FUNC) {
		speed_detect->mode |= MODE_FUNC;
		GxCore_GetTickTime(&(speed_detect->func_stime));
	}

	if ((mode & MODE_TOTAL) == MODE_TOTAL) {
		if (count == 0)
			return -1;

		if (speed_detect->cur_count == 0) {
			speed_detect->mode |= MODE_TOTAL;
			speed_detect->exec_count = count;
			GxCore_GetTickTime(&(speed_detect->total_stime));
		}
	}

	return 0;
}

int speed_detect_produce(SpeedDetect *speed_detect, int size)
{
	if (speed_detect == NULL)
		return -1;

	if ((speed_detect->mode & MODE_FUNC) == MODE_FUNC) {
		GxCore_GetTickTime(&(speed_detect->func_etime));
		speed_detect->costms_func += TIME_MINUS(speed_detect->func_etime, speed_detect->func_stime);
	}

	if ((speed_detect->mode & MODE_TOTAL)) {
		if (speed_detect->cur_count >= speed_detect->exec_count) {
			GxCore_GetTickTime(&(speed_detect->total_etime));
			speed_detect->costms_total = TIME_MINUS(speed_detect->total_etime, speed_detect->total_stime);
			speed_detect->cur_count = 0;
			speed_detect->data_size = 0;
		} else {
			speed_detect->cur_count++;
		}
	}

	speed_detect->data_size += size;

	return 0;
}

int speed_detect_gxlogd(SpeedDetect *speed_detect)
{
	if (speed_detect == NULL)
		return -1;

	if ((speed_detect->mode & MODE_TOTAL) == MODE_TOTAL) {
		if (speed_detect->cur_count == speed_detect->exec_count) {
			gxlogd("Func: %s, exec_count %d, \tsize_total %dKB, \ttotal_time %dms, \ttotal_speed %fMB/s\n",
					(speed_detect->func==FUNC_READ)?"RD":"WT",
					speed_detect->exec_count,
					speed_detect->data_size/1024,
					speed_detect->costms_total,
					(speed_detect->costms_total != 0)?((float)(speed_detect->data_size/1024)/(speed_detect->costms_total)):0);
		}
	}

	if ((speed_detect->mode & MODE_FUNC) == MODE_FUNC) {
		gxlogd("Func: %s, exec_count 1, \ttsize_total %dKB, \tfunc_time %dms, \tfunc_speed %fMB/s\n",
				(speed_detect->func==FUNC_READ)?"RD":"WT",
				speed_detect->data_size/1024,
				speed_detect->costms_func,
				(speed_detect->costms_func != 0)?((float)(speed_detect->data_size/1024)/(speed_detect->costms_func)):0);
	}

	return 0;
}

int speed_detect_consume(SpeedDetect *speed_detect)
{
	if (speed_detect == NULL)
		return -1;

	speed_detect->data_size    = 0;
	speed_detect->cur_count    = 0;
	speed_detect->exec_count   = 0;
	speed_detect->costms_func  = 0;
	speed_detect->costms_total = 0;
	memset(&(speed_detect->func_stime),  0, sizeof(GxTime));
	memset(&(speed_detect->func_etime),  0, sizeof(GxTime));
	memset(&(speed_detect->total_stime), 0, sizeof(GxTime));
	memset(&(speed_detect->total_etime), 0, sizeof(GxTime));

	return 0;
}

int speed_detect_uninit(SpeedDetect *speed_detect)
{
	if (speed_detect)
		av_free(speed_detect);

	return 0;
}
