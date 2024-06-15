/*___________________________________________________________________________________________________

Title:
	sd.h v2.0

Description:
	SD card interface using SPI
	
	For complete details visit:
	https://www.programming-electronics-diy.xyz/2022/07/sd-memory-card-library-for-avr.html

Author:
 	Liviu Istrate
	istrateliviu24@yahoo.com
	www.programming-electronics-diy.xyz

Donate:
	Software development takes time and effort so if you find this useful consider a small donation at:
	paypal.me/alientransducer
_____________________________________________________________________________________________________*/


/* ----------------------------- LICENSE - GNU GPL v3 -----------------------------------------------

* This license must be included in any redistribution.

* Copyright (C) 2022 Liviu Istrate, www.programming-electronics-diy.xyz (istrateliviu24@yahoo.com)

* Project URL: https://www.programming-electronics-diy.xyz/2022/07/sd-memory-card-library-for-avr.html
* Inspired by: http://www.rjhcoding.com/avrc-sd-interface-1.php

* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program. If not, see <https://www.gnu.org/licenses/>.

--------------------------------- END OF LICENSE --------------------------------------------------*/

/*___________________________________ CHANGELOG _____________________________________________________

v2.0
	04-02-2024:
	- Included support for modern UPDI AVR microcontrollers.

v1.2
	- Added check conditions to include the proper SPI registers depending on the microcontroller,
	since ATmega328PB has two SPI devices and ATmega328P has one, the registers must have a zero
	at the end indicating the SPI device number for PB and no zero for P.

v1.1
	- Fixed an issue with the SPI SS pin that if the user didn't make use of it by setting 
	  it as an output, the SPI won't work being stuck in the while loop. According to the AVR datasheet
	  the SS pin must be set as output regardless if used or not for chip select (CS).
_____________________________________________________________________________________________________*/

