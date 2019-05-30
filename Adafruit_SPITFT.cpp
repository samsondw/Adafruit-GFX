/*!
 * @file Adafruit_SPITFT.cpp
 *
 * @mainpage Adafruit SPI TFT Displays (and some others)
 *
 * @section intro_sec Introduction
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

 * @section dependencies Dependencies
 *
 * This library depends on <a href="https://github.com/adafruit/Adafruit_GFX">
 * Adafruit_GFX</a> being present on your system. Please make sure you have
 * installed the latest version before using this library.
 *
 * @section author Author
 *
 * Written by Limor "ladyada" Fried for Adafruit Industries,
 * with contributions from the open source community.
 *
 * @section license License
 *
 * BSD license, all text here must be included in any redistribution.
 */

#include "Adafruit_SPITFT.h"

#if defined(USE_SPI_DMA)
// TODO: Implement DMA
#endif // end USE_SPI_DMA

// Possible values for Adafruit_SPITFT.connection:
#define TFT_HARD_SPI 0 ///< Display interface = hardware SPI
#define TFT_SOFT_SPI 1 ///< Display interface = software SPI
// #define TFT_PARALLEL 2 ///< Display interface = 8- or 16-bit parallel

uint8_t *spi_buffer;
const int SPI_BUFFER_SIZE = 1024;

// CONSTRUCTORS ------------------------------------------------------------

/*!
    @brief   Adafruit_SPITFT constructor for software (bitbang) SPI.
    @param   w     Display width in pixels at default rotation setting (0).
    @param   h     Display height in pixels at default rotation setting (0).
    @param   cs    Arduino pin # for chip-select (-1 if unused, tie CS low).
    @param   dc    Arduino pin # for data/command select (required).
    @param   mosi  Arduino pin # for bitbang SPI MOSI signal (required).
    @param   sck   Arduino pin # for bitbang SPI SCK signal (required).
    @param   rst   Arduino pin # for display reset (optional, display reset
                   can be tied to MCU reset, default of -1 means unused).
    @param   miso  Arduino pin # for bitbang SPI MISO signal (optional,
                   -1 default, many displays don't support SPI read).
    @return  Adafruit_SPITFT object.
    @note    Output pins are not initialized; application typically will
             need to call subclass' begin() function, which in turn calls
             this library's initSPI() function to initialize pins.
*/
Adafruit_SPITFT::Adafruit_SPITFT(uint16_t w, uint16_t h, PinName cs, PinName dc, PinName mosi, PinName sck, PinName rst, PinName miso)
    : Adafruit_GFX(w, h), connection(TFT_SOFT_SPI), _w(w), _h(h), _rst(rst), _cs(cs), _dc(dc)
{
    swspi._sck = sck;
    swspi._mosi = mosi;
    swspi._miso = miso;
}

/*!
    @brief   Adafruit_SPITFT constructor for hardware SPI using the board's
             default SPI peripheral.
    @param   w     Display width in pixels at default rotation setting (0).
    @param   h     Display height in pixels at default rotation setting (0).
    @param   cs    Arduino pin # for chip-select (-1 if unused, tie CS low).
    @param   dc    Arduino pin # for data/command select (required).
    @param   rst   Arduino pin # for display reset (optional, display reset
                   can be tied to MCU reset, default of -1 means unused).
    @return  Adafruit_SPITFT object.
    @note    Output pins are not initialized; application typically will
             need to call subclass' begin() function, which in turn calls
             this library's initSPI() function to initialize pins.
*/
Adafruit_SPITFT::Adafruit_SPITFT(uint16_t w, uint16_t h, PinName cs, PinName dc, PinName rst)
    : Adafruit_GFX(w, h), connection(TFT_HARD_SPI), _w(w), _h(h), _rst(rst), _cs(cs), _dc(dc)
{
    hwspi._spi = new SPI(SPI_MOSI, SPI_MISO, SPI_SCK, SPI_CS);
}

/*!
    @brief   Adafruit_SPITFT constructor for hardware SPI using a specific
             SPI peripheral.
    @param   w      Display width in pixels at default rotation (0).
    @param   h      Display height in pixels at default rotation (0).
    @param   spi    Pointer to SPI type.
    @param   cs     Arduino pin # for chip-select (-1 if unused, tie CS low).
    @param   dc     Arduino pin # for data/command select (required).
    @param   rst    Arduino pin # for display reset (optional, display reset
                    can be tied to MCU reset, default of -1 means unused).
    @param  bits    SPI Bits (4 - 16, default: 8)
    @param  mode    SPI mode (default: 0)
    @param  freq    SPI frequency (optional, pass 0 if unused)
    @return  Adafruit_SPITFT object.
    @note    Output pins are not initialized; application typically will
             need to call subclass' begin() function, which in turn calls
             this library's initSPI() function to initialize pins.
*/
Adafruit_SPITFT::Adafruit_SPITFT(uint16_t w, uint16_t h, SPI &spi, PinName cs, PinName dc, PinName rst, int bits, int mode, uint32_t freq)
    : Adafruit_GFX(w, h), connection(TFT_HARD_SPI), _w(w), _h(h), _rst(rst), _cs(cs), _dc(dc), _freq(freq)
{
    hwspi._spi = &spi;
    hwspi._bits = bits;
    hwspi._mode = mode;
    hwspi._freq = freq;
}

