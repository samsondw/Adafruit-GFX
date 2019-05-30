/*!
 * @file Adafruit_SPITFT.h
 *
 * Part of Adafruit's GFX graphics library. Originally this class was
 * written to handle a range of color TFT displays connected via SPI,
 * but over time this library and some display-specific subclasses have
 * mutated to include some color OLEDs as well as parallel-interfaced
 * displays. The name's been kept for the sake of older code.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Limor "ladyada" Fried for Adafruit Industries,
 * with contributions from the open source community.
 *
 * BSD license, all text here must be included in any redistribution.
 */

#ifndef _ADAFRUIT_SPITFT_H_
#define _ADAFRUIT_SPITFT_H_

#include <SPI.h>
#include "Adafruit_GFX.h"

// HARDWARE CONFIG ---------------------------------------------------------

#define DEFAULT_SPI_FREQ 16000000L ///< Hardware SPI default speed

//#define USE_SPI_DMA               ///< If set, use DMA if available

#undef USE_SPI_DMA ///< DMA not implemented

#if defined(USE_SPI_DMA)
#pragma message("GFX DMA IS ENABLED. HIGHLY EXPERIMENTAL.")
#endif

// This is kind of a kludge. Needed a way to disambiguate the software SPI
// and parallel constructors via their argument lists. Originally tried a
// bool as the first argument to the parallel constructor (specifying 8-bit
// vs 16-bit interface) but the compiler regards this as equivalent to an
// integer and thus still ambiguous. SO...the parallel constructor requires
// an enumerated type as the first argument: tft8 (for 8-bit parallel) or
// tft16 (for 16-bit)...even though 16-bit isn't fully implemented or tested
// and might never be, still needed that disambiguation from soft SPI.
/*
// TODO: Implement Parallel
enum tftBusWidth
{
	tft8,
	tft16
}; ///< For first arg to parallel constructor
*/

// CLASS DEFINITION --------------------------------------------------------

/*!
  @brief  Adafruit_SPITFT is an intermediary class between Adafruit_GFX
          and various hardware-specific subclasses for different displays.
          It handles certain operations that are common to a range of
          displays (address window, area fills, etc.). Originally these were
          all color TFT displays interfaced via SPI, but it's since expanded
          to include color OLEDs and parallel-interfaced TFTs. THE NAME HAS
          BEEN KEPT TO AVOID BREAKING A LOT OF SUBCLASSES AND EXAMPLE CODE.
          Many of the class member functions similarly live on with names
          that don't necessarily accurately describe what they're doing,
          again to avoid breaking a lot of other code. If in doubt, read
          the comments.
*/
class Adafruit_SPITFT : public Adafruit_GFX
{

public:
	// CONSTRUCTORS --------------------------------------------------------

	// Software SPI constructor: expects width & height (at default rotation
	// setting 0), 4 signal pins (cs, dc, mosi, sclk), 2 optional pins
	// (reset, miso). cs argument is required but can be NC if unused --
	// rather than moving it to the optional arguments, it was done this way
	// to avoid breaking existing code (NC option was a later addition).
	Adafruit_SPITFT(uint16_t w, uint16_t h, PinName cs, PinName dc, PinName mosi, PinName sck, PinName rst = NC, PinName miso = NC);

	// Hardware SPI constructor using the default SPI port: expects width &
	// height (at default rotation setting 0), 2 signal pins (cs, dc),
	// optional reset pin. cs is required but can be NC if unused -- rather
	// than moving it to the optional arguments, it was done this way to
	// avoid breaking existing code (NC option was a later addition).
	Adafruit_SPITFT(uint16_t w, uint16_t h, PinName cs, PinName dc, PinName rst = NC);

	// Hardware SPI constructor using an arbitrary SPI peripheral: expects
	// width & height (rotation 0), SPIClass pointer, 2 signal pins (cs, dc)
	// and optional reset pin. cs is required but can be NC if unused.
	Adafruit_SPITFT(uint16_t w, uint16_t h, SPI &spi, PinName cs, PinName dc, PinName rst = NC, int bits = 8, int mode = 0, uint32_t freq = DEFAULT_SPI_FREQ);

