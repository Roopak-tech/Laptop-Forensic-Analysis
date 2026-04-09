#include <WINDOWS.H>
#include "WSTRemote.H"
#include "WSTLog.h"
#include <stdio.h>

// General Abort / Failure Errors
#define ERR_MAIL_CAPTURE_ABORTED_AT_START    -100
#define ERR_MAIL_CAPTURE_THREAD_TERMINATED   -101
#define GEN_ERROR -401
// Function Failure Errors
#define ERR_MAIL_CAPTURE_THREAD_CREATE_ERROR -200
#define CPY_INSUFF_SPACE_ERR -301

// Success "errors"
#define MAIL_SUCCESSFUL						  100


struct Args
{
	WSTLog &AL;
	gui_resources &GUI;
};