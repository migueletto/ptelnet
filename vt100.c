#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "vt100.h"
#include "screen.h"
#include "error.h"

#define maxVT100Arg 8

static Short vt100_s, vt100_num, vt100_arg[maxVT100Arg+1];
static Short rows, cols;
static Short keyMode;
static Boolean vt100_firstdigit;
static Boolean lastcol;

/*
* ESC  =
* ESC  >
*/

//#define DEBUG 1

#ifdef DEBUG
static Boolean colch;
static Boolean interr;

static void DebugVt100(Boolean colch, Boolean interr, uint8_t c, int unknown) {
  char arg[3][8];
  int flag = 0;

  arg[0][0] = arg[1][0] = arg[2][0] = 0;
  if (vt100_num > 0) { sprintf(arg[0], "%d", vt100_arg[0]); flag = 1; }
  if (vt100_num > 1) { sprintf(arg[1], "%s%d", flag ? ";" : "", vt100_arg[1]); flag = 1; }
  if (vt100_num > 2) { sprintf(arg[2], "%s%d", flag ? ";" : "", vt100_arg[2]); flag = 1; }

  debug(DEBUG_INFO, "PALMOS", "VT100 ESC %s%s%s%s%s %c%s", colch ? "[" : "", interr ? "?" : "", arg[0], arg[1], arg[2], c, unknown ? " (unknown)" : "");
}
#define DEBUGVT100(c,u) DebugVt100(colch, interr, c, u)
#else
#define DEBUGVT100(ci,u)
#endif

void InitVT100(Short r, Short c)
{
  vt100_s = 0;
  lastcol = false;
  rows = r;
  cols = c;
  keyMode = 0;	// cursor mode
}

