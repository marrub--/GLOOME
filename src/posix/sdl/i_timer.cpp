
// Moved from sdl/i_system.cpp

#include <assert.h>
#include <signal.h>
#include <sys/time.h>

#include <SDL.h>

#include "basictypes.h"
#include "basicinlines.h"
#include "hardware.h"
#include "i_system.h"
#include "templates.h"


static DWORD TicStart;
static DWORD BaseTime;
static int TicFrozen;

// Signal based timer.
static Semaphore timerWait;
static int tics;
static DWORD sig_start;

// [ZK] Variable ticrate
static int ticAdjust;
int I_SetTicAdjust(int a);
int I_GetTicAdjust();

void I_SelectTimer();

//==========================================================================
//
// I_SetTicAdjust
//
//==========================================================================

int I_SetTicAdjust(int a)
{
	ticAdjust = abs(a);
	return ticAdjust;
}

//==========================================================================
//
// I_GetTicAdjust
//
//==========================================================================

int I_GetTicAdjust()
{
	return ticAdjust;
}

// [RH] Returns time in milliseconds
unsigned int I_MSTime (void)
{
	unsigned int time = SDL_GetTicks ();
	return time - BaseTime;
}

// Exactly the same thing, but based does no modification to the time.
unsigned int I_FPSTime()
{
	return SDL_GetTicks();
}

//
// I_GetTime
// returns time in 1/35th second tics
//
int I_GetTimeSelect (bool saveMS)
{
	I_SelectTimer();
	return I_GetTime (saveMS);
}

int I_GetTimePolled (bool saveMS)
{
	if (TicFrozen != 0)
	{
		return TicFrozen;
	}

	DWORD tm = SDL_GetTicks();

	if (saveMS)
	{
		TicStart = tm;
	}
	return Scale(tm - BaseTime, (TICRATE + ticAdjust), 1000);
}

int I_GetTimeSignaled (bool saveMS)
{
	if (saveMS)
	{
		TicStart = sig_start;
	}
	return tics;
}

int I_WaitForTicPolled (int prevtic)
{
    int time;

	assert (TicFrozen == 0);
    while ((time = I_GetTimePolled(false)) <= prevtic)
		;

    return time;
}

int I_WaitForTicSignaled (int prevtic)
{
	assert (TicFrozen == 0);

	while(tics <= prevtic)
	{
		SEMAPHORE_WAIT(timerWait)
	}

	return tics;
}

void I_FreezeTimeSelect (bool frozen)
{
	I_SelectTimer();
	return I_FreezeTime (frozen);
}

void I_FreezeTimePolled (bool frozen)
{
	if (frozen)
	{
		assert(TicFrozen == 0);
		TicFrozen = I_GetTimePolled(false);
	}
	else
	{
		assert(TicFrozen != 0);
		int froze = TicFrozen;
		TicFrozen = 0;
		int now = I_GetTimePolled(false);
		BaseTime += (now - froze) * 1000 / TICRATE;
	}
}

void I_FreezeTimeSignaled (bool frozen)
{
	TicFrozen = frozen;
}

int I_WaitForTicSelect (int prevtic)
{
	I_SelectTimer();
	return I_WaitForTic (prevtic);
}

//
// I_HandleAlarm
// Should be called every time there is an alarm.
//
void I_HandleAlarm (int sig)
{
	if(!TicFrozen)
		tics++;
	sig_start = SDL_GetTicks();
	SEMAPHORE_SIGNAL(timerWait)
}

//
// I_SelectTimer
// Sets up the timer function based on if we can use signals for efficent CPU
// usage.
//
void I_SelectTimer()
{
	SEMAPHORE_INIT(timerWait, 0, 0)
#ifndef __sun
	signal(SIGALRM, I_HandleAlarm);
#else
	struct sigaction alrmaction;
	sigaction(SIGALRM, NULL, &alrmaction);
	alrmaction.sa_handler = I_HandleAlarm;
	sigaction(SIGALRM, &alrmaction, NULL);
#endif

	struct itimerval itv;
	itv.it_interval.tv_sec = itv.it_value.tv_sec = 0;
	itv.it_interval.tv_usec = itv.it_value.tv_usec = 1000000/(TICRATE + ticAdjust);

	if (setitimer(ITIMER_REAL, &itv, NULL) != 0)
	{
		I_GetTime = I_GetTimePolled;
		I_FreezeTime = I_FreezeTimePolled;
		I_WaitForTic = I_WaitForTicPolled;
	}
	else
	{
		I_GetTime = I_GetTimeSignaled;
		I_FreezeTime = I_FreezeTimeSignaled;
		I_WaitForTic = I_WaitForTicSignaled;
	}
}

// Returns the fractional amount of a tic passed since the most recent tic
fixed_t I_GetTimeFrac (uint32 *ms)
{
	DWORD now = SDL_GetTicks ();
	if (ms) *ms = TicStart + (1000 / (TICRATE + ticAdjust));
	if (TicStart == 0)
	{
		return FRACUNIT;
	}
	else
	{
		fixed_t frac = clamp<fixed_t> ((now - TicStart)*FRACUNIT*(TICRATE + ticAdjust)/1000, 0, FRACUNIT);
		return frac;
	}
}

void I_InitTimer ()
{
	I_GetTime = I_GetTimeSelect;
	I_WaitForTic = I_WaitForTicSelect;
	I_FreezeTime = I_FreezeTimeSelect;
}

void I_ShutdownTimer ()
{

}
