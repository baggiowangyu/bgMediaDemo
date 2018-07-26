#ifndef _GxxGmMediaPlayer_H_
#define _GxxGmMediaPlayer_H_

/**
 * 本模块负责与UI层对接，主要用于播放窗口的管理以及播放控制
 */
class GxxGmMediaPlayer
{
public:
	GxxGmMediaPlayer();
	~GxxGmMediaPlayer();

public:
	int Open();
};

#endif//_GxxGmMediaPlayer_H_
