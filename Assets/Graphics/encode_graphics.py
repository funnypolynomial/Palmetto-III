#!/usr/bin/python
import os
import sys
import math
from PIL import Image # https://pillow.readthedocs.io/en/stable/

# encode mono PNGs
# 1 bit per pixel, most significant bit is leftmost pixel, 1=foreground
# uses top-left pixel as white/background, mono pngs, the palette entries can be weird

outputHex = True # hex or binary

def ByteStr(b):
    if outputHex:
        return "0x" + hex(256 + b)[3:].upper()
    else:
        return "0b" + bin(256 + b)[3:].upper()

def WriteBlock(block):
    # block is list of lists of 1 & 0
    global headerFile
    global sourceFile
    bytes = 0
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

def EncodeImage(name):
    # encode the region as a PROGMEM array called name
    global headerFile
    global sourceFile
    global grand_total
    white = image.getpixel((0, 0))
    count = 0
    w = image.width
    h = image.height
    headerFile.write("  extern const unsigned char p" + name + "Image_P[];\n")
    sourceFile.write("static const uint8_t p" + name + "Image_P[] PROGMEM =\n{\n")
    block = []
    for y in range(h):
        r = []
        for x in range(w):
            if image.getpixel((x, y)) == white:
                r.append(0)
            else:
                r.append(1)
        block.append(r)
    count += WriteBlock(block) 
    grand_total += count
    sourceFile.write("}; // " + str(count) + " bytes\n\n")

def OpenFiles(name):
    global headerFile
    global sourceFile
    global grand_total
    headerFile = open(name + ".h", "w")
    headerFile.write("#pragma once\n\n")
    headerFile.write("\n" + "namespace " + name + "\n{\n")
    sourceFile = open(name + ".cpp", "w")
    sourceFile.write("#include <avr/pgmspace.h>\n")
    sourceFile.write("#include \"" + name + ".h\"\n")
    sourceFile.write("\n" + "namespace " + name + "\n{\n")
    grand_total = 0

def CloseFiles():
    global headerFile
    global sourceFile
    global grand_total
    sys.stdout.write("Created " + headerFile.name + " and " + sourceFile.name + "\n")    
    sourceFile.write("// total " + str(grand_total) + " bytes\n\n")
    headerFile.write("};\n")
    sourceFile.write("}\n")
    sourceFile.close()
    headerFile.close()    

# PDA --------------------------------
OpenFiles("PDAGraphicsData")

icon_ext = ".png" 
# Apps
image = Image.open("apps" + icon_ext)
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeImage("Apps")

# Menu
image = Image.open("menu" + icon_ext)
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeImage("Menu")

# Calc
image = Image.open("calc" + icon_ext)
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeImage("Calc")

# Find
image = Image.open("find" + icon_ext)
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeImage("Find")

# abc
image = Image.open("abc.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeImage("ABC")

# 123
image = Image.open("123.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeImage("123")

CloseFiles()

# tqCD --------------------------------
OpenFiles("tqCDGraphicsData")

# Icon
image = Image.open("tqcd.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeImage("TQCD")

# Logo
image = Image.open("tqlogo.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeImage("Logo")

# URL
image = Image.open("tq_url.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeImage("URL")

CloseFiles()

# Prefs --------------------------------
OpenFiles("PrefsGraphicsData")

# Icon
image = Image.open("prefs.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeImage("Prefs")

# Digitizer target
image = Image.open("digitizer_target.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeImage("Target")

# Digitizer top-left arrow
image = Image.open("digitiser_topleft.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeImage("TopLeftArrow")

# Digitizer bottom-right arrow
image = Image.open("digitiser_botright.png")
sourceFile.write("// (created from \"" + image.filename + "\")\n")
EncodeImage("BotRightArrow")

CloseFiles()