	// Parallel constructor: expects width & height (rotation 0), flag
	// indicating whether 16-bit (true) or 8-bit (false) interface, 3 signal
	// pins (d0, wr, dc), 3 optional pins (cs, rst, rd). 16-bit parallel
	// isn't even fully implemented but the 'wide' flag was added as a
	// required argument to avoid ambiguity with other constructors.
	/*
    // TODO: Implement Parallel
	Adafruit_SPITFT(uint16_t w, uint16_t h, tftBusWidth busWidth, PinName d0, PinName wr, PinName dc, PinName cs = NC, PinName rst = NC, PinName rd = NC);
	*/

	// CLASS MEMBER FUNCTIONS ----------------------------------------------

	// These first two functions MUST be declared by subclasses:

	/*!
        @brief  Display-specific initialization function.
    */
	virtual void begin(void) = 0;

	/*!
        @brief  Set up the specific display hardware's "address window"
                for subsequent pixel-pushing operations.
        @param  x  Leftmost pixel of area to be drawn (MUST be within
                   display bounds at current rotation setting).
        @param  y  Topmost pixel of area to be drawn (MUST be within
                   display bounds at current rotation setting).
        @param  w  Width of area to be drawn, in pixels (MUST be >0 and,
                   added to x, within display bounds at current rotation).
        @param  h  Height of area to be drawn, in pixels (MUST be >0 and,
                   added to x, within display bounds at current rotation).
    */
	virtual void setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) = 0;

	// Remaining functions do not need to be declared in subclasses
	// unless they wish to provide hardware-specific optimizations.
	// Brief comments here...documented more thoroughly in .cpp file.

	// Subclass' begin() function invokes this to initialize hardware.
	// Name is outdated (interface may be parallel) but for compatibility:
	void initSPI(void);
	// Chip select and/or hardware SPI transaction start as needed:
	void startWrite(void);
	// Chip deselect and/or hardware SPI transaction end as needed:
	void endWrite(void);

	// These functions require a chip-select and/or SPI transaction
	// around them. Higher-level graphics primitives might start a
	// single transaction and then make multiple calls to these functions
	// (e.g. circle or text rendering might make repeated lines or rects)
	// before ending the transaction. It's more efficient than starting a
	// transaction every time.
	void writePixel(int16_t x, int16_t y, uint16_t color);
	void writePixels(uint16_t *colors, uint32_t len, bool block = true, bool bigEndian = false);
	void writeColor(uint16_t color, uint32_t len);
	void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
	void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
	void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);

	// This is a new function, similar to writeFillRect() except that
	// all arguments MUST be onscreen, sorted and clipped. If higher-level
	// primitives can handle their own sorting/clipping, it avoids repeating
	// such operations in the low-level code, making it potentially faster.
	// CALLING THIS WITH UNCLIPPED OR NEGATIVE VALUES COULD BE DISASTROUS.
	inline void writeFillRectPreclipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

	// Another new function, companion to the new non-blocking
	// writePixels() variant.
	void dmaWait(void);

	// These functions are similar to the 'write' functions above, but with
	// a chip-select and/or SPI transaction built-in. They're typically used
	// solo -- that is, as graphics primitives in themselves, not invoked by
	// higher-level primitives (which should use the functions above).
	void drawPixel(int16_t x, int16_t y, uint16_t color);
	void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
	void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
	void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);

	// A single-pixel push encapsulated in a transaction. I don't think
	// this is used anymore (BMP demos might've used it?) but is provided
	// for backward compatibility, consider it deprecated:
	void pushColor(uint16_t color);

	using Adafruit_GFX::drawRGBBitmap; // Check base class first
	void drawRGBBitmap(int16_t x, int16_t y, uint16_t *pcolors, int16_t w, int16_t h);
	void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t fg_color, uint16_t bg_color);

	void invertDisplay(bool i);
	uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

	// Despite parallel additions, function names kept for compatibility:
	void writeCommand(uint8_t cmd); // Write single byte as COMMAND

	void SPI_WRITE8(uint8_t b);   // Write 8-bits (single byte) as DATA
	void SPI_WRITE16(uint16_t w); // Write 16-bits (two bytes) as DATA
	void SPI_WRITE32(uint32_t l); // Write 32-bits (four bytes) as DATA
	uint8_t SPI_READ8(void);	  // Read 8-bits (single byte) of data

	uint16_t SWAP_BYTES(uint16_t x)
	{
		return ((x & 0x00ff) << 8) | (((x & 0xff00) >> 8) & 0x00ff);
	}

	/*!
        @brief  Set the chip-select line HIGH. Does NOT check whether CS pin
                is set (>=0), that should be handled in calling function.
                Despite function name, this is used even if the display
                connection is parallel.
    */
	void SPI_CS_HIGH(void)
	{
		_cs = HIGH;
	}

	/*!
        @brief  Set the chip-select line LOW. Does NOT check whether CS pin
                is set (>=0), that should be handled in calling function.
                Despite function name, this is used even if the display
                connection is parallel.
    */
	void SPI_CS_LOW(void)
	{
		_cs = LOW;
	}

	/*!
        @brief  Set the data/command line HIGH (data mode).
    */
	void SPI_DC_HIGH(void)
	{
		_dc = HIGH;
	}

	/*!
        @brief  Set the data/command line LOW (command mode).
    */
	void SPI_DC_LOW(void)
	{
		_dc = LOW;
	}

