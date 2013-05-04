/*
 * hidInfo.c
 *
 * Created: 4/27/2013 11:12:25 AM
 *  Author: dan
 */ 

#include "hidInfo.h"
#include "constants.h"

static uint8_t PrevHIDReportBuffer[GENERIC_REPORT_SIZE];

USB_ClassInfo_HID_Device_t Generic_HID_Interface =
{
	.Config =
	{
		.InterfaceNumber              = 0,
		.ReportINEndpoint             =
		{
			.Address              = GENERIC_IN_EPADDR,
			.Size                 = GENERIC_EPSIZE,
			.Banks                = 1,
		},
		.PrevReportINBuffer           = PrevHIDReportBuffer,
		.PrevReportINBufferSize       = sizeof(PrevHIDReportBuffer),
	},
};

