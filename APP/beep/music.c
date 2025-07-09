#include "music.h"
#include "beep.h"
#include "SysTick.h"

// 音乐播放控制变量
u8 music_status = MUSIC_STOP;
u8 current_song = 0;
u16 current_note = 0;
u16 note_timer = 0;
u8 total_songs = 2; // 总共有2首歌
u8 music_speed = 3; // 固定3倍速播放

// 小星星音乐数据 - 加快播放速度
const Note song_little_star[] = {
    {NOTE_C4, 200}, {NOTE_C4, 200}, {NOTE_G4, 200}, {NOTE_G4, 200},
    {NOTE_A4, 200}, {NOTE_A4, 200}, {NOTE_G4, 400},
    {NOTE_F4, 200}, {NOTE_F4, 200}, {NOTE_E4, 200}, {NOTE_E4, 200},
    {NOTE_D4, 200}, {NOTE_D4, 200}, {NOTE_C4, 400},
    {NOTE_G4, 200}, {NOTE_G4, 200}, {NOTE_F4, 200}, {NOTE_F4, 200},
    {NOTE_E4, 200}, {NOTE_E4, 200}, {NOTE_D4, 400},
    {NOTE_G4, 200}, {NOTE_G4, 200}, {NOTE_F4, 200}, {NOTE_F4, 200},
    {NOTE_E4, 200}, {NOTE_E4, 200}, {NOTE_D4, 400},
    {NOTE_C4, 200}, {NOTE_C4, 200}, {NOTE_G4, 200}, {NOTE_G4, 200},
    {NOTE_A4, 200}, {NOTE_A4, 200}, {NOTE_G4, 400},
    {NOTE_F4, 200}, {NOTE_F4, 200}, {NOTE_E4, 200}, {NOTE_E4, 200},
    {NOTE_D4, 200}, {NOTE_D4, 200}, {NOTE_C4, 400}
};

const u16 song_little_star_length = sizeof(song_little_star) / sizeof(Note);

// 生日快乐歌音乐数据 - 加快播放速度
const Note song_happy_birthday[] = {
    {NOTE_C4, 150}, {NOTE_C4, 150}, {NOTE_D4, 300}, {NOTE_C4, 300}, {NOTE_F4, 300}, {NOTE_E4, 600},
    {NOTE_C4, 150}, {NOTE_C4, 150}, {NOTE_D4, 300}, {NOTE_C4, 300}, {NOTE_G4, 300}, {NOTE_F4, 600},
    {NOTE_C4, 150}, {NOTE_C4, 150}, {NOTE_C5, 300}, {NOTE_A4, 300}, {NOTE_F4, 300}, {NOTE_E4, 300}, {NOTE_D4, 600},
    {NOTE_A4, 150}, {NOTE_A4, 150}, {NOTE_G4, 300}, {NOTE_F4, 300}, {NOTE_G4, 300}, {NOTE_F4, 600}
};

const u16 song_happy_birthday_length = sizeof(song_happy_birthday) / sizeof(Note);

// 音乐数组指针
const Note* song_list[] = {song_little_star, song_happy_birthday};
const u16 song_lengths[] = {song_little_star_length, song_happy_birthday_length};

// 音乐名称
const char* song_names[] = {"Little Star", "Happy Birthday"};

/*******************************************************************************
* 函数名         : Music_Init
* 功能描述       : 音乐播放模块初始化
* 输入参数       : 无
* 输出参数       : 无
*******************************************************************************/
void Music_Init(void)
{
    music_status = MUSIC_STOP;
    current_song = 0;
    current_note = 0;
    note_timer = 0;
    music_speed = 3; // 固定3倍速播放
}

/*******************************************************************************
* 函数名         : Music_Play_Song
* 功能描述       : 播放指定歌曲
* 输入参数       : song_index - 歌曲索引
* 输出参数       : 无
*******************************************************************************/
void Music_Play_Song(u8 song_index)
{
    extern u8 beep_status; // 将声明移到函数开头
    
    if (song_index < total_songs)
    {
        current_song = song_index;
        current_note = 0;
        note_timer = 0;
        music_status = MUSIC_PLAY;
        
        // 设置第一个音符的频率
        if (song_lengths[song_index] > 0)
        {
            u16 freq = song_list[song_index][0].frequency;
            // 通过改变PWM频率来改变音调
            u16 period = 1000000 / freq; // 1MHz / freq
            if (period > 0)
            {
                TIM_SetAutoreload(TIM4, period - 1);
                TIM_SetCompare3(TIM4, period / 2); // 50%占空比
                // 启动蜂鸣器
                beep_status = 1;
            }
        }
        
        printf("Playing song: %s\r\n", song_names[song_index]);
    }
}

