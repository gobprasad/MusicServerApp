#ifndef __MUSIC_PLAYER_H__
#define __MUSIC_PLAYER_H__
#include <ao/ao.h>
#include <mpg123.h>

typedef struct mplayer
{
	
	mpg123_handle *mh;
	ao_sample_format format;
	int driver;
	unsigned char *buffer;
	size_t buffer_size;
	char *fileName;

	// for stop current Playing
	char stop;
	
	// playsong function pointer 
	void *(*playSong)(void *);
}MPlayer;

MPlayer *getMPlayerInstance();

#endif //__MUSIC_PLAYER_H__
