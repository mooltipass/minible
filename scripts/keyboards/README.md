# Mooltipass Keyboard Support

This is a work in progress document describing the keyboard support of the Mooltipass BLE project.

### Outline

1. Introduction
	1. Intended support
2. Layout Data
3. Possible Data structure




#### Introduction

In order to support passwords which contain characters outside the ASCII range (as the current mooltipass mini) we would like to support unicode BMP (plane 0) characters.

The mp has an HID interface which uses it's own protocol and can easily support sending binary data both ways.  This can be used by plugins and stand alone applications for setting and retrieving passwords as unicdoe strings.

The challenge here lies in the HID keyboard interface which could be used when a plugin or application is not available.  In this mode, the mp must use the HID keyboard key codes to send key strokes.   In order to know which characters are output, the mp needs to know what keyboard layout and platform (windows, osx, etc) is being used.

So if a unicode string is stored on the mp, and we want to 'type' that we need a sequence such as:

1. take each character of a password
2. find that character in the currently selected keyboard layout
	1. if it doesn't exist in the layout, we can no type it
3. find the HID key code
4. send the key code to the host

##### Intended Support

To support common keyboard layouts, we need to create mappings from unicode points to HID key codes.   We intend to support 'keys' that can be typed with the standard HID keyboard modifier bits (control, shift, alt, gui) plus 'normal' keys.  We would NOT support character that require caps lock, or sequences of keys, or long press plus another key.

#### Layout Data

The Unicode CLDR project provides keyboard layout data for different layouts/locale.   We can leverage this data to create mappings, for each layout, of a unicdoe point to a HID keyboard key code.

for example

```
Glyph         Unicode     modifier+keycode Description
∂                8706              0x0 0x2 PARTIAL DIFFERENTIAL
∆                8710              0x0 0x2 INCREMENT
∑                8721              0x0 0x1 N-ARY SUMMATION
–                8211             0x0 0x2c EN DASH
‘                8216              0x0 0x4 LEFT SINGLE QUOTATION MARK
’                8217             0x0 0x26 RIGHT SINGLE QUOTATION MARK
‚                8218             0x0 0x2d SINGLE LOW-9 QUOTATION MARK
                   32             0x0 0x31 SPACE
#                  35             0x0 0x14 NUMBER SIGN
$                  36             0x0 0x15 DOLLAR SIGN
&                  38             0x0 0x1a AMPERSAND
'                  39             0x0 0x27 APOSTROPHE
*                  42             0x0 0x1c ASTERISK
+                  43             0x0 0x12 PLUS SIGN
,                  44             0x0 0x2b COMMA
-                  45             0x0 0x2c HYPHEN-MINUS
-                  
```

This data varies widely for the various layouts.


### Possible Data structure

(work in progress)

To me, after looking at how varied the layouts are.  The only choice is to have a map for each layout and only load a single layout into memory at a time.

The data structure would be a map

```
[16 bit unicode point]  =>   [modifier byte][keycode byte]
```

So, with the current CLDR version 32.  The stats show as:

```
Showing stats for 322 layouts.
Maximum number of characters in a layout: 180
Mean number of characters in a layout: 96.8
Median number of characters in a layout: 95.0
Now... assuming 2 byte for unicode, 2 bytes for hid (modifier+keycode):
All maps add up to 124724 Bytes
Average map size is 387 Bytes
The number of unique unicode characters used is 1603
```