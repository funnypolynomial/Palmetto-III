#!/usr/bin/python
import os
import sys
import math
from PIL import Image # https://pillow.readthedocs.io/en/stable/

# encode fonts
# each char is 1 bit per pixel block, most significant bit is leftmost pixel, 1=foreground
# uses top-left pixel as white/background, mono pngs, the palette entries can be weird

outputHex = False # hex or binary font data

# NOTE: output of hex() and bin() start with "0x" & "0b" BUT oct() may start with "0o" or just "0"? (version specific?)
def ByteStr(b):
    if outputHex:
        return "0x" + hex(256 + b)[3:].upper()
    else:
        return "0b" + bin(256 + b)[3:].upper()
        
def ByteCharStr(b):
    # octal works better with "0x08""01234... " in map.
    octal = oct(512 + b) # force leading 0's
    return "\\" + octal.replace("o", "")[2:]

    
def FirstNonBlankCol(cellX, cellY, startRow, numRows, cellWidth, white):
    # return the first col with a pixel set, or cellX
    firstCol = cellX
    for col in range(cellWidth - 2):
        for row in range(numRows):
            if image.getpixel((cellX + col, cellY + startRow + row)) != white:
                return cellX + col
    return cellX

def WriteBlock(char, block):
    # block is list of lists of 1 & 0
    bytes = 0
    if ord(char) > 127 or ord(char) < 32:
        sourceFile.write("  // " + ByteCharStr(ord(char)) + "\n")
    elif char != '\\':
        sourceFile.write("  // " + char + "\n")
    else:
        sourceFile.write("  // <backslash>\n")
    for r in block:
        byte = 0
        sourceFile.write("  ")
        bytes += 1
        ctr = 0
        for c in r:
            byte = byte*2 + c
            ctr += 1
            if ctr == 8:
                ctr = 0
                sourceFile.write(ByteStr(byte) + ", ")
                byte = 0
                bytes += 1
        if len(r) % 8:
            for c in range(8 - (len(r) % 8)):
                byte = byte*2 # round-out byte
            sourceFile.write(ByteStr(byte) + ", ")
        sourceFile.write("\n")
    return bytes
    
def EscapeChars(str):
    # escape non-printable chars as octal
    result = ""
    for ch in str:
        if ord(ch) > 127 or ord(ch) < 32:
            result = result + ByteCharStr(ord(ch))
        else:
            result = result + ch
    return result

def RangeChar(ch):
    # fix comma or quotes as arg
    if ch == ",":
        return "\\054"      # octal comma
    elif ch == "\"":
        return "\\134\\042" # octal backslash, double quote
    else:
        return ch

def InsertRanges(str):
    # inserts RANGE macros for runs of sequential chars longer than 3
    mapStr = ""
    prevCh = -999
    rangeStr = ""
    nonRangeStr = ""
    for ch in str:
        if prevCh != -999 and ch == chr(ord(prevCh) + 1):  # range
            if len(rangeStr) == 0:
                rangeStr = prevCh + rangeStr
                nonRangeStr = nonRangeStr[:-1]
            if len(nonRangeStr) > 0:
                mapStr += "\"" + nonRangeStr + "\" "
                nonRangeStr = ""
            rangeStr += ch
        else:
            if len(rangeStr) > 0:
                # end of a range
                if len(rangeStr) < 3: # too short
                    mapStr += "\"" + rangeStr + "\" "
                else:
                    mapStr += " RANGE(" + RangeChar(rangeStr[0]) + ", " + RangeChar(rangeStr[len(rangeStr)-1]) + ") "
                rangeStr = ""
            nonRangeStr += ch
        prevCh = ch
    if len(nonRangeStr) > 0:
        mapStr += "\"" + nonRangeStr + "\" "
        nonRangeStr = ""
    if len(rangeStr) > 0:
        # end of a range
        if len(rangeStr) < 3: # too short
            mapStr += "\"" + rangeStr + "\" "
        else:
            mapStr += " RANGE(" + RangeChar(rangeStr[0]) + ", " + RangeChar(rangeStr[len(rangeStr)-1]) + ") "
        rangeStr = ""
    mapStr = mapStr.rstrip()
    mapStr = mapStr.replace("\" \"", "")
    mapStr = mapStr.replace("  ", " ")
    return EscapeChars(mapStr)