#ifndef SD_H_
#define SD_H_
#ifdef __cplusplus
extern "C" {
#endif
// F_CPU is best to be defined inside project properties in Microchip Studio
// or a Makefile if custom Makefiles are used.
// See https://www.programming-electronics-diy.xyz/2024/01/defining-fcpu.html
//#define F_CPU							16000000UL

/*************************************************************
	INCLUDES
**************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "sdmmc_cmd.h"


/*************************************************************
	USER SETTINGS
**************************************************************/
/*
// SPI0 I/O pins (MOSI and SCK)
#define SPI0_PORT								PORTB // SPI PORT
#define SPI_DDR									DDRB  // SPI DDR
#define SPI0_PINS								PINB  // Holds the state of each pin on port where SPI pins are

#define SPI_MOSI_PIN							PB3   // SDA pin
#define SPI_SCK_PIN								PB5   // SCK pin
// According to the Atmel datasheet the SPI SS pin
// must be set as output for the SPI to work in master mode.
// Can be the same pin as CARD_CS.
#define SPI_SS_PIN								PB2   // SS pin

// SPI Chip Select pin
#define CARD_CS_DDR								DDRB
#define CARD_CS_PORT							PORTB
#define CARD_CS_PIN								PB0

// Card detect pin for card sockets that have this feature (optional)
#define CARD_DETECT_DDR							DDRB
#define CARD_DETECT_PORT						PORTB
#define CARD_DETECT_PIN							PB1
#define CARD_DETECT_PINS						PINB


#define SPI0_CLK_DIV							2	// smallest SPI prescaler (gives highest speed)
*/
/*************************************************************
	SYSTEM DEFINES
**************************************************************/
/*
// SPI registers
#ifdef SPCR0
	#define _SPCR			SPCR0
	#define	_SPSR			SPSR0
	#define _SPDR			SPDR0
#else
	#define _SPCR			SPCR
	#define	_SPSR			SPSR
	#define _SPDR			SPDR
#endif
*/
// Block length can be set in Standard Capacity SD cards using CMD16 (SET_BLOCKLEN).
// For SDHC and SDXC cards the block length is always set to 512 bytes.
#define SD_BUFFER_SIZE							512 // CAUTION: only 512 bytes/block is implemented

// R1 Response
#define PARAM_ERROR(X)							X & 0b01000000
#define ADDR_ERROR(X)							X & 0b00100000
#define ERASE_SEQ_ERROR(X)						X & 0b00010000
#define CRC_ERROR(X)							X & 0b00001000
#define ILLEGAL_CMD(X)							X & 0b00000100
#define ERASE_RESET(X)							X & 0b00000010
#define IN_IDLE(X)								X & 0b00000001

// R3 Response
#define POWER_UP_STATUS(X)						X & 0x80
#define CCS_VAL(X)								X & 0x40	// Card Capacity Status
#define VDD_2728(X)								X & 0b10000000
#define VDD_2829(X)								X & 0b00000001
#define VDD_2930(X)								X & 0b00000010
#define VDD_3031(X)								X & 0b00000100
#define VDD_3132(X)								X & 0b00001000
#define VDD_3233(X)								X & 0b00010000
#define VDD_3334(X)								X & 0b00100000
#define VDD_3435(X)								X & 0b01000000
#define VDD_3536(X)								X & 0b10000000

// R7 Response
#define CMD_VER(X)								((X >> 4) & 0xF0) // command version
#define VOL_ACC(X)								(X & 0x1F)	// voltage accepted
#define VOLTAGE_ACC_27_33						0b00000001	// 2.7-3.6V
#define VOLTAGE_ACC_LOW							0b00000010	// low voltage
#define VOLTAGE_ACC_RES1						0b00000100	// reserved
#define VOLTAGE_ACC_RES2						0b00001000	// reserved

// Data Error Token
#define SD_TOKEN_OOR(X)							X & 0b00001000	// Data Out Of Range
#define SD_TOKEN_CECC(X)						X & 0b00000100	// Card ECC Failed
#define SD_TOKEN_CC(X)							X & 0b00000010	// CC Error
#define SD_TOKEN_ERROR(X)						X & 0b00000001

/* Command List */
// CMD0 - GO_IDLE_STATE
#define CMD0					0
#define CMD0_ARG				0x00000000
#define CMD0_CRC				0x94

// CMD8 - SEND_IF_COND (Send Interface Condition)
#define CMD8					8
#define CMD8_ARG				0x0000001AA
#define CMD8_CRC				0x86 // (1000011 << 1)

// CMD55 - APP_CMD
#define CMD55					55
#define CMD55_ARG				0x00000000
#define CMD55_CRC				0x00

// ACMD41 - SD_SEND_OP_COND (Send Operating Condition)
// ACMD stands for Application Specific Command and before issued, the CMD55 must be sent first
#define ACMD41					41
#define ACMD41_ARG				0x40000000
#define ACMD41_CRC				0x00

// CMD58 - read OCR (Operation Conditions Register)
#define CMD58					58
#define CMD58_ARG				0x00000000
#define CMD58_CRC				0x00

// CMD17 - READ_SINGLE_BLOCK
// For a 16MHz oscillator and SPI clock set to divide by 2. 
// Thus, to get the number of bytes we need to send over SPI to reach 100ms, we do 
// (0.1s * 16000000 MHz) / (2 * 8 bytes) = 100000
#define CMD17                   17
#define CMD17_CRC               0x00
//#define SD_MAX_READ_ATTEMPTS    (0.1 * F_CPU) / (SPI0_CLK_DIV * 8)

// CMD24 - WRITE_BLOCK
// For a 16MHz oscillator and SPI clock set to divide by 2.
// Thus, to get the number of bytes we need to send over SPI to reach 250ms, we do
// (0.25s * 16000000 MHz) / (2 * 8 bytes) = 250000
#define CMD24                   24
#define CMD24_CRC               0x00
//#define SD_MAX_WRITE_ATTEMPTS   (0.25 * F_CPU) / (SPI0_CLK_DIV * 8)

// Card Type
#define SD_V1_SDSC				1
#define SD_V2_SDSC				2
#define SD_V2_SDHC_SDXC			3


/*************************************************************
	MACRO FUNCTIONS
**************************************************************/
/*
#define SPI_CARD_CS_DISABLE()			CARD_CS_PORT |= (1 << CARD_CS_PIN)
#define SPI_CARD_CS_ENABLE()			CARD_CS_PORT &= ~(1 << CARD_CS_PIN)
*/

/*************************************************************
	ENUMS
**************************************************************/
/* Card initialization return codes (SD_RETURN_CODES) */
typedef enum {
	SD_OK,
	SD_IDLE_STATE_TIMEOUT,
	SD_GENERAL_ERROR,
	SD_CHECK_PATTERN_MISMATCH,
	SD_NONCOMPATIBLE_VOLTAGE_RANGE,
	SD_POWER_UP_BIT_NOT_SET,
	SD_NOT_SD_CARD,
	//SD_OP_COND_TIMEOUT,
	//SD_SET_BLOCKLEN_TIMEOUT,
	//SD_WRITE_BLOCK_TIMEOUT,
	//SD_WRITE_BLOCK_FAIL,
	//SD_READ_BLOCK_TIMEOUT,
	//SD_READ_BLOCK_DATA_TOKEN_MISSING,
	//SD_DATA_TOKEN_TIMEOUT,
	//SD_SELECT_CARD_TIMEOUT,
	//SD_SET_RELATIVE_ADDR_TIMEOUT
} SD_RETURN_CODES;


/*************************************************************
	GLOBALS
**************************************************************/
extern uint8_t SD_Buffer[SD_BUFFER_SIZE + 1]; // reserve 1 byte for the null
extern uint8_t SD_ResponseToken;


/*************************************************************
	FUNCTION PROTOTYPES
**************************************************************/
SD_RETURN_CODES sd_init(void);
uint8_t sd_write_single_block(uint32_t addr, uint8_t *buf);
uint8_t sd_read_single_block(uint32_t addr, uint8_t *buf);
bool sd_detected(void);

void SPI_Send(uint8_t *buf, uint16_t length);
void SPI_Receive(uint8_t *buf, uint16_t length);


void set_sdmmc_card_instance(sdmmc_card_t* _card);

#ifdef __cplusplus
}
#endif
#endif /* SD_H_ */