/*!
    @brief   Adafruit_SPITFT constructor for parallel display connection.
    @param   w         Display width in pixels at default rotation (0).
    @param   h         Display height in pixels at default rotation (0).
    @param   busWidth  If tft16 (enumeration in header file), is a 16-bit
                       parallel connection, else 8-bit.
                       16-bit isn't fully implemented or tested yet so
                       applications should pass "tft8" for now...needed to
                       stick a required enum argument in there to
                       disambiguate this constructor from the soft-SPI case.
                       Argument is ignored on 8-bit architectures (no 'wide'
                       support there since PORTs are 8 bits anyway).
    @param   d0        Arduino pin # for data bit 0 (1+ are extrapolated).
                       The 8 (or 16) data bits MUST be contiguous and byte-
                       aligned (or word-aligned for wide interface) within
                       the same PORT register (might not correspond to
                       Arduino pin sequence).
    @param   wr        Arduino pin # for write strobe (required).
    @param   dc        Arduino pin # for data/command select (required).
    @param   cs        Arduino pin # for chip-select (optional, -1 if unused,
                       tie CS low).
    @param   rst       Arduino pin # for display reset (optional, display reset
                       can be tied to MCU reset, default of -1 means unused).
    @param   rd        Arduino pin # for read strobe (optional, -1 if unused).
    @return  Adafruit_SPITFT object.
    @note    Output pins are not initialized; application typically will need
             to call subclass' begin() function, which in turn calls this
             library's initSPI() function to initialize pins.
             Yes, the name is a misnomer...this library originally handled
             only SPI displays, parallel being a recent addition (but not
             wanting to break existing code).
*/
/*
// TODO: Implement Parallel
Adafruit_SPITFT::Adafruit_SPITFT(uint16_t w, uint16_t h, tftBusWidth busWidth, PinName d0, PinName wr, PinName dc, PinName cs, PinName rst, PinName rd)
    : Adafruit_GFX(w, h), connection(TFT_PARALLEL), _w(w), _h(h), _rst(rst), _cs(cs), _dc(dc)
{
    tft8._d0 = d0;
    tft8._wr = wr;
    tft8._rd = rd;
    tft8.wide = (busWidth == tft16);
}
*/

// end constructors -------

// CLASS MEMBER FUNCTIONS --------------------------------------------------

// begin() and setAddrWindow() MUST be declared by any subclass.

/*!
    @brief  Configure microcontroller pins for TFT interfacing. Typically
            called by a subclass' begin() function.
    @note   Another anachronistically-named function; this is called even
            when the display connection is parallel (not SPI). Also, this
            could probably be made private...quite a few class functions
            were generously put in the public section.
*/
void Adafruit_SPITFT::initSPI(void)
{
    // Init basic control pins common to all connection types
    if (_cs >= 0)
    {
        pinMode(_cs, OUTPUT);
        digitalWrite(_cs, HIGH); // Deselect
    }
    pinMode(_dc, OUTPUT);
    digitalWrite(_dc, HIGH); // Data mode

    if (connection == TFT_HARD_SPI)
    {
        /*
        */
        // Setup spi buffer
        if ((spi_buffer = (uint8_t *)malloc(SPI_BUFFER_SIZE)))
            memset(spi_buffer, 0, SPI_BUFFER_SIZE);
        // Setup Hardware SPI
        hwspi._spi->format(hwspi._bits, hwspi._mode);
        hwspi._spi->frequency(hwspi._freq);
        //hwspi._spi->begin();
    }
    else if (connection == TFT_SOFT_SPI)
    {
        pinMode(swspi._mosi, OUTPUT);
        digitalWrite(swspi._mosi, LOW);
        pinMode(swspi._sck, OUTPUT);
        digitalWrite(swspi._sck, LOW);
        if (swspi._miso >= 0)
        {
            pinMode(swspi._miso, INPUT);
        }
    }
    /*
    // TODO: Implement Parallel
    else
    {   // TFT_PARALLEL
        // Initialize data pins.  We were only passed d0, so scan
        // the pin description list looking for the other pins.
        // They'll be on the same PORT, and within the next 7 (or 15) bits
        // (because we need to write to a contiguous PORT byte or word).

        pinMode(tft8._wr, OUTPUT);
        digitalWrite(tft8._wr, HIGH);
        if (tft8._rd >= 0)
        {
            pinMode(tft8._rd, OUTPUT);
            digitalWrite(tft8._rd, HIGH);
        }
    }
    */

    if (_rst >= 0)
    {
        // Toggle _rst low to reset
        pinMode(_rst, OUTPUT);
        digitalWrite(_rst, HIGH);
        wait_ms(100);
        digitalWrite(_rst, LOW);
        wait_ms(100);
        digitalWrite(_rst, HIGH);
        wait_ms(200);
    }

#if defined(USE_SPI_DMA)
    // TODO: Implement DMA
#endif // end USE_SPI_DMA
}

/*!
    @brief  Call before issuing command(s) or data to display. Performs
            chip-select (if required) and starts an SPI transaction (if
            using hardware SPI and transactions are supported). Required
            for all display types; not an SPI-specific function.
*/
void Adafruit_SPITFT::startWrite(void)
{
    SPI_BEGIN_TRANSACTION();
    if (_cs >= 0)
        SPI_CS_LOW();
}

