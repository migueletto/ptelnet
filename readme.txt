ptelnet 0.7
-----------

Released: 2020-09-13

ptelnet is a Telnet Client / Terminal Emulator for the Palm Computing platform.

ptelnet is distributed as freeware but is not in the public domain.
ptelnet is Copyright (c) 1997-2020 migueletto.

ptelnet can not be reverse engineered or modified in any way (including
this readme). To include ptelnet in any software collection, online or not,
first contact the author for permission. No one is allowed to sell or charge
for redistributing ptelnet.

This software comes with absolutely NO warranty, so use it at your own risk.

Send comments or suggestions to this email address: migueletto@yahoo.com
The ptelnet home-page is located at: http://migueletto.github.io/ptelnet/ptelnet.html

Regards,
migueletto

--

1. Introduction
 1.1. Requirements
 1.2. Features
 1.3. What is new
 1.4. Known bugs
 1.5. Upgrading from previous versions

2. Operation
 2.1. Connecting
 2.2. Sending text
 2.3. Terminal characteristics

3. Menus
 3.1. The "Options" menu
 3.2. The "Edit" menu

4. History log

--

1. Introduction

1.1. Requirements:

 - PalmOS 3.5 or greater.

1.2. Features:

 - RFC 854 Telnet protocol (telnet mode).
 - Experimental (incomplete) TN3270 terminal emulation (TN3270 mode).
 - RS-232 / IRComm connections up to 57600 bps (serial mode).
 - VT100 terminal emulation, including PF keys.
 - 12 configurable string macros (including ESC and control characters).
 - Paste MemoPad record.
 - Log input in MemoPad records (character or hexadecimal).

1.3. What is new:

 - Support for ANSI colors on color displays.
 - Support for high density displays on PalmOS 5.
 - Different fonts depending on screen density.
 - Experimental (incomplete) TN3270 mode.
 - Support for AlphaSmart Dana wide display.
 - Limited UTF-8 support.
 - Removed paste from Doc option.
 - Removed addioional charsets option.

1.4. Known bugs:

 - If you find any please tell me.

1.5. Upgrading from previous versions:

 - Delete previous version and install version 0.7.

2. Operation:

2.1. Connecting:

 - In Terminal Options, choose operation mode: serial, telnet or tn3270.
   In telnet and tn3270 modes, ptelnet uses the built-in TCP/IP stack.
   In serial mode, it uses one of the serial ports directly.

 - If using telnet or tn3270 modes, enter a host and a port in Network Options and tap
   the "On" button on the lower left corner.
   Before connecting to the host, ptelnet will start the TCP/IP stack
   by activating the service defined in PalmOS Prefs/Network (if it is
   not already started).
   If the connection is successful, the "On" button will turn black,
   otherwise it will remain blank and an error message will be displayed.
   Possible reasons for failing are: DNS lookup failure, host is unreachable,
   timeout, etc. You can configure the timeout (in seconds) for DNS lookup and
   connection.

 - If using serial mode, first choose the connection port.
   Then configure the baud, parity, word size, stop bits and
   flow control in Serial Options and tap the "On" button on the lower left
   corner. ptelnet will then open the serial port.
   If the connection is successful, the "On" button will turn black,
   otherwise it will remain blank and an error message will be displayed.

 - To disconnect from the host, either logout or tap the "On" button again.
   In telnet and tn3270 modes, note that the TCP/IP stack will NOT be shut down.
   To do this, go to the PalmOS Prefs/Network and disconnect. In serial mode,
   after logout you have to tap the "On" button to really disconnect.

 - Once connected, all other features are exactly the same, regardless of
   the mode chosen.

