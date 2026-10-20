#!/usr/bin/python
import sys
import argparse
import os
from PIL import Image

# playback serial capture of LCD commands to build a screen capture
# lines are comma-delimited fields:
#  width,height,pixel-size  (screen size)
# or
#  "filename"               (canvas) TODO?
# or
#  x,y,w,h                  (window defn)
# or
#  n,c                      (n pixels of colour c)
# or
#  c                        (1 pixel of colour c)
# or
#  W or B                   (1 pixel of white or black)
# or
#  !                        (fill with alternating black/white)
# see #define SERIALISE in #include "ILI948x.h" (does not work with LCD_TRANSFORM)

if len(sys.argv) != 2 and len(sys.argv) != 3:
    print 'playback.py <inputfile> {<scale>}'
    sys.exit(2)    
fileName = sys.argv[1]

script = open(fileName, 'r')
winX,winY,winW,winH = 0,0,0,0

bmpScale = -1
if len(sys.argv) == 3:
    bmpScale = int(sys.argv[2])
winIdx = 0
bmpW,bmpH = winW*abs(bmpScale),winH*abs(bmpScale)
bmp = Image.new("RGB", (bmpW,bmpH), "black")
line = script.readline();
lineNum = 1
while line:
    items = line.split(',')
    if line[0] == ';':
        # ignore comment
        pass
    elif line[0] == '\"':
        # todo?
        pass
    elif len(items) == 3:
        # width,height,scale
        winW,winH,tmpScale = int(items[0]),int(items[1]),int(items[2])
        if bmpScale == -1:
            bmpScale = tmpScale
        bmpW,bmpH = winW*bmpScale,winH*bmpScale
        bmp = Image.new("RGB", (bmpW,bmpH), "black")
    elif len(items) == 4:
        # window
        winX,winY,winW,winH = int(items[0]),int(items[1]),int(items[2]),int(items[3])
        winIdx = 0
    elif len(items) == 2 or len(items) == 1:
        # colour data
        if len(items) == 2:
            count,colour = int(items[0]),int(items[1])
        elif items[0] == 'B\n':
            count, colour = 1, 0x0000
        elif items[0] == 'W\n':
            count, colour = 1, 0xFFFF 
        elif items[0] == '!\n':
            for r in range(bmpH):
                for c in range(bmpW):
                    if ((c + (r & 0x0001)) & 0x0001):
                      bmp.putpixel((c, r), (255, 255, 255))
                    else:
                      bmp.putpixel((c, r), (0, 0, 0))
            count = 0
        else:
            count,colour = 1, int(items[0])
        if count:
            red = int((colour >> 11)*255/31)
            green = int(((colour >> 5) & 0x3F)*255/63)
            blue = int((colour & 0x1F)*255/31)
            if red == green and green == blue:
                # lighten dark greys
                if 0 < red and red < 32:
                    red = red*4
                    green,blue = red,red
                if 32 <= red and red < 64:
                    red = red + red/2
                    green,blue = red,red
            rgb = (red, green, blue)
            for ctr in range(count):
                pixX = bmpScale*(winX + (winIdx % winW))
                pixY = bmpScale*(winY + (winIdx / winW))
                if 0 <= pixX and pixX < bmpW and 0 <= pixY and pixY < bmpH:
                    for dX in range(bmpScale):
                        for dY in range(bmpScale):
                            bmp.putpixel((pixX+dX, pixY+dY), rgb)
                else:
                    print "bad data at %i (%i,%i,%i,%i)@[%i,%i]%i" % (lineNum, winX,winY,winW,winH,pixX,pixY,winIdx)
                winIdx = winIdx + 1
    line = script.readline();
    lineNum = lineNum + 1


bmp.save(os.path.splitext(fileName)[0] + ".png")
script.close()