/*!
    @brief  Call after issuing command(s) or data to display. Performs
            chip-deselect (if required) and ends an SPI transaction (if
            using hardware SPI and transactions are supported). Required
            for all display types; not an SPI-specific function.
*/
void Adafruit_SPITFT::endWrite(void)
{
    if (_cs >= 0)
        SPI_CS_HIGH();
    SPI_END_TRANSACTION();
}

// -------------------------------------------------------------------------
// Lower-level graphics operations. These functions require a chip-select
// and/or SPI transaction around them (via startWrite(), endWrite() above).
// Higher-level graphics primitives might start a single transaction and
// then make multiple calls to these functions (e.g. circle or text
// rendering might make repeated lines or rects) before ending the
// transaction. It's more efficient than starting a transaction every time.

/*!
    @brief  Draw a single pixel to the display at requested coordinates.
            Not self-contained; should follow a startWrite() call.
    @param  x      Horizontal position (0 = left).
    @param  y      Vertical position   (0 = top).
    @param  color  16-bit pixel color in '565' RGB format.
*/
void Adafruit_SPITFT::writePixel(int16_t x, int16_t y, uint16_t color)
{
    if ((x >= 0) && (x < _width) && (y >= 0) && (y < _height))
    {
        setAddrWindow(x, y, 1, 1);
        SPI_WRITE16(color);
    }
}

/*!
    @brief  Issue a series of pixels from memory to the display. Not self-
            contained; should follow startWrite() and setAddrWindow() calls.
    @param  colors     Pointer to array of 16-bit pixel values in '565' RGB
                       format.
    @param  len        Number of elements in 'colors' array.
    @param  block      If true (default case if unspecified), function blocks
                       until DMA transfer is complete. This is simply IGNORED
                       if DMA is not enabled. If false, the function returns
                       immediately after the last DMA transfer is started,
                       and one should use the dmaWait() function before
                       doing ANY other display-related activities (or even
                       any SPI-related activities, if using an SPI display
                       that shares the bus with other devices).
    @param  bigEndian  If using DMA, and if set true, bitmap in memory is in
                       big-endian order (most significant byte first). By
                       default this is false, as most microcontrollers seem
                       to be little-endian and 16-bit pixel values must be
                       byte-swapped before issuing to the display (which tend
                       to be big-endian when using SPI or 8-bit parallel).
                       If an application can optimize around this -- for
                       example, a bitmap in a uint16_t array having the byte
                       values already reordered big-endian, this can save
                       some processing time here, ESPECIALLY if using this
                       function's non-blocking DMA mode. Not all cases are
                       covered...this is really here only for SAMD DMA and
                       much forethought on the application side.
*/
void Adafruit_SPITFT::writePixels(uint16_t *colors, uint32_t len, bool block, bool bigEndian)
{
    if (!len)
        return; // Avoid 0-byte transfers

#if defined(USE_SPI_DMA)
        // TODO: Implement DMA
#endif // end USE_SPI_DMA

    if (connection == TFT_HARD_SPI)
    {
        uint32_t i = 0;
        for (int remaining_bytes = 2 * len; remaining_bytes > 0; remaining_bytes -= SPI_BUFFER_SIZE)
        {
            // Fill array of colors
            for (uint8_t *ptr = (uint8_t *)spi_buffer; (ptr < (uint8_t *)(spi_buffer + SPI_BUFFER_SIZE)) && (i < len); ptr++)
            {
                *ptr++ = highByte(colors[i]);
                *ptr = lowByte(colors[i++]);
            }
            // Write array of bytes to SPI
            hwspi._spi->write((char *)spi_buffer, std::min(remaining_bytes, SPI_BUFFER_SIZE), (char *)NULL, 0);
        }
    }
    else
    {
        // All other cases (bitbang SPI or non-DMA hard SPI or parallel),
        // use a loop with the normal 16-bit data write function:
        while (len--)
            SPI_WRITE16(*colors++);
    }
}

/*!
    @brief  Wait for the last DMA transfer in a prior non-blocking
            writePixels() call to complete. This does nothing if DMA
            is not enabled, and is not needed if blocking writePixels()
            was used (as is the default case).
*/
void Adafruit_SPITFT::dmaWait(void)
{
#if defined(USE_SPI_DMA)
    // TODO: Implement DMA
#endif // end USE_SPI_DMA
}

/*!
    @brief  Issue a series of pixels, all the same color. Not self-
            contained; should follow startWrite() and setAddrWindow() calls.
    @param  color  16-bit pixel color in '565' RGB format.
    @param  len    Number of pixels to draw.
*/
void Adafruit_SPITFT::writeColor(uint16_t color, uint32_t len)
{
    if (!len)
        return; // Avoid 0-byte transfers

#if defined(USE_SPI_DMA)
        // TODO: Implement DMA
#endif // end USE_SPI_DMA

    // All other cases (non-DMA hard SPI, bitbang SPI, parallel)...

    if (connection == TFT_HARD_SPI)
    {
        // Fill array of colors
        uint8_t hi = highByte(color), lo = lowByte(color);
        if (hi == lo) // if hi byte equals low byte, set all bytes using memset
        {
            memset(spi_buffer, hi, SPI_BUFFER_SIZE);
        }
        else // otherwise, loop through buffer setting pairs of bytes
        {
            uint16_t data = SWAP_BYTES(color);
            for (uint16_t *ptr = (uint16_t *)spi_buffer; ptr < (uint16_t *)(spi_buffer + SPI_BUFFER_SIZE); ptr++)
                *ptr = data;
        }
        // Write array of bytes to SPI
        for (int remaining_bytes = 2 * len; remaining_bytes > 0; remaining_bytes -= SPI_BUFFER_SIZE)
            hwspi._spi->write((char *)spi_buffer, std::min(remaining_bytes, SPI_BUFFER_SIZE), (char *)NULL, 0);
        /*
        // Write each byte
        while (len--)
        {
            hwspi._spi->write(hi);
            hwspi._spi->write(lo);
        }
        */
    }
    else if (connection == TFT_SOFT_SPI)
    {
        while (len--)
        {
            // Bit-Bang the data out
            for (uint16_t bit = 0, x = color; bit < 16; bit++)
            {
                if (x & 0x8000)
                    SPI_MOSI_HIGH();
                else
                    SPI_MOSI_LOW();
                SPI_SCK_HIGH();
                x <<= 1;
                SPI_SCK_LOW();
            }
        }
    }
    /*
    // TODO: Implement Parallel
    else
    {   // TFT_PARALLEL
        if (hi == lo)
        {
        }
        else
        {
            while (len--)
            {
                TFT_WR_STROBE();
            }
        }
    }
    */
}

