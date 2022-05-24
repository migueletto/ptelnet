#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "const.h"
#include "ptelnet.h"
#include "screen.h"
#include "error.h"

static WinHandle wh;
static Short maxL;      // # maximo de linhas da fonte atual
static Short maxC;      // # maximo de colunas da fonte atual
static UInt16 maxCols;
static Short cursorOn;
static Short font, fontId;
static Short x, y, cx, cy, xt, yt, saved_x, saved_y, saved_xt, saved_yt;
static Short top, bottom;
static Short reverse, underscore, saved_reverse, saved_underscore;
static Boolean hasColor;
static Short foregroundColor, highForegroundColor;
static Short backgroundColor, highBackgroundColor;
static Int16 maxX, maxY, maxXX, maxYY;
static UInt32 coordSys;
static Word ROMVerMajor, ROMVerMinor;

static const RGBColorType color_table[COLOR_SIZE] = {
  {0, 0x00, 0x00, 0x00}, // black
  {0, 0xA0, 0x20, 0x00}, // red
  {0, 0x00, 0x80, 0x00}, // green
  {0, 0x80, 0x80, 0x00}, // yellow
  {0, 0x00, 0x60, 0xA0}, // blue
  {0, 0x80, 0x00, 0x80}, // magenta
  {0, 0x80, 0x80, 0x00}, // cyan
  {0, 0x80, 0x80, 0x80}  // white
};

static const RGBColorType high_color_table[COLOR_SIZE] = {
  {0, 0x00, 0x00, 0x00}, // black
  {0, 0xA0, 0x20, 0x00}, // red
  {0, 0x00, 0xFF, 0x00}, // green
  {0, 0xFF, 0xFF, 0x00}, // yellow
  {0, 0x00, 0x60, 0xA0}, // blue
  {0, 0xFF, 0x00, 0xFF}, // magenta
  {0, 0xFF, 0xFF, 0x00}, // cyan
  {0, 0xFF, 0xFF, 0xFF}  // white
};

void InitFonts(void) {
  DWord dwVersion;
  MemHandle h;

  if ((h = DmGet1Resource(fontRscType, font4x6Id)) != NULL) {
    FntDefineFont(font4x6, MemHandleLock(h));
  }
  if ((h = DmGet1Resource(fontRscType, font5x9Id)) != NULL) {
    FntDefineFont(font5x9, MemHandleLock(h));
  }
  if ((h = DmGet1Resource(fontRscType, font6x6Id)) != NULL) {
    FntDefineFont(font6x6, MemHandleLock(h));
  }

  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &dwVersion);
  ROMVerMajor = sysGetROMVerMajor(dwVersion);

  if (ROMVerMajor >= 5) {
    if ((h = DmGet1Resource(fontExtRscType, hfont6x6Id)) != NULL) {
      FntDefineFont(hfont6x6, MemHandleLock(h));
    }
    if ((h = DmGet1Resource(fontExtRscType, hfont6x10Id)) != NULL) {
      FntDefineFont(hfont6x10, MemHandleLock(h));
    }
    if ((h = DmGet1Resource(fontExtRscType, hfont8x14Id)) != NULL) {
      FntDefineFont(hfont8x14, MemHandleLock(h));
    }
  }
}

Err InitScreen(UInt16 f, UInt16 density, UInt16 cols) {
  DWord dwVersion;
  //UInt32 depth;

  maxCols = cols;
  SetFont(f, density);

  top = 0;
  bottom = maxY;
  saved_x = saved_y = saved_xt = saved_yt = 0;
  saved_reverse = 0;
  saved_underscore = 0;

  Underscore(0);
  Reverse(0);
  SetX(0);
  SetY(0);

  wh = WinGetActiveWindow();
  WinSetDrawWindow(wh);

  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &dwVersion);
  ROMVerMajor = sysGetROMVerMajor(dwVersion);
  ROMVerMinor = sysGetROMVerMinor(dwVersion);

