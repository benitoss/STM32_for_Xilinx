//
// Multicore 2
//
// Copyright (c) 2017-2021 - Victor Trucco
//
// Additional code, debug and fixes: Diogo Patrão e Roberto Focosi
//
// All rights reserved
//
// Redistribution and use in source and synthezised forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// Redistributions in synthesized form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// Neither the name of the author nor the names of other contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS CODE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// You are responsible for any legal issues arising from your use of this code.
//

// === FIRMWARE_TYPE === (uncomment only ONE of these lines)
//---------------------------------
// #define MC2
// #define MC2P
// #define UNAMIGA2
// #define NEPTUNO
//---------------------------------

/*
*  ArduinoLog log level
*  --------------------
* LOG_LEVEL_SILENT     no output
* LOG_LEVEL_FATAL      fatal errors
* LOG_LEVEL_ERROR      all errors
* LOG_LEVEL_WARNING    errors, and warnings
* LOG_LEVEL_NOTICE     errors, warnings and notices
* LOG_LEVEL_TRACE      errors, warnings, notices & traces
* LOG_LEVEL_VERBOSE    all
 */
#define LOG_LEVEL LOG_LEVEL_VERBOSE
#define DISABLE_LOGGING // Uncomment this line, after debugging, to reduce binary size

//#define DISABLE_SPLASH_DELAY // Disable splash screen delay. To speed up development
//#define TEST_NAVIGATION // Enable pressing F12 to show current Core's menu without reloading
//#define DISABLE_NEW_OPTION_NAV // Disable new option navigation. Where you can use right and left arrows to select options

// spi version
const unsigned char version[4] = "201";

#define SDFAT_FILE_TYPE 3

#if SDFAT_FILE_TYPE == 1
#define SDFAT_FILE_TYPE_STR "FAT16/FAT32"
#elif SDFAT_FILE_TYPE == 2
#define SDFAT_FILE_TYPE_STR "exFAT"
#elif SDFAT_FILE_TYPE == 3
#define SDFAT_FILE_TYPE_STR "FAT16/FAT32/exFAT"
#endif

#ifndef PINS_H
#define PINS_H

#ifdef MC2P
#define SPLASH "     M U L T I C O R E  2 +"
#define EXTENSION "MCP"
#define LAUNCHER_CORE_EXT "CRP"
#endif

#ifdef MC2
#define SPLASH "      M U L T I C O R E  2"
#define EXTENSION "MC2"
#define LAUNCHER_CORE_EXT "CR2"
#endif

#ifdef UNAMIGA2
#define SPLASH "        U N A M I G A  2"
#define EXTENSION "UA2"
#define LAUNCHER_CORE_EXT "CA2"
#endif

#ifdef NEPTUNO
#define SPLASH "         N E P T U N O  "
#define EXTENSION "NP1"
#define LAUNCHER_CORE_EXT "CNU"
#endif

#define LAUNCHER_EXTENSION "ARC"

#define CORE_EXTENSIONS EXTENSION"/"LAUNCHER_EXTENSION

//blue pill
#define PIN_TCK     PB0
#define PIN_TDI     PB1
#define PIN_TMS     PB10
#define PIN_TDO     PB11

//black pill
// #define PIN_TCK     PB0
// #define PIN_TDI     PB1
// #define PIN_TMS     PB2
// #define PIN_TDO     PB10

#define PIN_CSSD    PA4
#define PIN_LED     PC13
#define PIN_nWAIT   PA15
#define PIN_NSS     PB12

#define SPI_FPGA 2

//pseudo functions
#define SPI_DESELECTED()   GPIOB->regs->BSRR = (1U << 12) //pin PB12 (SPI_SS)
#define SPI_SELECTED()     GPIOB->regs->BRR  = (1U << 12) //pin PB12 (SPI_SS)

#define JTAG_clock() GPIOB->regs->ODR |=1; GPIOB->regs->ODR &= ~(1);
#define TDI_HIGH()   GPIOB->regs->ODR |= 2;
#define TDI_LOW()    GPIOB->regs->ODR &= ~(2);

#define CHECK_WAIT()    GPIOA->regs->IDR & 0x8000

#endif
