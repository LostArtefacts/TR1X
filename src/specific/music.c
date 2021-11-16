#include "specific/music.h"

#include "global/vars_platform.h"
#include "log.h"

static uint32_t m_MCIDeviceID;
static uint32_t m_AUXDeviceID;
static int32_t m_NumTracks;

bool S_Music_Init()
{
    MCI_OPEN_PARMS open_parms;
    open_parms.dwCallback = 0;
    open_parms.wDeviceID = 0;
    open_parms.lpstrDeviceType = (LPSTR)MCI_DEVTYPE_CD_AUDIO;
    open_parms.lpstrElementName = NULL;
    open_parms.lpstrAlias = NULL;

    MCIERROR result = mciSendCommandA(
        0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID, (DWORD_PTR)&open_parms);
    if (result) {
        LOG_ERROR("cannot initailize music device: %x", result);
        return false;
    }
    m_MCIDeviceID = open_parms.wDeviceID;

    m_AUXDeviceID = 0;
    for (int i = 0; i < (int)auxGetNumDevs(); i++) {
        AUXCAPSA caps;
        auxGetDevCapsA((UINT_PTR)i, &caps, sizeof(AUXCAPSA));
        if (caps.wTechnology == AUXCAPS_CDAUDIO) {
            m_AUXDeviceID = i;
        }
    }

    MCI_STATUS_PARMS status_parms;
    status_parms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    mciSendCommandA(
        m_MCIDeviceID, MCI_STATUS, MCI_STATUS_ITEM, (DWORD_PTR)&status_parms);
    m_NumTracks = status_parms.dwReturn;
    return true;
}

void S_Music_SetVolume(int16_t volume)
{
    int32_t volume_aux = volume * 0xFFFF / 0xFF;
    volume_aux |= volume_aux << 16;
    auxSetVolume(m_AUXDeviceID, volume_aux);
}

void S_Music_Pause()
{
    MCIERROR result;
    MCI_GENERIC_PARMS pause_parms;

    result = mciSendCommandA(
        m_MCIDeviceID, MCI_PAUSE, MCI_WAIT, (DWORD_PTR)&pause_parms);
    if (result) {
        LOG_ERROR("Error while calling mciSendCommandA: 0x%lx", result);
    }
}

void S_Music_Unpause()
{
    MCIERROR result;
    MCI_GENERIC_PARMS pause_parms;

    result = mciSendCommandA(
        m_MCIDeviceID, MCI_RESUME, MCI_WAIT, (DWORD_PTR)&pause_parms);
    if (result) {
        LOG_ERROR("Error while calling mciSendCommandA: 0x%lx", result);
    }
}

bool S_Music_Play(int16_t track)
{
    MCI_SET_PARMS set_parms;
    set_parms.dwTimeFormat = MCI_FORMAT_TMSF;
    if (mciSendCommandA(
            m_MCIDeviceID, MCI_SET, MCI_SET_TIME_FORMAT,
            (DWORD_PTR)&set_parms)) {
        return false;
    }

    MCI_PLAY_PARMS open_parms;
    open_parms.dwFrom = track;
    open_parms.dwCallback = (DWORD_PTR)TombHWND;

    DWORD_PTR dwFlags = MCI_NOTIFY | MCI_FROM;
    if (track != m_NumTracks) {
        open_parms.dwTo = track + 1;
        dwFlags |= MCI_TO;
    }

    if (mciSendCommandA(
            m_MCIDeviceID, MCI_PLAY, dwFlags, (DWORD_PTR)&open_parms)) {
        return false;
    }

    return true;
}

bool S_Music_Stop()
{
    MCI_GENERIC_PARMS gen_parms;
    return !mciSendCommandA(
        m_MCIDeviceID, MCI_STOP, MCI_WAIT, (DWORD_PTR)&gen_parms);
}