/*
  WinScreenMode(winScreenModeGet, NULL, NULL, &depth, NULL);
  if (depth == 1) {
    WinScreenMode(winScreenModeGetSupportedDepths, NULL, NULL, &depth, NULL);
    if (depth & 0x80)
      depth = 8;
    else if (depth & 0x08)
      depth = 4;
    else if (depth & 0x02)
      depth = 2;
    else
      depth = 1;
    WinScreenMode(winScreenModeSet, NULL, NULL, &depth, NULL);
  }
*/

  hasColor = ROMVerMajor > 3 || (ROMVerMajor == 3 && ROMVerMinor >= 5);
  if (hasColor) {
    WinScreenMode(winScreenModeGetSupportsColor, NULL, NULL, NULL, &hasColor);
  }
  ScreenDefaultColors();

  ClearScreen();
  Cursor(1);

  return 0;
}

void CloseScreen(void) {
}

void ScreenDefaultColors(void) {
  SetForegroundColor(COLOR_WHITE);
  SetBackgroundColor(COLOR_BLACK);
}

Short GetCols(void) {
  return maxC;
}

Short GetRows(void) {
  return maxL;
}

void SetFont(UInt16 f, UInt16 density) {
  DWord dwVersion;
  UInt32 swidth, sheight;
  Int16 factor;
  FontID old;

  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &dwVersion);
  ROMVerMajor = sysGetROMVerMajor(dwVersion);
  font = f;

  if (ROMVerMajor >= 5) {
    switch (font) {
      case 0: fontId = hfont6x6;  break;
      case 1: fontId = hfont6x10; break;
      case 2: fontId = hfont8x14; break;
    }
  } else {
    switch (font) {
      case 0: fontId = font4x6;   break;
      case 1: fontId = font5x9;   break;
      case 2: fontId = font6x6;   break;
    }
  }

  switch (density) {
    case kDensityLow      : coordSys = kCoordinatesStandard;  factor = 1; break;
    case kDensityDouble   : coordSys = kCoordinatesDouble;    factor = 2; break;
    default               : coordSys = kCoordinatesStandard;  factor = 1; break;
  }

  old = FntSetFont(fontId);
  cx = FntCharWidth('A') * factor;
  cy = FntCharHeight() * factor;
  FntSetFont(old);

  if (ROMVerMajor >= 5) WinSetCoordinateSystem(coordSys);
  WinScreenMode(winScreenModeGet, &swidth, &sheight, NULL, NULL);
  maxC = swidth / cx;
  if (maxCols > 0 && maxC > maxCols) maxC = maxCols;
  maxL = (sheight - 14*factor) / cy;
  maxXX = swidth - 1;
  maxYY = sheight - 14*factor - 1;
  if (ROMVerMajor >= 5) WinSetCoordinateSystem(kCoordinatesStandard);

  maxX = maxC * cx;
  maxY = maxL * cy;
}

Short GetX(void) {
  return xt;
}

Short GetY(void) {
  return yt;
}

void IncX(Short d) {
  x += d*cx;
  xt += d;
}

void DecX(void) {
  x -= cx;
  xt--;
}

void IncY(void) {         
  y += cy;
  yt++;   
}           

void DecY(void) {
  if (yt) {
    y -= cy;
    yt--;
  }
}

void RestoreCursor(void) {
  x = saved_x;
  xt = saved_xt;
  y = saved_y;
  yt = saved_yt;
  reverse = saved_reverse;
  underscore = saved_underscore;
}

void SaveCursor(void) {
  saved_x = x;
  saved_xt = xt;
  saved_y = y;
  saved_yt = yt;
  saved_reverse = reverse;
  saved_underscore = underscore;
}

void SetLinear(Short l) {
  SetY(l / maxC);
  SetX(l % maxC);
}

Short GetLinear(void) {
  return GetY()*maxC+GetX();
}

void SetX(Short x1) {
  if (x1 >= maxC)
    x1 = maxC-1;
  xt = x1;
  x = xt*cx;
}

