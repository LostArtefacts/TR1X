#include "game/music.h"

#include "config.h"
#include "game/sound.h"
#include "global/vars.h"
#include "specific/music.h"

static bool m_Loop = false;
static int16_t m_Track = 0;
static int16_t m_TrackLooped = 0;

bool Music_Init()
{
    return S_Music_Init();
}

bool Music_Play(int16_t track)
{
    if (CurrentLevel == GF.title_level_num && T1MConfig.disable_music_in_menu) {
        return false;
    }

    if (track <= 1) {
        return false;
    }

    if (track >= 57) {
        m_TrackLooped = track;
    }

    m_Loop = false;

    if (T1MConfig.fix_secrets_killing_music && track == 13) {
        return Sound_Effect(SFX_SECRET, NULL, SPM_ALWAYS);
    }

    if (track == 0) {
        S_Music_Stop();
        return false;
    }

    if (track == 5) {
        return false;
    }

    m_Track = track;
    return S_Music_Play(track);
}

void Music_PlayLooped()
{
    if (m_Loop && m_TrackLooped > 0) {
        S_Music_Play(m_TrackLooped);
    }
}

bool Music_Stop()
{
    m_Track = 0;
    m_TrackLooped = 0;
    m_Loop = false;
    return S_Music_Stop();
}

void Music_Loop()
{
    m_Loop = true;
}

void Music_SetVolume(int16_t volume)
{
    int16_t volume_raw = volume ? 25 * volume + 5 : 0;
    S_Music_SetVolume(volume_raw);
}

void Music_Pause()
{
    S_Music_Pause();
}

void Music_Unpause()
{
    S_Music_Unpause();
}

int16_t Music_CurrentTrack()
{
    return m_Track;
}