/*!
    @brief  Draw a filled rectangle to the display. Not self-contained;
            should follow startWrite(). Typically used by higher-level
            graphics primitives; user code shouldn't need to call this and
            is likely to use the self-contained fillRect() instead.
            writeFillRect() performs its own edge clipping and rejection;
            see writeFillRectPreclipped() for a more 'raw' implementation.
    @param  x      Horizontal position of first corner.
    @param  y      Vertical position of first corner.
    @param  w      Rectangle width in pixels (positive = right of first
                   corner, negative = left of first corner).
    @param  h      Rectangle height in pixels (positive = below first
                   corner, negative = above first corner).
    @param  color  16-bit fill color in '565' RGB format.
    @note   Written in this deep-nested way because C by definition will
            optimize for the 'if' case, not the 'else' -- avoids branches
            and rejects clipped rectangles at the least-work possibility.
*/
void Adafruit_SPITFT::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (w && h)
    { // Nonzero width and height?
        if (w < 0)
        {               // If negative width...
            x += w + 1; //   Move X to left edge
            w = -w;     //   Use positive width
        }
        if (x < _width)
        { // Not off right
            if (h < 0)
            {               // If negative height...
                y += h + 1; //   Move Y to top edge
                h = -h;     //   Use positive height
            }
            if (y < _height)
            { // Not off bottom
                int16_t x2 = x + w - 1;
                if (x2 >= 0)
                { // Not off left
                    int16_t y2 = y + h - 1;
                    if (y2 >= 0)
                    { // Not off top
                        // Rectangle partly or fully overlaps screen
                        if (x < 0)
                        {
                            x = 0;
                            w = x2 + 1;
                        } // Clip left
                        if (y < 0)
                        {
                            y = 0;
                            h = y2 + 1;
                        } // Clip top
                        if (x2 >= _width)
                        {
                            w = _width - x;
                        } // Clip right
                        if (y2 >= _height)
                        {
                            h = _height - y;
                        } // Clip bottom
                        writeFillRectPreclipped(x, y, w, h, color);
                    }
                }
            }
        }
    }
}

/*!
    @brief  Draw a horizontal line on the display. Performs edge clipping
            and rejection. Not self-contained; should follow startWrite().
            Typically used by higher-level graphics primitives; user code
            shouldn't need to call this and is likely to use the self-
            contained drawFastHLine() instead.
    @param  x      Horizontal position of first point.
    @param  y      Vertical position of first point.
    @param  w      Line width in pixels (positive = right of first point,
                   negative = point of first corner).
    @param  color  16-bit line color in '565' RGB format.
*/
void inline Adafruit_SPITFT::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    if ((y >= 0) && (y < _height) && w)
    { // Y on screen, nonzero width
        if (w < 0)
        {               // If negative width...
            x += w + 1; //   Move X to left edge
            w = -w;     //   Use positive width
        }
        if (x < _width)
        { // Not off right
            int16_t x2 = x + w - 1;
            if (x2 >= 0)
            { // Not off left
                // Line partly or fully overlaps screen
                if (x < 0)
                {
                    x = 0;
                    w = x2 + 1;
                } // Clip left
                if (x2 >= _width)
                {
                    w = _width - x;
                } // Clip right
                writeFillRectPreclipped(x, y, w, 1, color);
            }
        }
    }
}

/*!
    @brief  Draw a vertical line on the display. Performs edge clipping and
            rejection. Not self-contained; should follow startWrite().
            Typically used by higher-level graphics primitives; user code
            shouldn't need to call this and is likely to use the self-
            contained drawFastVLine() instead.
    @param  x      Horizontal position of first point.
    @param  y      Vertical position of first point.
    @param  h      Line height in pixels (positive = below first point,
                   negative = above first point).
    @param  color  16-bit line color in '565' RGB format.
*/
void inline Adafruit_SPITFT::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    if ((x >= 0) && (x < _width) && h)
    { // X on screen, nonzero height
        if (h < 0)
        {               // If negative height...
            y += h + 1; //   Move Y to top edge
            h = -h;     //   Use positive height
        }
        if (y < _height)
        { // Not off bottom
            int16_t y2 = y + h - 1;
            if (y2 >= 0)
            { // Not off top
                // Line partly or fully overlaps screen
                if (y < 0)
                {
                    y = 0;
                    h = y2 + 1;
                } // Clip top
                if (y2 >= _height)
                {
                    h = _height - y;
                } // Clip bottom
                writeFillRectPreclipped(x, y, 1, h, color);
            }
        }
    }
}