void SetY(Short y1) {
  if (y1 >= maxL)
    y1 = maxL-1;
  yt = y1;
  y = yt*cy;
}

void Home(void) {
  Cursor(0);
  SetX(0);
  SetY(0);
  Cursor(1);
}

void Reverse(Short u) {
  reverse = u;
}

Short IsReverse(void) {
  return reverse;
}

void Underscore(Short u) {
  underscore = u;
}

Short IsUnderscore(void) {
  return underscore;
}

static void MyWinEraseRectangle(RectangleType *rect) {
  IndexedColorType oldb;

  if (ROMVerMajor >= 5) WinSetCoordinateSystem(coordSys);
  if (hasColor) {
    oldb = WinSetBackColor(backgroundColor);
    WinEraseRectangle(rect, 0);
    WinSetBackColor(oldb);
  } else {
    WinEraseRectangle(rect, 0);
  }
  if (ROMVerMajor >= 5) WinSetCoordinateSystem(kCoordinatesStandard);
}

void ClearScreen(void) {
  RectangleType rect;
          
  RctSetRectangle(&rect, 0, 0, maxXX+1, maxYY+1);
  WinSetDrawWindow(wh);
  MyWinEraseRectangle(&rect);
}

void ClearScreenToEnd(void) {
  RectangleType rect;

  if (x == 0 && y == 0) {
    ClearScreen();
  } else {
    ClearLineToEnd();
    if ((y+cy) < maxY) {
      RctSetRectangle(&rect, 0, y+cy, maxX+1, maxY-(y+cy)+1);
      MyWinEraseRectangle(&rect);
    }
  }
}

void ClearScreenFromBegin(void) {
  RectangleType rect;

  if (x != 0 || y != 0) {
    if (y >= cy) {
      RctSetRectangle(&rect, 0, 0, maxX+1, y-1);
      MyWinEraseRectangle(&rect);
    }
    ClearLineFromBegin();
  }
}

void ClearLine(void) {
  Short x1;

  x1 = GetX();
  SetX(0);
  ClearLineToEnd();
  SetX(x1);
}

void ClearLineToEnd(void) {
  RectangleType rect;

  WinSetDrawWindow(wh);
  RctSetRectangle(&rect, x, y, maxX+1, cy);
  MyWinEraseRectangle(&rect);
}

void ClearLineFromBegin(void) {   
  RectangleType rect;
   
  WinSetDrawWindow(wh);
  RctSetRectangle(&rect, 0, y, x, cy);
  MyWinEraseRectangle(&rect);
}

void DeleteChar(void) {
  RectangleType rect;
  Short x0, xi, xf, i;
   
  for (i = GetX(), x0 = x; i < maxC; i++, x0 += cx) {
    xi = x0+cx;
    xf = x0;
    if (ROMVerMajor >= 5) WinSetCoordinateSystem(coordSys);
    RctSetRectangle(&rect, xi, y, cx, cy);
    WinCopyRectangle(wh, wh, &rect, xf, y, scrCopy);
    if (ROMVerMajor >= 5) WinSetCoordinateSystem(kCoordinatesStandard);
  }
  xi = (maxC-1)*cx;
  RctSetRectangle(&rect, xi, y, cx, cy);
  WinSetDrawWindow(wh);
  MyWinEraseRectangle(&rect);
}

void ChangeScrollingRegion(Int l0, Int l1) {
  if (l0 < 0 || l0 >= maxL || l1 < 0 || l1 >= maxL || l1 < l0) {
    return;
  }

  top = l0*cy;
  bottom = (l1+1)*cy - 1;
}

void ScrollUp(void) {
  RectangleType rect;

  if (ROMVerMajor >= 5) WinSetCoordinateSystem(coordSys);
  RctSetRectangle(&rect, 0, top+cy, maxX+1, bottom-top-cy+1);
  WinCopyRectangle(wh, wh, &rect, 0, top, scrCopy);
  if (ROMVerMajor >= 5) WinSetCoordinateSystem(kCoordinatesStandard);

  RctSetRectangle(&rect, 0, bottom-cy+1, maxX+1, cy);
  MyWinEraseRectangle(&rect);
}