2.2. Sending text:

 - The cursor position is marked by a fixed underscore (if disconnected) or a
   blinking underscore (if connected) in the screen.

 - To send characters, simply write them in the Graffiti area.
   You can also tap the "Kbd" button, which will bring up the built-in keyboard.
   After tapping "Done", the characters you have typed will be sent (max. 64). 
   Note that there is no editable text field on the screen, so every time
   you call the keyboard it will NOT edit the current line, and will start
   empty. Naturally, you can also use Graffiti shortcuts to send text.
   Shift and Caps lock are the same as in Graffiti.

 - See the "Edit Menu" section below on how to send the contents of a MemoPad
   record.

 - To send ASCII control characters, tap the "Ctl" button and write one of the
   characters in the table below. Tapping "Ctl" without writing a character
   will deactivate the "control state".

   Character ASCII code sent (decimal)
   --------- -------------------------
   @  00
   A..Z  01..26
   a..z  01..26
   [  27
   \  28
   ]  29
   ^  30
   _  31

 - To send an ESC, tap the "Esc" button (or tap "Ctl" and write "[").

 - To send one of the 12 configurable macros, select the "Macros" listbox
   and choose the macro name. The macro definition will be sent.

 - To send special keys, tap the "Pad" button to bring either VT100 or TN3270 keypad.
   Tap one of the key buttons to send the corresponding function key.

 - These hardware buttons generate the following VT100 key codes:
   PageUp: up-arrow
   PageDown: down-arrow
   Date Book: space
   Address Book: left-arrow
   Todo List: right-arrow
   Memo Pad: return key
   If your terminal is set properly, programs like vi, pine, lynx, etc
   recognize these key codes.

 - Special control sequences:
   There are special control sequences that you can type while using the
   built-in keyboard or put inside a macro definition or a MemoPad record.
   If you insert the sequence "\r" (without quotes), ptelnet will send the
   "return key" you chose in Terminal options.
   The sequence "\\" sends a single "\".
   The sequence "\^" sends a "^".
   The sequence "\xnn", where n is a hex digit (0-9, A-F, or a-f), sends the
   equivalent byte value (0x00 - 0xFF).
   If you insert the sequence "^x" (without quotes) where x is any character in
   the ASCII control characters table shown above, ptelnet will send the
   corresponding code.
   There one special control character "^," which actually does not
   send any character, but pauses the output for 1 second.

2.3. Terminal characteristics

 - If you are connecting to a Unix host and are using the serial mode, check
   that your terminal is set to VT100 (or TERM=vt100)
   and that the screen is the correct size (number of rows and columns).
   If you are using the telnet mode, the terminal type and the window size 
   are automatically configured.

3. Menus:

3.1. The "Options" menu:

 - Network (applicable to telnet and tn3270 modes):
   Enter a host name or IP address and a port number (decimal).
   Configure the timeout (in seconds) for DNS lookup and connection.
   Every time you tap the "On" button, ptelnet will try to
   connect to this port on this host.

 - Serial (applicable to serial mode):
   Configure the port, baud, parity, word size, stop bits and flow control.

 - Terminal:
   Choose the operation mode: serial, telnet or tn3270.
   Choose what kind of "return key" you are using: when you write a
   Graffiti return, ptelnet will send a CR, a LF or both.
   Choose the font by tapping tha appropriate button.
   If you change the current font, the screen will be erased.
   Checking "Local echo" will make ptelnet echo locally every
   character you send.

 - Macros:
   In the left column, you choose up to 12 macro names (max. 8 chars each).
   You can see only 6 macros at a time. To see the other 6, tap on the Up/Down
   arrows on the bottom right. The names defined here are shown in the "Macros" 
   listbox in the bottom of the main screen.
   In the right column, you choose their definitions (max. 20 chars each).
   Common uses for macros are: login names, passwords, terminal
   configuration commands, logout command, etc.
 
 - Log:
   This option controls if input is logged or not, and the format of the log.
   The input is stored in standard MemoPad records, which can be viewed with
   the MemoPad application. Due to the 4K MemoPad record limit, the input is
   broken in 4K chunks, and new records are created as needed. All records are
   created in the Unfiled category.
   If the "Active" option is checked, input is logged. To disable logging,
   uncheck this option.
   If the "Log header" option is checked, each record is identified with the
   string "ptelnet log - n" in its first line, where n is the sequential
   record number of the current logging session. This means that if you turn
   logging off, the next time you turn it on the numbering will start at 1
   again. If "Log header" is not checked, there is no easy way to "glue"
   records in the correct order, other than visual inspection.
   "Hex dump" specifies how characters are logged. If not checked, all
   characters are directly stored, without any translation. Note that not
   every character can be displayed in the MemoPad application (like the
   CR - carriage return - which is displayed as a small square).
   If "Hex dump" is checked, for each character the corresponding 2 digit
   hexadecimal value is stored instead, 8 values in each line. This is useful
   for recording binary data for debugging.

 - About: About box.


3.2. The "Edit" menu:

 - Paste Memo:
   A window will show up listing all your MemoPad records, six at a time.
   To see the others tap the small arrows on the right side of the listbox.
   Only the first line of each record is shown. Choose one record by tapping
   its name, and then tap the Ok button. ptelnet will send the content of
   the selected record as if you were typing it. While it is sending the record
   it is not possible to send any other character directly using Graffiti,
   Kdb, Macro, etc. Control sequences inside a memo are interpreted the same
   way as in normal text entry.