/*!
    @brief  A lower-level version of writeFillRect(). This version requires
            all inputs are in-bounds, that width and height are positive,
            and no part extends offscreen. NO EDGE CLIPPING OR REJECTION IS
            PERFORMED. If higher-level graphics primitives are written to
            handle their own clipping earlier in the drawing process, this
            can avoid unnecessary function calls and repeated clipping
            operations in the lower-level functions.
    @param  x      Horizontal position of first corner. MUST BE WITHIN
                   SCREEN BOUNDS.
    @param  y      Vertical position of first corner. MUST BE WITHIN SCREEN
                   BOUNDS.
    @param  w      Rectangle width in pixels. MUST BE POSITIVE AND NOT
                   EXTEND OFF SCREEN.
    @param  h      Rectangle height in pixels. MUST BE POSITIVE AND NOT
                   EXTEND OFF SCREEN.
    @param  color  16-bit fill color in '565' RGB format.
    @note   This is a new function, no graphics primitives besides rects
            and horizontal/vertical lines are written to best use this yet.
*/
inline void Adafruit_SPITFT::writeFillRectPreclipped(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    setAddrWindow(x, y, w, h);
    writeColor(color, (uint32_t)w * h);
}

// -------------------------------------------------------------------------
// Ever-so-slightly higher-level graphics operations. Similar to the 'write'
// functions above, but these contain their own chip-select and SPI
// transactions as needed (via startWrite(), endWrite()). They're typically
// used solo -- as graphics primitives in themselves, not invoked by higher-
// level primitives (which should use the functions above for better
// performance).

/*!
    @brief  Draw a single pixel to the display at requested coordinates.
            Self-contained and provides its own transaction as needed
            (see writePixel(x,y,color) for a lower-level variant).
            Edge clipping is performed here.
    @param  x      Horizontal position (0 = left).
    @param  y      Vertical position   (0 = top).
    @param  color  16-bit pixel color in '565' RGB format.
*/
void Adafruit_SPITFT::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    // Clip first...
    if ((x >= 0) && (x < _width) && (y >= 0) && (y < _height))
    {
        // THEN set up transaction (if needed) and draw...
        startWrite();
        setAddrWindow(x, y, 1, 1);
        SPI_WRITE16(color);
        endWrite();
    }
}

/*!
    @brief  Draw a filled rectangle to the display. Self-contained and
            provides its own transaction as needed (see writeFillRect() or
            writeFillRectPreclipped() for lower-level variants). Edge
            clipping and rejection is performed here.
    @param  x      Horizontal position of first corner.
    @param  y      Vertical position of first corner.
    @param  w      Rectangle width in pixels (positive = right of first
                   corner, negative = left of first corner).
    @param  h      Rectangle height in pixels (positive = below first
                   corner, negative = above first corner).
    @param  color  16-bit fill color in '565' RGB format.
    @note   This repeats the writeFillRect() function almost in its entirety,
            with the addition of a transaction start/end. It's done this way
            (rather than starting the transaction and calling writeFillRect()
            to handle clipping and so forth) so that the transaction isn't
            performed at all if the rectangle is rejected. It's really not
            that much code.
*/
void Adafruit_SPITFT::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (w && h)
    { // Nonzero width and height?
        if (w < 0)
        {               // If negative width...
            x += w + 1; //   Move X to left edge
            w = -w;     //   Use positive width
        }
        if (x < _width)
        { // Not off right
            if (h < 0)
            {               // If negative height...
                y += h + 1; //   Move Y to top edge
                h = -h;     //   Use positive height
            }
            if (y < _height)
            { // Not off bottom
                int16_t x2 = x + w - 1;
                if (x2 >= 0)
                { // Not off left
                    int16_t y2 = y + h - 1;
                    if (y2 >= 0)
                    { // Not off top
                        // Rectangle partly or fully overlaps screen
                        if (x < 0)
                        {
                            x = 0;
                            w = x2 + 1;
                        } // Clip left
                        if (y < 0)
                        {
                            y = 0;
                            h = y2 + 1;
                        } // Clip top
                        if (x2 >= _width)
                        {
                            w = _width - x;
                        } // Clip right
                        if (y2 >= _height)
                        {
                            h = _height - y;
                        } // Clip bottom
                        startWrite();
                        writeFillRectPreclipped(x, y, w, h, color);
                        endWrite();
                    }
                }
            }
        }
    }
}

/*!
    @brief  Draw a horizontal line on the display. Self-contained and
            provides its own transaction as needed (see writeFastHLine() for
            a lower-level variant). Edge clipping and rejection is performed
            here.
    @param  x      Horizontal position of first point.
    @param  y      Vertical position of first point.
    @param  w      Line width in pixels (positive = right of first point,
                   negative = point of first corner).
    @param  color  16-bit line color in '565' RGB format.
    @note   This repeats the writeFastHLine() function almost in its
            entirety, with the addition of a transaction start/end. It's
            done this way (rather than starting the transaction and calling
            writeFastHLine() to handle clipping and so forth) so that the
            transaction isn't performed at all if the line is rejected.
*/
void Adafruit_SPITFT::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
    if ((y >= 0) && (y < _height) && w)
    { // Y on screen, nonzero width
        if (w < 0)
        {               // If negative width...
            x += w + 1; //   Move X to left edge
            w = -w;     //   Use positive width
        }
        if (x < _width)
        { // Not off right
            int16_t x2 = x + w - 1;
            if (x2 >= 0)
            { // Not off left
                // Line partly or fully overlaps screen
                if (x < 0)
                {
                    x = 0;
                    w = x2 + 1;
                } // Clip left
                if (x2 >= _width)
                {
                    w = _width - x;
                } // Clip right
                startWrite();
                writeFillRectPreclipped(x, y, w, 1, color);
                endWrite();
            }
        }
    }
}