def EncodeChars(startX, startY, cellWidth, cellHeight, startRow, numRows, numCols, cellCols, cellRows, descender, startChar, lastChar, name, Map,
                fixedWidthDigitsFrom=0):
    # pixel coords of first cell; size of cell; top row of char and number of rows; cells; descender; first char, last char (to scan). Map is chars to write
    # PROPS define is 0xDDHHWW WW=char nominal width, HH=full height, DD=height of descender (not used)
    headerFile.write("  // " + name + "\n")
    headerFile.write("  static const unsigned long k" + name + "Props = PROPS(" + str(descender) + ", " + str(numRows) + ", " + str(numCols) + ");\n")
    headerFile.write("  extern const char          p" + name + "Map_P[];\n")
    headerFile.write("  extern const unsigned char p" + name + "Font_P[];\n\n")
    
    sourceFile.write("// Map: \"" + EscapeChars(Map) + "\"\n")
    sourceFile.write("static const char p" + name + "Map_P[] PROGMEM = " + InsertRanges(Map) + ";\n")
    sourceFile.write("static const unsigned char p" + name + "Font_P[] PROGMEM =\n{\n")
    count = 0
    white = image.getpixel((startX, startY))
    for cellRow in range(cellRows):
        cellY = startY + cellRow*cellHeight
        for cellCol in range(cellCols):
            if Map == "" or Map.find(startChar) != -1:
                cellX = startX + cellCol*cellWidth
                if fixedWidthDigitsFrom != 0 and '0' <= startChar and startChar <= '9':
                    # grab digits in a fixed-width block, make them monospaced, it's probably actually kerning
                    firstCol = cellX + fixedWidthDigitsFrom
                    lastCol = firstCol + numCols
                else:
                    # in each cell find the first non-blank column
                    firstCol = FirstNonBlankCol(cellX, cellY, startRow, numRows, cellWidth, white)
                    lastCol = min(firstCol + numCols, cellX + cellWidth - 2)
                block = []
                for row in range(startRow, startRow + numRows):
                    r = []
                    for col in range(firstCol, lastCol):
                        if image.getpixel((col, cellY + row)) == white:
                            r.append(0)
                        else:
                            r.append(1)
                    block.append(r)
                count += WriteBlock(startChar, block) 
            startChar = chr(ord(startChar) + 1)
            if ord(startChar) > ord(lastChar):
                break
        if ord(startChar) > ord(lastChar):
            break
    global grand_total
    grand_total += count
    sourceFile.write("}; // " + str(count) + " bytes\n\n")

# ****************************
headerFile = open("FontData.h", "w")
headerFile.write("#pragma once\n\n")
headerFile.write("namespace FontData\n{\n")
headerFile.write("  // PROPS is 0xDDHHWW WW=char nominal width, HH=full height, DD=height of descender (not used)\n")
headerFile.write("  #define PROPS(_descender, _height, _width) (unsigned long int)((((unsigned long int)_descender)<<16) | (((unsigned long int)_height)<<8) | ((unsigned long int)_width))\n\n")

sourceFile = open("FontData.cpp", "w")
sourceFile.write("#include <avr/pgmspace.h>\n")
sourceFile.write("#include \"FontData.h\"\n")
sourceFile.write("namespace FontData\n{\n")
sourceFile.write("// Defines char range in map\n")
sourceFile.write("#define RANGE(_from, _to) #_to #_from\n\n")

grand_total = 0

