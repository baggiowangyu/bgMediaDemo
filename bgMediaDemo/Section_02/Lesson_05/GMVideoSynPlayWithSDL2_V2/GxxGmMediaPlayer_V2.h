#ifndef _GxxGmMediaPlayer_V2_H_
#define _GxxGmMediaPlayer_V2_H_

class GxxGmMediaPlayer_V2
{
public:
	GxxGmMediaPlayer_V2();
	~GxxGmMediaPlayer_V2();

public:
	int Initialize(void* screen);
	int Initialize(int width, int height);

public:
	int Play(const char *url);
	void Close();

	void Pause();
	void Resume();
};

#endif//_GxxGmMediaPlayer_V2_H_
