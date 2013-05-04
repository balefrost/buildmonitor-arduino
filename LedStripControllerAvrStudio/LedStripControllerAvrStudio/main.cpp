#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#include <limits.h>
#include <math.h>
#include "LUFA/Drivers/USB/USB.h"
#include "RgbColor.h"
#include "hidInfo.h"

typedef RgbColor<uint8_t> rgb8;

unsigned char buffer[32*3];
unsigned char bufferIndex = UCHAR_MAX;
rgb8 colors[32];
volatile bool refreshFrame = false;

volatile unsigned long millis = 0;
volatile unsigned short extraMicros = 0;

ISR(SPI_STC_vect)
{
	++bufferIndex;
	if (bufferIndex < 32 * 3) {
		SPDR = buffer[bufferIndex];
	} else {
		refreshFrame = true;
	}
}

ISR(TIMER3_COMPA_vect)
{
	millis += 8;
	extraMicros += 332;
	if (extraMicros > 1000) {
		++millis;
		extraMicros -= 1000;
	}
	if (refreshFrame == false) {
		PIND = _BV(PIND6);
		//PORTC = _BV(PORTC6);
		bufferIndex = 0;
		SPDR = buffer[0];
	}	
}

void EVENT_USB_Device_ConfigurationChanged(void)
{
	HID_Device_ConfigureEndpoints(&Generic_HID_Interface);

	USB_Device_EnableSOFEvents();
}

void EVENT_USB_Device_ControlRequest(void)
{
	HID_Device_ProcessControlRequest(&Generic_HID_Interface);
}

void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Generic_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean true to force the sending of the report, false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                         uint8_t* const ReportID,
                                         const uint8_t ReportType,
                                         void* ReportData,
                                         uint16_t* const ReportSize)
{
	return false;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
	if (ReportType == HID_REPORT_ITEM_Out && ReportSize == 3) {
		const uint8_t *typedReportData = (uint8_t*)ReportData;
		rgb8 color(typedReportData[0], typedReportData[1], typedReportData[2]);
		for (int i = 0; i < 32; ++i) {
			colors[i] = color;
		}
	}
}

int main(void)
{
	USB_Init();
	
	colors[0] = rgb8(0x30, 0x30, 0x30);
	colors[1] = rgb8(0x30, 0x30, 0x30);
	colors[2] = rgb8(0x30, 0x30, 0x30);
	
	DDRC = _BV(DDC6);
	PORTC = 0;

	DDRD = _BV(DDD6);
	
	//set SS, SCK, and MOSI to outputs
	DDRB |= _BV(DDB0) | _BV(DDB1) | _BV(DDB2);
	
	//SS low
	PORTB &= ~_BV(PORTB0);
	
	//clock normally low, clocked on rising edges, at osc/2
	SPSR = 0; //_BV(SPI2X);
	SPCR = _BV(SPIE) | _BV(MSTR) | _BV(SPE) | _BV(SPR0);

	//16M / (2083 * 64) = 120.02hz
	OCR3A = 2082;
	
	//interrupt on overflow
	TIMSK3 = _BV(OCIE3A);

	//do nothing on counter match
	//CTC, TOP is OCR3A
	//timer prescale is 64
	TCCR3A = 0;
	TCCR3C = 0;
	TCCR3B = _BV(WGM32) | _BV(CS31) | _BV(CS30);
	sei();

	while(true) {
		if (refreshFrame) {
			PORTC |= _BV(PORTC6);
			
			//copy out of volatile
			unsigned long now = millis;
			
			float scale = 0.1f + 0.9f * (sin(2.0 * M_PI * now / 3000.0f) + 1.0) / 2.0;
			for (unsigned char i = 0, idx = 0; i < 32; ++i) {
				buffer[idx++] = colors[i].b * scale;
				buffer[idx++] = colors[i].g * scale;
				buffer[idx++] = colors[i].r * scale;
			}
			
			PORTC &= ~_BV(PORTC6);
			refreshFrame = false;
		}
		HID_Device_USBTask(&Generic_HID_Interface);
		USB_USBTask();
		//sleep_mode();
	}
}