4. History log:

 Version 0.1 (1997-10-13), initial release:
   - RFC 854 Telnet protocol.
   - VT100 terminal emulation (almost complete).
   - 5x10 fixed character font.
   - 32 columns x 15 lines screen.
   - Configurable string macros.

 Version 0.2 (1997-10-19):
   - Configurable screen width: 32 or 64 columns.
   - Reverse video (VT100).
   - Hardware buttons generate VT100 up-arrow, down-arrow, right-arrow and
     left-arrow key codes.
   - Error messages.
   - Corrected a bug that caused a connection to fail sometimes.
   - Corrected a bug that caused a fatal exception in the Macros form.

 Version 0.21 (1997-10-19):
   - Corrected a bug that caused a fatal exception everytime a connection
     failed.

 Version 0.22 (1997-10-25):
   - Added a standard Edit menu (Copy, Paste, Keyboard, Graffiti, etc) to
     these forms: Macros, Network.
   - Added Graffiti State Indicator to these forms: Main, Macros, Network.
   - Better VT100 support (now works with elm).

 Version 0.3 (1997-11-03):
   - ISO 8859-1 character set.
   - Minor changes in the character font.
   - Ability to send characters using the built-in keyboard.
   - Faster startup.
   - Fixed a bug in VT100 emulation.
   - Fixed a bug that made the Graffiti State Indicator disappear sometimes.

 Version 0.31 (1997-11-11):
   - Implemented VT100 Scrolling Regions: now PamlTelnet works correctly with
     ircII client on UNIX !

 Version 0.4 (1998-02-07):
   - Dual mode operation: telnet or serial.
   - Added a new form: Serial Options.
   - Changed the current font from 5x10 to 5x9, giving 16 lines.
   - Added a new font (4x6), giving two new screen sizes: 24x40 and 24x80.
   - Configurable timeouts for telnet mode.
   - Now uses all 6 hardware buttons.
   - Blinking cursor.
   - Fixed a few bugs in the VT100 emulation.

 Version 0.41 (1998-03-04):
   - Three views of wide screens: left, middle and right.
   - It is possible to send all 32 lower ASCII codes (0-31) using "Ctl" button.
   - It is possible to insert control characters in macros.
   - Fixed a few bugs in the VT100 emulation.
   - Fixed the "Prompt" bug in the Service connection dialog. In previous
     versions, if you left the Password unassigned or had a Prompt field in the
     Service script, ptelnet would crash with a "Fatal Exception".
   - Fixed the "Alarm Popup" bug. In previous versions, if an alarm went off
     while ptelnet was running it would crash with a "Fatal Exception".

 Version 0.42 (1998-11-18):
   - Minor changes in the user interface.
   - Removed the "not tested" warning, although it remains untested under
     PalmOS 3.
   - Added 6 more macros (there are now 12 macros).
   - Added a VT100 Keypad, from which it is possible to send PF keys.
   - New Home-Page location and email contact address.

 Version 0.43 (1998-11-24):
   - BUGFIX: ptelnet crashed sometimes when using the serial mode with
     the PalmPilot Modem.

 Version 0.5 (1998-12-13):
   - Paste MemoPad record function.
   - The 4x6 font now has the complete ISO 8859-1 character set.
   - Ability to send a break signal in serial mode.
   - Ability to Send the Del key.
   - A special control character "^," which pauses the
     output for 1 second.

 Version 0.51 (1999-01-23):
   - Added the PalmOS 3 small icon.
   - You can log the input in MemoPad records, saving the incoming characters
     or their hexadecimal values.

 Version 0.52 (1999-02-01):
   - Ability to send any byte value (0x00 - 0xFF).
   - Fixed a bug that caused a fatal exception in a multi record hex dump.

 Version 0.53 (1999-10-22):
   - PalmTelnet is now known as ptelnet. As a result, the Home-Page location
     also changed to reflect the new name:
     http://netpage.em.com.br/mmand/ptelnet.htm
   - Support for multiple character sets. Besides the built-in ISO 8859-1
     (Latin1), you can download and install additional character sets.
     Check the Home-Page for available sets.
   - A Charset Map dialog, from which the user can see the upper half
     of the current character set (0x80 - 0xFF) and send any of them.

 Version 0.6 (2000-07-30):
   - New support for serial connections using the IR port (IRComm).
   - Ability to paste text from DOC files.
   - The VT100 keypad now has buttons for PF1 to PF20, and also for arrow keys.
   - Fixed a bug with displaying macro names.
   - Fixed a bug in display of Graffiti State Indicator on PalmOS 3.5.

--

End.