void EmitVT100(Char *buf, Int n, Short done)
{
  Int k, i, m;
  Char c;

  Cursor(0);

  for (k = 0; k < n; k++) {
    c = buf[k];
    switch (vt100_s) {
      case 0:
        if (c == ESC) {
          vt100_num = 0;
          vt100_firstdigit = true;
          vt100_arg[0] = 0;
          vt100_s = 1;
#ifdef DEBUG
          colch = false;
          interr = false;
#endif
        }
        else
          EmitCharVT100(c);
        break;

      case 1:
        vt100_s = 0;
        switch (c) {
          case '[':
            vt100_s = 2;
#ifdef DEBUG
            colch = true;
#endif
            break;
          case 'D':		/* Scroll text up */
            DEBUGVT100(c,0);
            ScrollUp();
            break;
          case 'E':		/* Newline (behaves like cr followed by do) */
            DEBUGVT100(c,0);
            EmitCharVT100('\r');
            EmitCharVT100('\n');
            break;
          case 'M':		/* Scroll text down */
            DEBUGVT100(c,0);
            ScrollDown();
            break;
          case '7':		/* Save cursor position & attributes */
            DEBUGVT100(c,0);
            SaveCursor();
            break;
          case '8':		/* Restore cursor position & attributes */
            DEBUGVT100(c,0);
            RestoreCursor();
            break;
          case '=':
          case '>':
            DEBUGVT100(c,0);
            break;
          case '(':
          case ')':		/* Start/End alternate character set */
          case '#':
            DEBUGVT100(c,0);
            vt100_s = 3;
            break;
          default:
            DEBUGVT100(c,1);
            break;
        }
        break;

      case 2:
        vt100_s = 0;
        switch (c) {
          case 'A':		/* Upline (cursor up) */
            DEBUGVT100(c,0);
            m = vt100_num ? vt100_arg[0] : 1;   
            for (i = 0; i < m; i++)
              DecY();
            break;
          case 'B':		/* Down one line */
            DEBUGVT100(c,0);
            m = vt100_num ? vt100_arg[0] : 1;   
            for (i = 0; i < m; i++)
              EmitCharVT100('\n');
            break;
          case 'C':		/* Non-destructive space (cursor right) */
            DEBUGVT100(c,0);
            m = vt100_num ? vt100_arg[0] : 1;   
            for (i = 0; i < m; i++) {
              if (GetX() < cols-1) {
                IncX(1);
              }
            }
            break;
          case 'D':		/* Move cursor left n positions */
            DEBUGVT100(c,0);
            m = vt100_num ? vt100_arg[0] : 1;   
            for (i = 0; i < m; i++)
              if (GetX())
                DecX();
            lastcol = false;
            break;
          case 'H':
          case 'f':
            DEBUGVT100(c,0);
            if (vt100_num == 2) {	/* Screen-relative cursor motion */
              SetY(vt100_arg[0]-1);
              SetX(vt100_arg[1]-1);
            } else if (vt100_num == 0) {	/* Home cursor */
              Home();
            }
            lastcol = false;
            break;
          case 'J':		/* Clear display */
            DEBUGVT100(c,0);
            if (!vt100_num) {
              vt100_arg[0] = 0;
            }
            switch (vt100_arg[0]) {
              case 0: ClearScreenToEnd(); break;
              case 1: ClearScreenFromBegin(); break;
              case 2: ClearScreen(); break;
            }
            lastcol = false;
            break;
          case 'K':		/* Clear line */
            DEBUGVT100(c,0);
            if (!vt100_num) {
              vt100_arg[0] = 0;
            }
            switch (vt100_arg[0]) {
              case 0: ClearLineToEnd(); break;
              case 1: ClearLineFromBegin(); break;
              case 2: ClearLine(); break;
            }
            lastcol = false;
            break;
          case 'h':
            DEBUGVT100(c,0);
            if (vt100_num == 1) switch (vt100_arg[0]) {
              case 1:
                keyMode = 1;	// application
                break;
            }
            break;
          case 'l':
            DEBUGVT100(c,0);
            if (vt100_num == 1) switch (vt100_arg[0]) {
              case 1:
                keyMode = 0;	// cursor
                break;
            }
            break;
          case 'm':
            DEBUGVT100(c,0);
            if (vt100_num == 0) {
              /* Turn off all attributes */
              ScreenDefaultColors();
              Underscore(0);
              Reverse(0);
            } else for (i = 0; i < vt100_num; i++) {
              switch (vt100_arg[i]) {
              case 0:		/* Turn off all attributes */
                ScreenDefaultColors();
                Underscore(0);
                Reverse(0);
                break;
              case 1:		/* Turn on bold (extra bright) attribute */
                break;
              case 4:		/* Start underscore mode */
                Underscore(1);
                break;
              case 5:		/* Turn on blinking attribute */
                break;
              case 7:		/* Turn on reverse-video attribute */
                Reverse(1);
                break;
              case 30:
              case 31:
              case 32:
              case 33:
              case 34:
              case 35:
              case 36:
              case 37:
		SetForegroundColor(vt100_arg[i] - 30);
                break;
              case 40:
              case 41:
              case 42:
              case 43:
              case 44:
              case 45:
              case 46:
              case 47:
		SetBackgroundColor(vt100_arg[i] - 40);
                break;
            }
            }
            break;
          case 'r':		/* Change scrolling region (VT100) */
            DEBUGVT100(c,0);
            if (vt100_num != 2) {
              vt100_arg[0] = 1;
              vt100_arg[1] = GetRows();
            } else if (vt100_arg[1] > GetRows()) {
              vt100_arg[1] = GetRows();
            }
            ChangeScrollingRegion(vt100_arg[0]-1, vt100_arg[1]-1);
            rows = vt100_arg[1];
            break;
          case 'L':		/* Insert line(s) */
            DEBUGVT100(c,0);
            if (!vt100_num)
              vt100_arg[0] = 1;
            ChangeScrollingRegion(GetY(), GetRows()-1);
            for (i = 0; i < vt100_arg[0]; i++)
              ScrollDown();
            ChangeScrollingRegion(0, GetRows()-1);
            break;
          case 'M':		/* Delete line(s) */
            DEBUGVT100(c,0);
            if (!vt100_num)
              vt100_arg[0] = 1;
            ChangeScrollingRegion(GetY(), GetRows()-1);
            for (i = 0; i < vt100_arg[0]; i++)
              ScrollUp();
            ChangeScrollingRegion(0, GetRows()-1);
            break;
          case 'P':		/* Delete char(s) */
            DEBUGVT100(c,0);
            if (!vt100_num)
              vt100_arg[0] = 1;
            for (i = 0; i < vt100_arg[0]; i++)
              DeleteChar();
            break;
          case '?':
            vt100_s = 2;
#ifdef DEBUG
            interr = true;
#endif
            break;
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            if (vt100_firstdigit) {
              vt100_num++;
              vt100_arg[vt100_num] = 0;
              vt100_firstdigit = false;
            }
            vt100_arg[vt100_num-1] *= 10;
            vt100_arg[vt100_num-1] += c - '0';
            vt100_s = 2;
            break;
          case ';':
            vt100_firstdigit = true;
            vt100_s = 2;
            break;
          default:
            DEBUGVT100(c,1);
            break;
        }
        break;

      case 3:
        vt100_s = 0;
    }
  }

  Cursor(1);
}

