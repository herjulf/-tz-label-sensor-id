#!/bin/bash

# Use Brother P-touch 2430

TMP=/tmp/text2pbm.$$
TMP2=/tmp/text.$$
TEXT=$*
PRINTER=/dev/usb/lp1
LABEL=./pt1230.bin.static
TTY_TALK=./tty_talk.bin.static

function s1 ()
{

#FONT="Times-Roman"
#FONT="Helvetica"
FONT="Courier-bold"
    FS=9
    gs -r180 -sDEVICE=pbm -sOutputFile=$TMP -g64x240 <<xxEOFxx
%!PS
/$FONT findfont $FS scalefont setfont
17 5 moveto
90 rotate
($TEXT) show
showpage
xxEOFxx
#eog $TMP
}    

stty -F /dev/ttyUSB0 sane

$TTY_TALK -38400 /dev/ttyUSB0 eui64 | grep -v eui64  | grep -v Lock | grep -v -e '^$' > $TMP2
$TTY_TALK -38400 /dev/ttyUSB0 eui64 | grep -v eui64  | grep -v Lock | grep -v -e '^$' > $TMP2

read TEXT < $TMP2

#TEXT="C MAC=$TEXT"
#TEXT="MAC=$TEXT"

echo $TEXT

cat  $TMP2 >> ID

s1
$LABEL $TMP > $PRINTER
#cat $TMP | display -flip

#rm $TMP
#rm $TMP2
