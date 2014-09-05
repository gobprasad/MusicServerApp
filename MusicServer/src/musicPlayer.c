#include "musicPlayer.h"
#include "resourceManager.h"
#include "loggingFrameWork.h"
#include <pthread.h>

#define BITS	8
static MPlayer *mpInstance = NULL;

static void *playSong(void *arg);
MPlayer *getMPlayerInstance()
{
	if(mpInstance == NULL)
	{
		mpInstance = (MPlayer *)malloc(sizeof(MPlayer));
		// Constructor, Initialize mpInstance
		initMplayer(mpInstance);
	}
	return mpInstance;
}

void initMplayer(MPlayer *mp)
{
	int err;
	size_t buffer_size;
	ao_initialize();
	mp->driver = ao_default_driver_id();
	mpg123_init();
	mp->mh = mpg123_new(NULL, &err);
	mp->buffer_size = mpg123_outblock(mp->mh);
	mp->buffer = (unsigned char*) malloc(mp->buffer_size * sizeof(unsigned char));
	mp->stop   = 0;
	mp->playSong = playSong;
}


static void *playSong(void *arg)
{

	rmMsg_t *rmMsg = NULL;
	rmMsg = (rmMsg_t *)malloc(sizeof(rmMsg_t));

	rmMsg->msgId = rm_mplayerDone_m;

	RManager *rm = getRManagerInstance();

	MPlayer *mp = (MPlayer *)arg;
	ao_sample_format format;
	int channels, encoding;
	ao_device *dev;
	long rate;
	size_t done;
	if(mpg123_open(mp->mh, mp->fileName) == MPG123_OK){
		mpg123_getformat(mp->mh, &rate, &channels, &encoding);
	
		format.bits = mpg123_encsize(encoding) * BITS;
		format.rate = rate;
		format.channels = channels;
		format.byte_format = AO_FMT_NATIVE;
		format.matrix = 0;

		dev = ao_open_live(mp->driver, &format, NULL);

		while (mpg123_read(mp->mh, mp->buffer, mp->buffer_size, &done) == MPG123_OK){
			if(mp->stop == 1){
				rmMsg->msgId = rm_mplayerStop_m;
				break;
			}
			ao_play(dev, mp->buffer, done);
		}
		// clean up
		ao_close(dev);
		mpg123_close(mp->mh);
	}
	else
		{
			rmMsg->msgId = rm_mplayerErr_m;
			LOG_ERROR("can not open file %s for playing\n",mp->fileName);
		}
	rm->postMsgToRm(rm, rmMsg);
}
