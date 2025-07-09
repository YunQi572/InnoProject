#ifndef _music_H
#define _music_H

#include "system.h"

// 音符频率定义（Hz）
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988

// 音乐播放状态
#define MUSIC_STOP    0
#define MUSIC_PLAY    1
#define MUSIC_PAUSE   2

// 音乐结构体
typedef struct {
    u16 frequency;  // 音符频率
    u16 duration;   // 持续时间（ms）
} Note;

// 音乐播放控制变量
extern u8 music_status;
extern u8 current_song;
extern u16 current_note;
extern u16 note_timer;
extern u8 total_songs;

// 函数声明
void Music_Init(void);
void Music_Play_Song(u8 song_index);
void Music_Stop(void);
void Music_Pause(void);
void Music_Resume(void);
void Music_Next_Song(void);
void Music_Prev_Song(void);
void Music_Update(void);

// 音乐数据声明
extern const Note song_little_star[];
extern const Note song_happy_birthday[];
extern const u16 song_little_star_length;
extern const u16 song_happy_birthday_length;

#endif 