/*!
    @brief  Draw a vertical line on the display. Self-contained and provides
            its own transaction as needed (see writeFastHLine() for a lower-
            level variant). Edge clipping and rejection is performed here.
    @param  x      Horizontal position of first point.
    @param  y      Vertical position of first point.
    @param  h      Line height in pixels (positive = below first point,
                   negative = above first point).
    @param  color  16-bit line color in '565' RGB format.
    @note   This repeats the writeFastVLine() function almost in its
            entirety, with the addition of a transaction start/end. It's
            done this way (rather than starting the transaction and calling
            writeFastVLine() to handle clipping and so forth) so that the
            transaction isn't performed at all if the line is rejected.
*/
void Adafruit_SPITFT::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
    if ((x >= 0) && (x < _width) && h)
    { // X on screen, nonzero height
        if (h < 0)
        {               // If negative height...
            y += h + 1; //   Move Y to top edge
            h = -h;     //   Use positive height
        }
        if (y < _height)
        { // Not off bottom
            int16_t y2 = y + h - 1;
            if (y2 >= 0)
            { // Not off top
                // Line partly or fully overlaps screen
                if (y < 0)
                {
                    y = 0;
                    h = y2 + 1;
                } // Clip top
                if (y2 >= _height)
                {
                    h = _height - y;
                } // Clip bottom
                startWrite();
                writeFillRectPreclipped(x, y, 1, h, color);
                endWrite();
            }
        }
    }
}

/*!
    @brief  Essentially writePixel() with a transaction around it. I don't
            think this is in use by any of our code anymore (believe it was
            for some older BMP-reading examples), but is kept here in case
            any user code relies on it. Consider it DEPRECATED.
    @param  color  16-bit pixel color in '565' RGB format.
*/
void Adafruit_SPITFT::pushColor(uint16_t color)
{
    startWrite();
    SPI_WRITE16(color);
    endWrite();
}

/*!
    @brief  Draw a 16-bit image (565 RGB) at the specified (x,y) position.
            For 16-bit display devices; no color reduction performed.
            Adapted from https://github.com/PaulStoffregen/ILI9341_t3
            by Marc MERLIN. See examples/pictureEmbed to use this.
            5/6/2017: function name and arguments have changed for
            compatibility with current GFX library and to avoid naming
            problems in prior implementation.  Formerly drawBitmap() with
            arguments in different order. Handles its own transaction and
            edge clipping/rejection.
    @param  x        Top left corner horizontal coordinate.
    @param  y        Top left corner vertical coordinate.
    @param  pcolors  Pointer to 16-bit array of pixel values.
    @param  w        Width of bitmap in pixels.
    @param  h        Height of bitmap in pixels.
*/
void Adafruit_SPITFT::drawRGBBitmap(int16_t x, int16_t y, uint16_t *pcolors, int16_t w, int16_t h)
{

    int16_t x2, y2;                 // Lower-right coord
    if ((x >= _width) ||            // Off-edge right
        (y >= _height) ||           // " top
        ((x2 = (x + w - 1)) < 0) || // " left
        ((y2 = (y + h - 1)) < 0))
        return; // " bottom

    int16_t bx1 = 0, by1 = 0, // Clipped top-left within bitmap
        saveW = w;            // Save original bitmap width value
    if (x < 0)
    { // Clip left
        w += x;
        bx1 = -x;
        x = 0;
    }
    if (y < 0)
    { // Clip top
        h += y;
        by1 = -y;
        y = 0;
    }
    if (x2 >= _width)
        w = _width - x; // Clip right
    if (y2 >= _height)
        h = _height - y; // Clip bottom

    pcolors += by1 * saveW + bx1; // Offset bitmap ptr to clipped top-left
    startWrite();
    setAddrWindow(x, y, w, h); // Clipped area
    while (h--)
    {                            // For each (clipped) scanline...
        writePixels(pcolors, w); // Push one (clipped) row
        pcolors += saveW;        // Advance pointer by one full (unclipped) line
    }
    endWrite();
}