/*******************************************************************************
* 函数名         : Music_Stop
* 功能描述       : 停止音乐播放
* 输入参数       : 无
* 输出参数       : 无
*******************************************************************************/
void Music_Stop(void)
{
    music_status = MUSIC_STOP;
    current_note = 0;
    note_timer = 0;
    BEEP_Off();
    printf("Music stopped\r\n");
}

/*******************************************************************************
* 函数名         : Music_Pause
* 功能描述       : 暂停音乐播放
* 输入参数       : 无
* 输出参数       : 无
*******************************************************************************/
void Music_Pause(void)
{
    if (music_status == MUSIC_PLAY)
    {
        music_status = MUSIC_PAUSE;
        BEEP_Off();
        printf("Music paused\r\n");
    }
}

/*******************************************************************************
* 函数名         : Music_Resume
* 功能描述       : 恢复音乐播放
* 输入参数       : 无
* 输出参数       : 无
*******************************************************************************/
void Music_Resume(void)
{
    if (music_status == MUSIC_PAUSE)
    {
        music_status = MUSIC_PLAY;
        // 恢复当前音符
        if (current_note < song_lengths[current_song])
        {
            u16 freq = song_list[current_song][current_note].frequency;
            u16 period = 1000000 / freq;
            if (period > 0)
            {
                TIM_SetAutoreload(TIM4, period - 1);
                TIM_SetCompare3(TIM4, period / 2);
            }
        }
        printf("Music resumed\r\n");
    }
}

/*******************************************************************************
* 函数名         : Music_Next_Song
* 功能描述       : 播放下一首歌
* 输入参数       : 无
* 输出参数       : 无
*******************************************************************************/
void Music_Next_Song(void)
{
    u8 next_song = (current_song + 1) % total_songs;
    Music_Play_Song(next_song);
    printf("Next song: %s\r\n", song_names[next_song]);
}

/*******************************************************************************
* 函数名         : Music_Prev_Song
* 功能描述       : 播放上一首歌
* 输入参数       : 无
* 输出参数       : 无
*******************************************************************************/
void Music_Prev_Song(void)
{
    u8 prev_song = (current_song == 0) ? (total_songs - 1) : (current_song - 1);
    Music_Play_Song(prev_song);
    printf("Previous song: %s\r\n", song_names[prev_song]);
}

/*******************************************************************************
* 函数名         : Music_Update
* 功能描述       : 音乐播放更新函数，需要在主循环中调用
* 输入参数       : 无
* 输出参数       : 无
*******************************************************************************/
void Music_Update(void)
{
    extern u8 beep_status; // 将声明移到函数开头
    
    if (music_status == MUSIC_PLAY)
    {
        if (current_song < total_songs && current_note < song_lengths[current_song])
        {
            note_timer += 50 * music_speed; // 根据速度倍率调整更新间隔
            
            if (note_timer >= song_list[current_song][current_note].duration)
            {
                // 当前音符播放完毕，切换到下一个音符
                current_note++;
                note_timer = 0;
                
                printf("Note %d/%d completed\r\n", current_note, song_lengths[current_song]);
                
                if (current_note < song_lengths[current_song])
                {
                    // 设置下一个音符的频率
                    u16 freq = song_list[current_song][current_note].frequency;
                    u16 period = 1000000 / freq;
                    if (period > 0)
                    {
                        TIM_SetAutoreload(TIM4, period - 1);
                        TIM_SetCompare3(TIM4, period / 2);
                        // 确保蜂鸣器状态正确
                        beep_status = 1;
                        printf("Playing note: %d Hz\r\n", freq);
                    }
                }
                else
                {
                    // 歌曲播放完毕，停止播放
                    printf("Song completed\r\n");
                    Music_Stop();
                }
            }
        }
    }
} 