void EmitCharVT100(UChar c)
{
  Short lxt = 0, lyt = 0, n;
  Boolean draw;
  UShort attr;

  draw = false;

  switch (c) {
    case '\0':
      break;
    case '\n':
      IncY();
      break;
    case '\r':
      SetX(0);
      lastcol = false;
      break;
    case 0x7:
      SndPlaySystemSound(sndInfo);
      break;
    case '\b':
      if (GetX()) {
        if (lastcol)
          lastcol = false;
        else
          DecX();
      }
      break;
    case '\t':
      n = 8 - (GetX() % 8);
      if (GetX()+n == cols)
        n--;
      IncX(n);
      break;
    default:
      if (c < 32)
        break;
      draw = true;
      lxt = GetX();
      lyt = GetY();
      if (lxt < (cols-1))
        IncX(1);
      else {
        if (lastcol) {
          lxt = 0;
          SetX(1);
          IncY();
          if (GetY() >= rows) {
            ScrollUp();
            DecY();
          }
          lyt = GetY();
          lastcol = false;
        }
        else
          lastcol = true;
      }
      if (GetX() >= cols) {
        SetX(0);
        IncY();
      }
  }

  if (draw && lyt < rows) {
    attr = 0;
    if (IsUnderscore()) attr |= m_underscore;
    if (IsReverse()) attr |= m_reverse;
    DrawChar(c, attr, lxt, lyt);
  }

  if (GetY() >= rows) {
    ScrollUp();
    DecY();
  }
}

static UChar seq[8];

Char *GetKeySeq(UChar key)
{
  UChar base;

  seq[0] = ESC;

  switch (key) {
    case VT100_upArrow:
    case VT100_downArrow:
    case VT100_leftArrow:
    case VT100_rightArrow:
      // keyMode == 0 :	cursor
      // keyMode == 1 :	application
      seq[1] = keyMode ? 'O' : '[';
      seq[2] = key;
      seq[3] = 0;
      return (Char *)seq;

    case 0x81:	// F1
    case 0x82:	// F2
    case 0x83:	// F3
    case 0x84:	// F4
      base = VT100_PF;
      key &= 0x0F;
      seq[1] = 'O';
      seq[2] = key + base;
      seq[3] = 0;
      return (Char *)seq;

    case 0x85:	// F5
    case 0x87:	// F6
    case 0x88:	// F7
    case 0x89:	// F8
      key &= 0x0F;
      seq[2] = '1';
      seq[3] = key + '0';
      break;

    case 0x90:	// F9
    case 0x91:	// F10
    case 0x93:	// F11
    case 0x94:	// F12
    case 0x95:	// F13
    case 0x96:	// F14
    case 0x98:	// F15
    case 0x99:	// F16
      key &= 0x0F;
      seq[2] = '2';
      seq[3] = key + '0';
      break;

    case 0xA1:	// F17
    case 0xA2:	// F18
    case 0xA3:	// F19
    case 0xA4:	// F20
      key &= 0x0F;
      seq[2] = '3';
      seq[3] = key + '0';
      break;

    default:	// numeric keypad
      base = VT100_NUM;
      key &= 0x0F;
      seq[1] = 'O';
      seq[2] = key + base;
      seq[3] = 0;
      return (Char *)seq;
  }

  seq[1] = '[';
  seq[4] = '~';
  seq[5] = 0;

  return (Char *)seq;
}