/**************************************************************************/
/*!
   @brief      Draw PROGMEM-resident XBitMap Files (*.xbm), exported from GIMP. 
   Usage: Export from GIMP to *.xbm, rename *.xbm to *.c and open in editor.
   C Array can be directly used with this function.
   There is no RAM-resident version of this function; if generating bitmaps
   in RAM, use the format defined by drawBitmap() and call that instead.
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Hieght of bitmap in pixels
    @param    color 16-bit 5-6-5 Color to draw pixels with
*/
/**************************************************************************/
void Adafruit_SPITFT::drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t fg_color, uint16_t bg_color)
{
    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad = whole byte
    uint8_t byte = 0;

    // Nearly identical to drawBitmap(), only the bit order
    // is reversed here (left-to-right = LSB to MSB):

    uint16_t fg_color_data = SWAP_BYTES(fg_color);
    uint16_t bg_color_data = SWAP_BYTES(bg_color);

    startWrite();
    for (int16_t j = 0; j < h; j++, y++)
    {
        for (int16_t i = 0; i < w; i++)
        {
            if (i & 7)
                byte >>= 1;
            else
                byte = pgm_read_byte(&bitmap[j * byteWidth + i / 8]);

            // if (byte & 0x01)
            //     writePixel(x + i, y, fg_color);
            if (byte & 0x01)
                *(((uint16_t *)spi_buffer) + i) = fg_color_data;
            else
                *(((uint16_t *)spi_buffer) + i) = bg_color_data;
        }
        setAddrWindow(x, y, w, 1); // Clipped area
        hwspi._spi->write((char *)spi_buffer, 2 * w, (char *)NULL, 0);
    }
    endWrite();
}

// -------------------------------------------------------------------------
// Miscellaneous class member functions that don't draw anything.

/*!
    @brief  Invert the colors of the display (if supported by hardware).
            Self-contained, no transaction setup required.
    @param  i  true = inverted display, false = normal display.
*/
void Adafruit_SPITFT::invertDisplay(bool i)
{
    startWrite();
    writeCommand(i ? invertOnCommand : invertOffCommand);
    endWrite();
}

/*!
    @brief   Given 8-bit red, green and blue values, return a 'packed'
             16-bit color value in '565' RGB format (5 bits red, 6 bits
             green, 5 bits blue). This is just a mathematical operation,
             no hardware is touched.
    @param   red    8-bit red brightnesss (0 = off, 255 = max).
    @param   green  8-bit green brightnesss (0 = off, 255 = max).
    @param   blue   8-bit blue brightnesss (0 = off, 255 = max).
    @return  'Packed' 16-bit color value (565 format).
*/
uint16_t Adafruit_SPITFT::color565(uint8_t red, uint8_t green, uint8_t blue)
{
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
}

// -------------------------------------------------------------------------
// Lowest-level hardware-interfacing functions. Many of these are inline and
// compile to different things based on #defines -- typically just a few
// instructions. Others, not so much, those are not inlined.

/*!
    @brief  Start an SPI transaction if using the hardware SPI interface to
            the display. If using an earlier version of the Arduino platform
            (before the addition of SPI transactions), this instead attempts
            to set up the SPI clock and mode. No action is taken if the
            connection is not hardware SPI-based. This does NOT include a
            chip-select operation -- see startWrite() for a function that
            encapsulated both actions.
*/
inline void Adafruit_SPITFT::SPI_BEGIN_TRANSACTION(void)
{
    /*
    if (connection == TFT_HARD_SPI)
    {
        hwspi._spi->format(hwspi._bits, hwspi._mode);
        hwspi._spi->frequency(hwspi._freq);
    }
    */
}

/*!
    @brief  End an SPI transaction if using the hardware SPI interface to
            the display. No action is taken if the connection is not
            hardware SPI-based or if using an earlier version of the Arduino
            platform (before the addition of SPI transactions). This does
            NOT include a chip-deselect operation -- see endWrite() for a
            function that encapsulated both actions.
*/
inline void Adafruit_SPITFT::SPI_END_TRANSACTION(void)
{
}

/*!
    @brief  Issue a single 8-bit value to the display. Chip-select,
            transaction and data/command selection must have been
            previously set -- this ONLY issues the byte. This is another of
            those functions in the library with a now-not-accurate name
            that's being maintained for compatibility with outside code.
            This function is used even if display connection is parallel.
    @param  b  8-bit value to write.
*/
void Adafruit_SPITFT::SPI_WRITE8(uint8_t b)
{
    if (connection == TFT_HARD_SPI)
    {
        // Write a byte of data
        hwspi._spi->write(b);
    }
    else if (connection == TFT_SOFT_SPI)
    {
        // Bit-Bang the data out
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if (b & 0x80)
                SPI_MOSI_HIGH();
            else
                SPI_MOSI_LOW();
            SPI_SCK_HIGH();
            b <<= 1;
            SPI_SCK_LOW();
        }
    }
    /*
    // TODO: Implement Parallel
    else
    {   // TFT_PARALLEL
        TFT_WR_STROBE();
    }
    */
}

/*!
    @brief  Write a single command byte to the display. Chip-select and
            transaction must have been previously set -- this ONLY sets
            the device to COMMAND mode, issues the byte and then restores
            DATA mode. There is no corresponding explicit writeData()
            function -- just use SPI_WRITE8().
    @param  cmd  8-bit command to write.
*/
void Adafruit_SPITFT::writeCommand(uint8_t cmd)
{
    SPI_DC_LOW();
    SPI_WRITE8(cmd);
    SPI_DC_HIGH();
}

/*!
    @brief   Read a single 8-bit value from the display. Chip-select and
             transaction must have been previously set -- this ONLY reads
             the byte. This is another of those functions in the library
             with a now-not-accurate name that's being maintained for
             compatibility with outside code. This function is used even if
             display connection is parallel.
    @return  Unsigned 8-bit value read 
*/
uint8_t Adafruit_SPITFT::SPI_READ8(void)
{
    uint8_t b = 0;
    if (connection == TFT_HARD_SPI)
    {
        // Read a byte of data
        return hwspi._spi->write((uint8_t)0);
    }
    else if (connection == TFT_SOFT_SPI)
    {
        if (swspi._miso >= 0)
        {
            // Bit-Bang the data in
            for (uint8_t i = 0; i < 8; i++)
            {
                SPI_SCK_HIGH();
                b <<= 1;
                if (SPI_MISO_READ())
                    b++;
                SPI_SCK_LOW();
            }
        }
        return b;
    }
    /*
    // TODO: Implement Parallel
    else
    {   // TFT_PARALLEL
        uint16_t w = 0;
        if (tft8._rd >= 0)
            w = 0;
        return w;
    }
    */
    return 0;
}