protected:
	// A few more low-level member functions -- some may have previously
	// been macros. Shouldn't have a need to access these externally, so
	// they've been moved to the protected section. Additionally, they're
	// declared inline here and the code is in the .cpp file, since outside
	// code doesn't need to see these.
	inline void SPI_MOSI_HIGH(void);
	inline void SPI_MOSI_LOW(void);
	inline void SPI_SCK_HIGH(void);
	inline void SPI_SCK_LOW(void);
	inline bool SPI_MISO_READ(void);
	inline void SPI_BEGIN_TRANSACTION(void);
	inline void SPI_END_TRANSACTION(void);
	inline void TFT_WR_STROBE(void); // Parallel interface write strobe
	inline void TFT_RD_HIGH(void);   // Parallel interface read high
	inline void TFT_RD_LOW(void);	// Parallel interface read low

	// CLASS INSTANCE VARIABLES --------------------------------------------

	// Here be dragons! There's a big union of three structures here --
	// one each for hardware SPI, software (bitbang) SPI, and parallel
	// interfaces. This is to save some memory, since a display's connection
	// will be only one of these. The order of some things is a little weird
	// in an attempt to get values to align and pack better in RAM.

#if defined(__cplusplus) && (__cplusplus >= 201100)
	union {
#endif
		struct
		{					//   Values specific to HARDWARE SPI:
			SPI *_spi;		///< SPI class pointer
			int _bits = 8;  ///< SPI Bits per frame (4 - 16).  Default: 8
			int _mode = 0;  ///< SPI Mode: Clock polarity and phase (0 - 3).  Default: 0
			uint32_t _freq; ///< SPI bitrate (if no SPI transactions)
		} hwspi;			///< Hardware SPI values
		struct
		{				   //   Values specific to SOFTWARE SPI:
			PinName _mosi; ///< MOSI pin #
			PinName _miso; ///< MISO pin #
			PinName _sck;  ///< SCK pin #
		} swspi;		   ///< Software SPI values
		struct
		{				   //   Values specific to 8-bit parallel:
			PinName _d0;   ///< Data pin 0 #
			PinName _wr;   ///< Write strobe pin #
			PinName _rd;   ///< Read strobe pin # (or NC)
			bool wide = 0; ///< If true, is 16-bit interface
		} tft8;			   ///< Parallel interface settings
#if defined(__cplusplus) && (__cplusplus >= 201100)
	}; ///< Only one interface is active
#endif
	uint8_t connection; ///< TFT_HARD_SPI, TFT_SOFT_SPI, etc.
	DigitalInOut _rst;  ///< Reset pin # (or NC)
	DigitalInOut _cs;   ///< Chip select pin # (or NC)
	DigitalInOut _dc;   ///< Data/command pin #

	int16_t _w, _h;
	int16_t _xstart = 0;		  ///< Internal framebuffer X offset
	int16_t _ystart = 0;		  ///< Internal framebuffer Y offset
	uint8_t invertOnCommand = 0;  ///< Command to enable invert mode
	uint8_t invertOffCommand = 0; ///< Command to disable invert mode

	uint32_t _freq = 0; ///< Dummy var to keep subclasses happy
};

#endif // end _ADAFRUIT_SPITFT_H_