# NOTE: gaps in maps are OK, but the chars must be in order (apart from RANGE)
# ******* Standard *******
image = Image.open("font_standard.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
# starts on row 2, 10 rows high, 2 rows of descender
# max 8 wide
# grid@ 20, 100. 14x14
EncodeChars(20, 100,        # pixel coords of first character cell
            14, 14,         # width & height of cells
            2, 10,          # first row in cell to scan, number of rows to scan (=text height)
            8,              # cols in cell to scan (nominal width)
            16, 10,         # columns and rows of cells to scan
            2,              # height of text descender
            ' ', '\xBB',    # first & last char
            "Standard",     # fontname
            ",-./0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz\x85\xA9\xBB",      # map (ellipsis, copyright, right chevron)
            4)

# ******* Large *******
image = Image.open("font_large.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
# starts on row 2, 13 rows high, 3 rows of descender
# max 10 wide
# grid 20, 106.  16x17
EncodeChars(20, 106,        # pixel coords of first character cell
            16, 17,         # width & height of cells
            3, 13,          # first row in cell to scan, number of rows to scan (=text height)
            8,              # cols in cell to scan (nominal width)
            16, 6,          # columns and rows of cells to scan
            2,              # height of text descender
            ' ', '~',       # first & last char
            "Large",        # fontname
            "0123456789ABCDEFGHIJKLNOPQRSTUVXYZabcdefghijklnopqrstuvxyz"         # map
            )
           
# only 4 chars need the extra byte per row
EncodeChars(20, 106,        # pixel coords of first character cell
            16, 17,         # width & height of cells
            3, 13,          # first row in cell to scan, number of rows to scan (=text height)
            10,             # cols in cell to scan (nominal width)
            16, 6,          # columns and rows of cells to scan
            2,              # height of text descender
            ' ', '~',       # first & last char
            "LargeWide",    # fontname
            "MWmw")         # map

# ******* Bold *******
image = Image.open("font_bold.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
# starts on row 2, 10 rows high, 2 rows of descender
# max 10 wide
# grid 20, 100. 20x14
EncodeChars(20, 100,        # pixel coords of first character cell
            20, 14,         # width & height of cells
            2, 10,          # first row in cell to scan, number of rows to scan (=text height)
            8,              # cols in cell to scan (nominal width)
            16, 6,          # columns and rows of cells to scan
            2,              # height of text descender
            ' ', '~',       # first & last char
            "Bold",         # fontname
            "-./0123456789:ABCDEFGHIJKLNOPQRSTUVXYZabcdefghijklmnopqrstuvwxyz"      # map
            )
# A secondary table of the wide chars to reduce the memory footprint             
EncodeChars(20, 100,        # pixel coords of first character cell
            20, 14,         # width & height of cells
            2, 10,          # first row in cell to scan, number of rows to scan (=text height)
            10,             # cols in cell to scan (nominal width)
            16, 6,          # columns and rows of cells to scan
            2,              # height of text descender
            ' ', '~',       # first & last char
            "BoldWide",     # fontname
            "MW"            # map
            )

# ******* Large Bold *******
# VERY LIMITED! Just enough for the Splash screen
image = Image.open("font_large_bold.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeChars(20, 108,        # pixel coords of first character cell
            15, 18,         # width & height of cells
            3, 10,          # first row in cell to scan, number of rows to scan (=text height)
            8,              # cols in cell to scan (nominal width)
            16, 6,          # columns and rows of cells to scan
            3,              # height of text descender
            ' ', '~',       # first & last char
            "LargeBold",    # fontname
            "IPaelmot"      # map, just Palmetto III
            )

# ******* LED *******
# DIGITS ONLY!
image = Image.open("font_led.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
# Note that "font_led.png" has dummy white pixels in the 0x20 cell so the background determination works
# starts on row 3, 14 rows high, 0 rows of descender
# max 8 wide
# grid 20, 116. 18x22
# fixed width?
EncodeChars(20, 116,        # pixel coords of first character cell
            18, 22,         # width & height of cells
            3, 14,          # first row in cell to scan, number of rows to scan (=text height)
            8,              # cols in cell to scan (nominal width)
            16, 2,          # columns and rows of cells to scan
            0,              # height of text descender
            ' ', '~',       # first & last char
            "LED",          # fontname
            "-0123456789",  # map
            4)              # fixed width digits

# ******* Graffiti *******
# used by the "keyboard"
image = Image.open("font_graffiti.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeChars(1, 1,           # pixel coords of first character cell
            23, 23,         # width & height of cells
            2, 17,          # first row in cell to scan, number of rows to scan (=text height)
            14,             # cols in cell to scan (nominal width)
            16, 6,          # columns and rows of cells to scan
            0,              # height of text descender
            '\x00', 'Z',    # first & last char
            "Graffiti",     # fontname
            "\x08\x0F\x20023569ABCDEGKMNOPQRSUVWXYZ") # map (\x08 = BS \x0F=CAP \x20=SPC [\x0D=P1 \x0E=P2, unused])
# A secondary table of the thin chars to reduce the memory footprint             
EncodeChars(1, 47,          # pixel coords of first character cell
            23, 23,         # width & height of cells
            2, 17,          # first row in cell to scan, number of rows to scan (=text height)
            8,              # cols in cell to scan (nominal width)
            16, 6,          # columns and rows of cells to scan
            0,              # height of text descender
            ' ', 'Z',       # first & last char
            "GraffitiThin", # fontname
            "1478FHIJLT"     # map
            )


sourceFile.write("// total " + str(grand_total) + " bytes\n\n")
sourceFile.write("}\n")
sourceFile.close()
headerFile.write("};\n")
headerFile.close()
sys.stdout.write("Created " + headerFile.name + " and " + sourceFile.name + "\n")
