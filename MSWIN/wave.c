/* wave.c
 * -- Simple tone generation.
 * -- To build: "cc -c -d -DSTRICT wave.c"
 *
 * From Windows/DOS Developer's Journal, September 1994,
 * Windows Questions & Answers, by Paul Bonneau.
 *
 * Eyal's note: while correct and apropriate for the context where
 * it was published, for Fly8 this method is too time consuming
 * to be useful. The peoper way is to have a static wavetable and
 * adjust the sampling rate to achieve the desired tone. If the
 * sampling rate range is limited then a small number of static
 * wave tables should be used.
*/

#include "config.h"
#if HAVE_WAVE

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <math.h>
#include <limits.h>

#include "wave.h"


/* 0	sine()
 * 1	square
 * 2	fast square
*/
#define WAVE_TYPE	2

/* Available sample rates.
*/
#define cspsLo		11025U
#define cspsMid		22050U
#define cspsHi		44100U

/* Available sample sizes.
*/
#define cbitLo		8
#define cbitHi		16

/* Types.
*/
typedef struct {
	DWORD	lwTag;		/* Tag. */
	DWORD	lcb;		/* Size of data. */
}	CNK;			/* ChuNK. */

typedef struct {
	CNK	cnkRiff;	/* 'RIFF' chunk. */
	DWORD	lwWaveId;	/* Must be 'WAVE'. */
	CNK	cnkFormat;	/* Wave specific format chunk. */
	PCMWAVEFORMAT wft;	/* PCM stuff. */
	CNK	cnkData;	/* PCM Data chunk. */
	BYTE	rgb[1];		/* The sound data. */
}	SND;			/* SouND. */

/* Constants.
*/
#define rFreqMin	20.f    /* Allowable frequency range. */
#define rFreqMax	20000.f
#define cbBufMax	(0x00001000 - sizeof(SND))

/* Globals.
*/
static SND	far *lpsnd = 0;

#if 0 == WAVE_TYPE
static float	r2Pi;
#endif

/* Initialize this module.
*/
extern BOOL FAR
FInitSnd (void)
{
#if 0 == WAVE_TYPE
    r2Pi = (float)(asin(1) * 4.0);
#endif

    if (NULL == (lpsnd = (SND far *)GlobalAllocPtr(GMEM_MOVEABLE |
      GMEM_SHARE, 0x00010000)))
        return FALSE;

    lpsnd->cnkRiff.lwTag = *(long *)"RIFF";
    lpsnd->cnkRiff.lcb = sizeof(SND) - sizeof(CNK);
    lpsnd->lwWaveId = *(long *)"WAVE";
    lpsnd->cnkFormat.lwTag = *(long *)"fmt ";
    lpsnd->cnkFormat.lcb = sizeof(PCMWAVEFORMAT);
    lpsnd->wft.wf.wFormatTag = WAVE_FORMAT_PCM;
    lpsnd->cnkData.lwTag = *(long *)"data";
    return TRUE;
}

/* Close this module.
*/
extern void FAR
CloseSnd (void)
{
    if (NULL != lpsnd) {
        GlobalFreePtr(lpsnd);
        lpsnd = NULL;
    }
}

/* Play the given sound.
 * rFreq : Frequency of sound (Hz).
 * cms   : Duration (ms).
 * csps  : Sampling rate (Hz).
 * cbit  : Sample size (bits/sample/channel).
 * cchn  : Number of channels.
*/
static BOOL
FPlaySnd (float rFreq, long cms, UINT csps, UINT cbit, UINT cchn)
{
    float r;         /* General purpose. */
    float csmpCycle; /* Samples per tone cycle. */
    UINT  cbSample;  /* Bytes/sample (per channel). */
    DWORD cbBufReq;  /* Required buffer size. */
    UINT  csmpBuf;   /* Sample count in buffer. */
    UINT  ismp, ismpT; /* Sample indices. */
    UINT  ichn;      /* Channel index. */
    DWORD mscStart;  /* Tone onset time. */

    if (rFreq < rFreqMin)
	rFreq = rFreqMin;
    else if (rFreq > rFreqMax)
	rFreq = rFreqMax;

    csmpCycle = csps / rFreq;
    csmpBuf = (UINT)(csmpCycle + 0.5);
    cbSample = cbit / 8;
    cbBufReq = csmpBuf * cbSample * cchn;
    if (cbBufMax < cbBufReq)
        return FALSE; /* Too big. */

    lpsnd->wft.wf.nChannels = cchn;
    lpsnd->wft.wf.nSamplesPerSec = csps;
    lpsnd->wft.wf.nAvgBytesPerSec = csps * cchn * cbSample;
    lpsnd->wft.wf.nBlockAlign = cchn * cbSample;
    lpsnd->wft.wBitsPerSample = cbit;
    lpsnd->cnkData.lcb = cbBufReq;
    lpsnd->cnkRiff.lcb = cbBufReq + sizeof(SND) - sizeof(CNK);

#if 2 == WAVE_TYPE
    memset (lpsnd->rgb,           0x00, csmpBuf/2);
    memset (lpsnd->rgb+csmpBuf/2, 0xff, csmpBuf-csmpBuf/2);
#else
    for (ismp = 0; ismp < csmpBuf; ismp++) {
#if 0 == WAVE_TYPE
        r = (float)sin(r2Pi * ismp / csmpCycle);
#elif
	r = (ismp*2 > csmpBuf) ? 0.99 : -1.0;
#endif
        for (ichn = 0; ichn < cchn; ichn++) {
            ismpT = ismp * cchn + ichn;
            if (1 == cbSample)
                lpsnd->rgb[ismpT] = 128 + (int)(128.0 * r);
            else
                ((LPWORD)lpsnd->rgb)[ismpT] = (int)(SHRT_MAX * r);
        }
    }
#endif
    sndPlaySound((LPCSTR)lpsnd, SND_ASYNC | SND_MEMORY | SND_LOOP);

    if (cms > 0) {
        mscStart = timeGetTime();
        while (timeGetTime() - mscStart < (DWORD)cms)
            ;
    }

    return TRUE;
}

extern void FAR
Fly8PlaySnd (int Freq)
{
	FPlaySnd ((float)Freq, -1L, cspsLo, 8, 1);
}

/* Stop playing the current sound.
*/
extern void FAR
StopSnd (void)
{
    sndPlaySound((LPCSTR)NULL, SND_ASYNC);
}
#endif /* if HAVE_WAVE */
