#ifndef __JUKEBOX_H__
#define __JUKEBOX_H__

typedef struct jbox
{
	char isActive;
	char fileName[2048];
	void (*startJukeBoxPlayer)(struct jbox *);
	void (*stopJukeBoxPlayer)(struct jbox *);
	void (*scheduleNextRandomSong)(struct jbox *);
}JBoxPlayer;

JBoxPlayer *getJBoxPlayerInstance();


#endif // __JUKEBOX_H__