/*!
    @brief  Set the software (bitbang) SPI MOSI line HIGH.
*/
inline void Adafruit_SPITFT::SPI_MOSI_HIGH(void)
{
    digitalWrite(swspi._mosi, HIGH);
}

/*!
    @brief  Set the software (bitbang) SPI MOSI line LOW.
*/
inline void Adafruit_SPITFT::SPI_MOSI_LOW(void)
{
    digitalWrite(swspi._mosi, LOW);
}

/*!
    @brief  Set the software (bitbang) SPI SCK line HIGH.
*/
inline void Adafruit_SPITFT::SPI_SCK_HIGH(void)
{
    digitalWrite(swspi._sck, HIGH);
}

/*!
    @brief  Set the software (bitbang) SPI SCK line LOW.
*/
inline void Adafruit_SPITFT::SPI_SCK_LOW(void)
{
    digitalWrite(swspi._sck, LOW);
}

/*!
    @brief   Read the state of the software (bitbang) SPI MISO line.
    @return  true if HIGH, false if LOW.
*/
inline bool Adafruit_SPITFT::SPI_MISO_READ(void)
{
    return digitalRead(swspi._miso);
}

/*!
    @brief  Issue a single 16-bit value to the display. Chip-select,
            transaction and data/command selection must have been
            previously set -- this ONLY issues the word. Despite the name,
            this function is used even if display connection is parallel;
            name was maintaned for backward compatibility. 
    @param  w  16-bit value to write.
*/
void Adafruit_SPITFT::SPI_WRITE16(uint16_t w)
{
    if (connection == TFT_HARD_SPI)
    {
        uint8_t data[] = {(uint8_t)(w >> 8), (uint8_t)w};
        // Write data
        hwspi._spi->write((char *)data, sizeof(data), (char *)NULL, 0);
        /*
        // Write hi byte
        hwspi._spi->write(highByte(w));
        // Write lo byte
        hwspi._spi->write(lowByte(w));
        */
    }
    else if (connection == TFT_SOFT_SPI)
    {
        // Bit-Bang the data out
        for (uint8_t bit = 0; bit < 16; bit++)
        {
            if (w & 0x8000)
                SPI_MOSI_HIGH();
            else
                SPI_MOSI_LOW();
            SPI_SCK_HIGH();
            SPI_SCK_LOW();
            w <<= 1;
        }
    }
    /*
    // TODO: Implement Parallel
    else
    {   // TFT_PARALLEL
        TFT_WR_STROBE();
    }
    */
}

/*!
    @brief  Issue a single 32-bit value to the display. Chip-select,
            transaction and data/command selection must have been
            previously set -- this ONLY issues the longword. Despite the
            name, this function is used even if display connection is
            parallel; name was maintaned for backward compatibility. 
    @param  l  32-bit value to write.
*/
void Adafruit_SPITFT::SPI_WRITE32(uint32_t l)
{
    if (connection == TFT_HARD_SPI)
    {
        uint8_t data[] = {(uint8_t)(l >> 24), (uint8_t)(l >> 16), (uint8_t)(l >> 8), (uint8_t)l};
        // Write data
        hwspi._spi->write((char *)data, sizeof(data), (char *)NULL, 0);
        /*
        // Write out 4 bytes of data
        hwspi._spi->write((uint8_t)(l >> 24));
        hwspi._spi->write((uint8_t)(l >> 16));
        hwspi._spi->write((uint8_t)(l >> 8));
        hwspi._spi->write((uint8_t)l);
        */
    }
    else if (connection == TFT_SOFT_SPI)
    {
        // Bit-Bang the data out
        for (uint8_t bit = 0; bit < 32; bit++)
        {
            if (l & 0x80000000)
                SPI_MOSI_HIGH();
            else
                SPI_MOSI_LOW();
            SPI_SCK_HIGH();
            SPI_SCK_LOW();
            l <<= 1;
        }
    }
    /*
    // TODO: Implement Parallel
    else
    {   // TFT_PARALLEL
        TFT_WR_STROBE();
    }
    */
}

/*!
    @brief  Set the WR line LOW, then HIGH. Used for parallel-connected
            interfaces when writing data.
*/
inline void Adafruit_SPITFT::TFT_WR_STROBE(void)
{
    digitalWrite(tft8._wr, LOW);
    digitalWrite(tft8._wr, HIGH);
}

/*!
    @brief  Set the RD line HIGH. Used for parallel-connected interfaces
            when reading data.
*/
inline void Adafruit_SPITFT::TFT_RD_HIGH(void)
{
    digitalWrite(tft8._rd, HIGH);
}

/*!
    @brief  Set the RD line LOW. Used for parallel-connected interfaces
            when reading data.
*/
inline void Adafruit_SPITFT::TFT_RD_LOW(void)
{
    digitalWrite(tft8._rd, LOW);
}