void ScrollDown(void) { 
  RectangleType rect;
            
  if (ROMVerMajor >= 5) WinSetCoordinateSystem(coordSys);
  RctSetRectangle(&rect, 0, top, maxX+1, bottom-top-cy+1);
  WinCopyRectangle(wh, wh, &rect, 0, top+cy, scrCopy);
  if (ROMVerMajor >= 5) WinSetCoordinateSystem(kCoordinatesStandard);

  RctSetRectangle(&rect, 0, top, maxX+1, cy);
  MyWinEraseRectangle(&rect);
}

void DrawChar(Short c, UShort attr, Short x0, Short y0) {
  IndexedColorType fg, bg, tmp, oldf = 0, oldb = 0, oldt = 0;
  RectangleType r;
  FontID old;
  Short x, y;

  if (c < 32 || c > 255) c = 32;

  x = x0*cx;
  y = y0*cy;
  old = FntSetFont(fontId);
  WinSetDrawWindow(wh);

  if (hasColor) {
    if (attr & m_highlight) {
      fg = highForegroundColor;
      bg = highBackgroundColor;
    } else {
      fg = foregroundColor;
      bg = backgroundColor;
    }

    if (attr & m_reverse) {
      tmp = fg;
      fg = bg;
      bg = tmp;
    }
    oldt = WinSetTextColor(fg);
    oldf = WinSetForeColor(fg);
    oldb = WinSetBackColor(bg);
  }

  if (ROMVerMajor >= 5) WinSetCoordinateSystem(coordSys);
  r.topLeft.x = x;
  r.topLeft.y = y;
  r.extent.x = cx;
  r.extent.y = cy;
  WinEraseRectangle(&r, 0);
  WinDrawChar(c, x, y);
  if (attr & m_underscore) {
    WinDrawLine(x, y+cy-1, x+cx-1, y+cy-1);
  }
  if (ROMVerMajor >= 5) WinSetCoordinateSystem(kCoordinatesStandard);

  if (hasColor) {
    WinSetForeColor(oldf);
    WinSetTextColor(oldt);
    WinSetBackColor(oldb);
  }
  FntSetFont(old);
}

void Cursor(Int on) {
  IndexedColorType oldf;

  if (on == -1) {
    on = 1 - cursorOn;
  }

  if (y+cy-1 <= maxY) {
    if (ROMVerMajor >= 5) WinSetCoordinateSystem(coordSys);

    if (hasColor) {
      oldf = WinSetForeColor(on ? foregroundColor : backgroundColor);
      WinDrawLine(x, y+cy-1, x+cx-1, y+cy-1);
      WinSetForeColor(oldf);
    } else {
      if (on) {
        WinDrawLine(x, y+cy-1, x+cx-1, y+cy-1);
      } else {
        WinEraseLine(x, y+cy-1, x+cx-1, y+cy-1);
      }
    }

    if (ROMVerMajor >= 5) WinSetCoordinateSystem(kCoordinatesStandard);
  }

  cursorOn = on;
}

void DrawSeparator(void) {
  UInt32 swidth, sheight, y;
  WinScreenMode(winScreenModeGet, &swidth, &sheight, NULL, NULL);
  y = sheight - (160 - bottomLine);
  WinDrawLine(0, y, swidth, y);
}

void SetForegroundColor(UShort color) {
  if (hasColor && color < COLOR_SIZE) {
    foregroundColor = WinRGBToIndex(&color_table[color]);
    highForegroundColor = WinRGBToIndex(&high_color_table[color]);
  }
}

void SetBackgroundColor(UShort color) {
  if (hasColor && color < COLOR_SIZE) {
    backgroundColor = WinRGBToIndex(&color_table[color]);
    highBackgroundColor = WinRGBToIndex(&high_color_table[color]);
  }
}

Int16 FontWidth(void) {
  return cx;
}

Int16 FontHeight(void) {
  return cy;
}
