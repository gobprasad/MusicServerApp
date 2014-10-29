#include "jukebox.h"
#include "Gthreads.h"
#include <dirent.h>
#include <string.h>


#define JUKEBOX_ROOT_DIR  "/mnt/hdd/HDD/HindiSongs"

static JBoxPlayer *jbInstance = NULL;

static void initjbPlayer(JBoxPlayer *jb);


static void scheduleNextRandomSongFunc(JBoxPlayer *jb);
static void startJukeBoxPlayerFunc(JBoxPlayer *jb);
static void stopJukeBoxPlayerFunc(JBoxPlayer *jb);


static int checkMP3File(const char *);


int filterDirectory(const struct dirent *dir);


typedef enum
{
	JB_NONE = 1,
	JB_DIR,
	JB_MP3,
	JB_LAST
}JB_FILE_TYPE;

JBoxPlayer *getJBoxPlayerInstance()
{
	if(jbInstance == NULL)
	{
		jbInstance = (JBoxPlayer *)malloc(sizeof(JBoxPlayer));
		// Constructor, Initialize jbInstance
		initjbPlayer(jbInstance);
	}
	return jbInstance;
}

static void initjbPlayer(JBoxPlayer *jb)
{
	jb->isActive = 0;
	memset(jb->fileName,0, sizeof(jb->fileName));
	jb->scheduleNextRandomSong = scheduleNextRandomSongFunc;
	jb->startJukeBoxPlayer     = startJukeBoxPlayerFunc;
	jb->stopJukeBoxPlayer      = stopJukeBoxPlayerFunc;
}


static void scheduleNextRandomSongFunc(JBoxPlayer *jb)
{
	MPlayer *mp = getMPlayerInstance();
	char oldDir[2048] = {0};
	JB_FILE_TYPE retType = JB_NONE;
	memset(jb->fileName,0, sizeof(jb->fileName));
	sprintf(jb->fileName,"%s",JUKEBOX_ROOT_DIR);
	sprintf(oldDir,"%s",JUKEBOX_ROOT_DIR);
	while( (retType = getRandomDir(jb->fileName)) != JB_MP3)
	{
		if(retType == JB_NONE)
		{
			LOG_ERROR("getRandom Dir return NULL type");
			sprintf(jb->fileName,"%s",oldDir);
			continue;
		}
		sprintf(oldDir,"%s",jb->fileName);
	}
	jb->isActive = 1;

	mp->fileName = jb->fileName;
	LOG_MSG("JUKE_BOX Sending file %s to MP3 Player for playing", mp->fileName);
	if(addJobToQueue(mp->playSong,(void *)mp) != G_OK)
	{
		LOG_ERROR("Fatal Error cant sent to job queue");
	}
}

static JB_FILE_TYPE getRandomDir(char *rootDir)
{
	struct dirent **namelist;
	JB_FILE_TYPE retType = JB_NONE;
    int num;
	int selected = 0;
	unsigned int iseed = (unsigned int)time(NULL);
  	srand (iseed);

   	num = scandir(rootDir, &namelist, filterDirectory, alphasort);
    if (num <= 0)
    {
        LOG_ERROR("scandir");
		return retType;
    }
	selected = rand()%num;
	if(namelist[selected]->d_type == DT_DIR)
	{
		retType = JB_DIR;
		sprintf(rootDir,"%s/%s",rootDir,namelist[selected]->d_name);
	}
	else if((namelist[selected]->d_type == DT_REG) && (checkMP3File(namelist[selected]->d_name)))
	{
		retType = JB_MP3;
		sprintf(rootDir,"%s/%s",rootDir,namelist[selected]->d_name);
	}
	else
	{
		LOG_ERROR("No Directory or MP3 file found");
	}

	// cleanup
    while (num--) 
	{
    	free(namelist[num]);
    }
    free(namelist);
	return retType;
}

int filterDirectory(const struct dirent *dir)
{
	if((strcmp(dir->d_name,".") == 0) || (strcmp(dir->d_name,"..") == 0))
		return 0;

	return 1;
}


static int checkMP3File(const char *fileName)
{

	int length = 0;
	length = strlen(fileName);
	if(length < 5)
		return 0;
	
	if(strcmp(fileName+(length-4),".mp3") == 0)
		return 1;

	return 0;
}




#if 0
static JB_FILE_TYPE getRandomDir(char *rootDir)
{
	int folder_count = 0;
	int file_count   = 0;
	DIR * dirp;
	JB_FILE_TYPE retType = JB_NONE;
	struct dirent * entry;
	
	dirp = opendir(rootDir); /* There should be error handling after this */
	while ((entry = readdir(dirp)) != NULL) {
		if (entry->d_type == DT_DIR) { /* If the entry is a regular file */
			 folder_count++;
		}
		else if(entry->d_type == DT_REG)
		{
			file_count++;
		}
	}
	closedir(dirp);

	unsigned int selectedFolder = 0;
	unsigned int counter = 0;
	unsigned int iseed = (unsigned int)time(NULL);
  	srand (iseed);

	// Counting done now random function will starts from here
	if(folder_count > 0)
	{
		selectedFolder = (rand()%folder_count)+1;

		dirp = opendir(rootDir); /* There should be error handling after this */
		while ((entry = readdir(dirp)) != NULL) {
			if (entry->d_type == DT_DIR) { /* If the entry is a regular file */
				 counter++;
				 if(counter == selectedFolder)
				 {
					fprintf(rootDir,"%s/%s",rootDir,entry->d_name);
					retType = JB_DIR;
					break;
				 }
				 	
			}
		}
		closedir(dirp);
	}
	else
	{
		selectedFolder = (rand()%file_count)+1;

		dirp = opendir(rootDir); /* There should be error handling after this */
		while ((entry = readdir(dirp)) != NULL) {
			if (entry->d_type == DT_REG) { /* If the entry is a regular file */
				 counter++;
				 if(counter == selectedFolder)
				 {
					fprintf(rootDir,"%s/%s",rootDir,entry->name);
					retType = JB_MP3;
					break;
				 }
			}
		}
		closedir(dirp);
	}

	return retType;
}
#endif

static void startJukeBoxPlayerFunc(JBoxPlayer *jb)
{
	scheduleNextRandomSongFunc(jb);
}

static void stopJukeBoxPlayerFunc(JBoxPlayer *jb)
{
	MPlayer *mp = getMPlayerInstance();
	mp->stop = 1;
}
