#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "sec.h"
#include "const.h"
#include "ptelnet.h"
#include "tn3270.h"
#include "screen.h"
#include "telnet.h"

#define BufferSize 1920

static Int16 tn3270_s, isfcmd;
static UInt8 CMD, WCC, FA, ORD, AID, SFCMD[4], ba_mode, sa_type, sa_value, sa_highlight, sa_fg, sa_bg, npairs;
static Int16 BA, cursorBA, tmpBA, length;
static Int16 rows, cols, tnDone;
static UInt8 codBA[2];

static DmOpenRef dbRef;

static MemHandle BufferH;
static UInt8 *Buffer, *Attr;
static UInt16 recBuffer;

static MemHandle Ascii2EbcdicH;
static UInt8 *Ascii2Ebcdic;

static MemHandle Ebcdic2AsciiH;
static UInt8 *Ebcdic2Ascii;

static MemHandle replyHandle;
static UInt16 replySize;
static UInt8 *replyData;

static UInt16 sa2color(UInt16 value, Boolean fg) SEC("tn3270");
static void EmitChar3270(UInt8 c, UInt8 fa) SEC("tn3270");
static void CursorTN3270(Int16 s) SEC("tn3270");
static UInt8 ebcdic2ascii(UInt8) SEC("tn3270");
static UInt8 ascii2ebcdic(UInt8) SEC("tn3270");
static UInt8 FromBuffer(Int16) SEC("tn3270");
static Int16 ToBuffer(UInt8) SEC("tn3270");
static void ToAttr(Int16, UInt8 c) SEC("tn3270");
static void MakeBA_12A(UInt16, UInt8 *b) SEC("tn3270");
static Int16 FirstField(void) SEC("tn3270");
static Int16 NextField(Int16 ba) SEC("tn3270");
static Int16 CurrentField(Int16) SEC("tn3270");
static void CursorTN3270(Int16 s) SEC("tn3270");

Err InitTN3270(Int16 r, Int16 c) {
  CMD = 0;
  AID = AID_NO;
  WCC = 0;
  FA = 0;
  BA = 0;
  cursorBA = -1;
  DmSet(Buffer, 0, BufferSize*2, 0);
  tn3270_s = 1;
  rows = r;
  cols = c;
  tnDone = 1;
  sa_highlight = 0;
  sa_fg = 0;
  sa_bg = 0;

  return 0;
}

Err Init0TN3270(Int16 r, Int16 c) {
  if (!(dbRef = DmOpenDatabaseByTypeCreator(DBType, AppID, dmModeReadWrite))) {
    if (DmCreateDatabase(0, BufferDB, AppID, DBType, false))
      return -1;
    if (!(dbRef = DmOpenDatabaseByTypeCreator(DBType, AppID, dmModeReadWrite)))
      return -1;
    recBuffer = 0;
    if (!(BufferH = DmNewRecord(dbRef, &recBuffer, BufferSize*2)))
      goto error0;
  } else {
    recBuffer = 0;
    if (!(BufferH = DmGetRecord(dbRef, recBuffer)))
      goto error0;
  }
  
  if (!(Buffer = MemHandleLock(BufferH)))
    goto error2;

  if ((Ebcdic2AsciiH = DmGetResource(strRsc, e2a)) == NULL)
    goto error4;
  if ((Ebcdic2Ascii = MemHandleLock(Ebcdic2AsciiH)) == NULL)
    goto error5;

  if ((Ascii2EbcdicH = DmGetResource(strRsc, a2e)) == NULL)
    goto error6;
  if ((Ascii2Ebcdic = MemHandleLock(Ascii2EbcdicH)) == NULL)
    goto error7;

  replyHandle = DmGetResource('Data', queryReply);
  replySize = MemHandleSize(replyHandle);
  replyData = MemHandleLock(replyHandle);

  Cursor(0);
  Attr = &Buffer[BufferSize];
  InitTN3270(r, c);

  return 0;

error7:
  DmReleaseResource(Ascii2EbcdicH);
error6:
  MemHandleUnlock(Ebcdic2AsciiH);
error5:
  DmReleaseResource(Ebcdic2AsciiH);
error4:
  MemHandleUnlock(BufferH);
error2:
  DmReleaseRecord(dbRef, recBuffer, false);
error0:
  DmCloseDatabase(dbRef);
  return -1;
}

void CloseTN3270(void) {
  MemHandleUnlock(replyHandle);
  DmReleaseResource(replyHandle);
  MemHandleUnlock(Ebcdic2AsciiH);
  MemHandleUnlock(Ascii2EbcdicH);
  DmReleaseResource(Ebcdic2AsciiH);
  DmReleaseResource(Ascii2EbcdicH);
  MemHandleUnlock(BufferH);
  DmReleaseRecord(dbRef, recBuffer, true);
  DmCloseDatabase(dbRef);
}

static UInt16 sa2color(UInt16 value, Boolean fg) {
  UInt16 c;

  switch (value) {
    case 0xF0: c = COLOR_BLACK;   break; // neutral = black
    case 0xF1: c = COLOR_BLUE;    break; // blue
    case 0xF2: c = COLOR_RED;     break; // red
    case 0xF3: c = COLOR_MAGENTA; break; // pink
    case 0xF4: c = COLOR_GREEN;   break; // green
    case 0xF5: c = COLOR_CYAN;    break; // turquoise
    case 0xF6: c = COLOR_YELLOW;  break; // yellow
    case 0xF7: c = COLOR_WHITE;   break; // neutral = white
    default: c = fg ? COLOR_GREEN : COLOR_BLACK; break; // default
  }

  return c;
}

static void EmitChar3270(UInt8 c, UInt8 fa) {
  UInt16 attr;

  CursorTN3270(0);
  attr = 0;
  if (FA_INTENSIFIED(fa)) attr |= m_highlight;

  switch (sa_highlight) {
    case 0x00: // default
    case 0xF0: // normal
      break;
    case 0xF1: // blink
      break;
    case 0xF2: // reverse video
      attr |= m_reverse;
      break;
    case 0xF4: // underscore
      attr |= m_underscore;
      break;
  }

  SetForegroundColor(sa2color(sa_fg, true));
  SetBackgroundColor(sa2color(sa_bg, false));

  if (!FA_DISPLAY(fa)) c = ' ';
  DrawChar(c, attr, GetX(), GetY());
  SetLinear(BA);
}

void EmitTN3270(Char *buf, Int16 n, Int16 done) {
  Int16 i, j, k, next, len;
  UInt8 c, a;

  for (i = 0; i < n; i++) {
    c = buf[i];
    switch (tn3270_s) {
      case 1:	// CMD
        switch (c) {
          case CMD_EWA:
          case CMD_EW:
          case SNA_CMD_EWA:
          case SNA_CMD_EW:
            ClearScreen();
            Home();
            BA = 0;
            sa_highlight = 0;
            sa_fg = 0;
            sa_bg = 0;
            cursorBA = -1;
            DmSet(Buffer, 0, BufferSize*2, 0);
            // fall-through
          case CMD_W:
          case SNA_CMD_W:
            CMD = CMD_W;
            tn3270_s = 2;
            break;
          case CMD_RM:
          case SNA_CMD_RM:
            if (AID == AID_CLEAR || AID == AID_PA1 || AID == AID_PA2) {
              // Short read operation
              SendTelnet(&AID, 1, true);
              tn3270_s = 1;
              break;
            }
            // fall-through
          case CMD_RMA:
          case SNA_CMD_RMA:
            CMD = c;
            SendTelnet(&AID, 1, false);
            MakeBA_12A(BA, codBA);
            SendTelnet(codBA, 2, false);

            for (k = 0; k < BufferSize; k++) {
              if (!Attr[k]) continue;
              FA = Attr[k] & 0x7F;
              if (!FA_PROTECTED(FA) && FA_MDT(FA)) {     // se o campo foi modificado
                if ((next = NextField(k)) != -1) {
                  len = next - k - 1;
                  if (len < 0) len += BufferSize;
                  c = ORD_SBA;
                  SendTelnet(&c, 1, false);
                  // inicio do campo, apos o Field Attribute
                  tmpBA = (k + 1) % BufferSize;
                  MakeBA_12A(tmpBA, codBA);
                  SendTelnet(codBA, 2, false);
                  for (j = 0; j < len; j++) {
                    c = FromBuffer(tmpBA);
                    tmpBA = (tmpBA + 1) % BufferSize;
                    if (c) { // RM/RMA suprimem nulos
                      SendTelnet(&c, 1, false);
                    }
                  }
                  // passa o campo para nao modificado
                  ToAttr(k, FA_RESETMDT(FA) | 0x80);
                }
              }
            }
            // sinaliza fim dos dados: tamanho = 0, done = true
            SendTelnet(&c, 0, true);

            AID = AID_NO;
            tn3270_s = 1;
            break;
          case CMD_RB:
          case SNA_CMD_RB:
            CMD = c;
            AID = AID_NO;
            SendTelnet(&AID, 1, false);
            MakeBA_12A(BA, codBA);
            SendTelnet(codBA, 2, false);
            tmpBA = 0;

            for (k = 0; k < BufferSize; k++) {
              if (!Attr[k]) continue;
              if (k > tmpBA) {
                // campo artificial, preenchido com nulos
                c = ORD_SF;
                SendTelnet(&c, 1, false);
                c = 0xe4;
                SendTelnet(&c, 1, false);
                len = NextField(k) - tmpBA;
                c = 0;
                for (j = 0; j < len; j++)
                  SendTelnet(&c, 1, false);
              }
              tmpBA = k;
              len = NextField(k) - k;
              c = ORD_SF;
              SendTelnet(&c, 1, false);
              for (j = 0; j < len; j++) {
                c = FromBuffer(tmpBA+j);
                SendTelnet(&c, 1, false);
              }
              tmpBA += j;
            }

            // nulos apos o ultimo campo ate' o final do buffer
            len = BufferSize - tmpBA;
            c = 0;
            for (j = 0; j < len; j++)
              SendTelnet(&c, 1, false);

            // sinaliza fim dos dados: tamanho = 0, done = true
            SendTelnet(&c, 0, true);
            tn3270_s = 1;
            break;
          case CMD_EAU:
          case SNA_CMD_EAU:
            CMD = c;
            for (k = 0; k < BufferSize; k++) {
              if (!Attr[k]) continue;
              FA = Attr[k] & 0x7F;
              if (!FA_PROTECTED(FA)) {
                BA = k;
                ToAttr(k, FA_RESETMDT(FA) | 0x80);
                ToBuffer(0);
                CursorTN3270(0);
                SetLinear(BA);
                len = NextField(k) - k;
                for (j = 1; j < len; j++) {
                  ToAttr(BA, 0);
                  ToBuffer(0);
                  EmitChar3270(' ', FA);
                }
              }
            }
            AID = AID_NO;
            SendSpecial(keyHOME);
            tn3270_s = 1;
            break;
          case CMD_WSF:
          case SNA_CMD_WSF:
            tn3270_s = 12;
          default:
            break;
        }
        break;

      case 2:	// WCC
        WCC = c;
        if ((WCC & 0x04)) {
          // sound alarm bit
        }
        if ((WCC & 0x02)) {
          // keyboard restore bit (also resets AID byte)
        }
        if ((WCC & 0x01)) {
          // reset MDT bits in the field attributes
        }
        tn3270_s = 3;
        break;

      case 3:	// Orders and data
        switch (c) {
          case ORD_SF:
            tn3270_s = 4;
            break;
          case ORD_SBA:
            ORD = c;
            tn3270_s = 5;
            break;
          case ORD_SFE:		// ignored
            tn3270_s = 8;
            break;
          case ORD_SA:
            ORD = c;
            npairs = 1;
            tn3270_s = 9;
            break;
          case ORD_MF:		// ignored
            tn3270_s = 8;
            break;
          case ORD_IC:
            cursorBA = BA;
            tn3270_s = 3;
            break;
          case ORD_PT:		// ignored
            // advances the current buffer address to the address of the first character position of the next unprotected field
            tn3270_s = 3;
            break;
          case ORD_RA:
            // stores a specified character in all character buffer locations, starting
            // at the current buffer address and ending at (but not including) the specified stop address.
            ORD = c;
            tn3270_s = 5;
            break;
          case ORD_EUA:
            ORD = c;
            tn3270_s = 5;
            break;
          case ORD_GE:
            tn3270_s = 7;
            break;
          default:
            a = ebcdic2ascii(c);
            if (CMD == CMD_W) {
              ToAttr(BA, 0);
              ToBuffer(c);
              EmitChar3270(a, FA);
            }
            break;
        }
        break;

      case 4:	// Start Field: field attribute
        FA = c;
        ToAttr(BA, FA | 0x80);
        ToBuffer(0);
        EmitChar3270(' ', FA);
        tn3270_s = 3;
        break;

      case 5:	// Set Buffer Address: byte 1
        switch (ba_mode = BA_MODE(c)) {
          case BA_14:
            tmpBA = ((UInt16)(c & 0x3f)) << 8;
            break;
          case BA_12A:
          case BA_12B:
            tmpBA = ((UInt16)(c & 0x3f)) << 6;
            break;
          case BA_RESERVED:
            tmpBA = 0;
        }
        tn3270_s = 6;
        break;

      case 6:	// Set Buffer Address: byte 2
        switch (ba_mode) {
          case BA_14:  
            tmpBA = tmpBA | (UInt16)c;
            break;
          case BA_12A: 
          case BA_12B:
            tmpBA = tmpBA | (UInt16)(c & 0x3f);
        }
        switch (ORD) {
          case ORD_SBA:
            if (tmpBA < BufferSize) {
              BA = tmpBA;
              CursorTN3270(0);
              SetLinear(BA);
            }
            else {
            }
            tn3270_s = 3;
            break;
          case ORD_RA:
            tn3270_s = 11;
            break;
          case ORD_EUA:
            tn3270_s = 3;
        }
        break;

      case 7:	// Graphic Escape
        tn3270_s = 3;
        break;

      case 8:	// Start Field Extended: number of attr. type-value pairs
        npairs = c;
        tn3270_s = npairs ? 9 : 3;
        break;

      case 9:	// Type-Value pair: type
        sa_type = c;
        tn3270_s = 10;
        break;

      case 10:	// Type-Value pair: value
        sa_value = c;
        npairs--;
        tn3270_s = npairs ? 9 : 3;

        switch (ORD) {
          case ORD_SA:
            // http://www.prycroft6.com.au/misc/3270.html
            switch (sa_type) {
              case 0x00: // reset all attribute types (color, highlight, charset)
                sa_highlight = 0;
                sa_fg = 0;
                sa_bg = 0;
                break;
              case 0x41: // highlight
                sa_highlight = sa_value;
                break;
              case 0x42: // foreground color
                sa_fg = sa_value;
                break;
              case 0x43: // charset
                break;
              case 0x45: // background color
                sa_bg = sa_value;
                break;
            }
            break;
        }
        break;

      case 11:	// Order Repeat to Address: character
        if (c == ORD_GE) {
          tn3270_s = 7;
        } else {
          FA = 0;
          a = ebcdic2ascii(c);
          for (; BA < tmpBA;) {
            ToAttr(BA, 0);
            BA = ToBuffer(c);
            EmitChar3270(a, FA);
          }
          tn3270_s = 3;
        }
        break;

      case 12:	// Write Structured Field: length, byte 1
        length = ((UInt16)c) << 8;
        tn3270_s = 13;
        break;

      case 13:	// Write Structured Field: length, byte 2
        length |= (UInt16)c;
        length = (length > 1) ? length-2 : 0;
        isfcmd = 0;
        tn3270_s = length ? 14 : 12;
        break;

      case 14:	// Write Structured Field: field
        if (isfcmd < 3) {
          SFCMD[isfcmd++] = c;
        }
        length--;
        if (!length) {
          // Read Partition Reply
          if (SFCMD[0] == 0x01 && SFCMD[1] == 0xff && SFCMD[2] == 0x02) {
            SendTelnet(replyData, replySize, true);
/*
            tmpBA = 0x0003;
            SendTelnet((UInt8 *)&tmpBA, 2, false);
            c = 0x81;	// Query Reply
            SendTelnet(&c, 1, true);
*/
          }
          tn3270_s = 12;
        }
    }
  }

  if (done) {
    tn3270_s = 1;
    length = 0;
    tmpBA = BA;
    BA = BufferSize;
    BA = tmpBA;
    if (cursorBA != -1) {
      BA = cursorBA;
      SetLinear(BA);
      CursorTN3270(1);
    }
  }

  tnDone = done;
}

void PositionTN3270(Int16 col, Int16 row) {
  Int16 pos;
  if (col >= 0 && col < GetCols() && row >= 0 && row < GetRows()) {
    pos = row * GetCols() + col;
    if (cursorBA != -1) {
      CursorTN3270(0);
    }
    cursorBA = pos;
    BA = cursorBA;
    SetLinear(BA);
    CursorTN3270(1);
  }
}

static void CursorTN3270(Int16 s) {
  Int16 ba, i;
  UInt8 fa, c;

  ba = GetLinear();
  if (!Attr[ba]) {
    if ((i = CurrentField(ba)) != -1) {
      c = ebcdic2ascii(Buffer[ba]);
      fa = Attr[i];
      if (!FA_DISPLAY(fa)) c = ' ';
      DrawChar(c, s ? m_reverse : 0, GetX(), GetY());
    }
  }
}

Err SendTN3270(UInt16 c) {
  Int16 i;
  Char cmd;

  if (!tnDone)
    return -1;

  switch (c) {
    case pageUpChr:	// Up arrow key
      if (GetY())
        DecY();
      else
        SetY(rows-1);
      BA = GetLinear();
      break; 
    case pageDownChr:	// Down arrow key
      if (GetY() < (rows-1))
        IncY();
      else
        SetY(0);
      BA = GetLinear();
      break;
    case hard1Chr:
      break;
    case hard2Chr:	// Left arrow key
      if (GetX())
        DecX();
      else
        SetX(cols-1);
      BA = GetLinear();
      break;
    case hard3Chr:	// Right arrow key
      if (GetX() < (cols-1))
        IncX(1);
      else
        SetX(0);
      BA = GetLinear();
      break;
    case hard4Chr:	// Enter key
    case 10:
      AID = AID_ENTER;
      // simula um comando RMA (o efeito e' o mesmo)
      cmd = CMD_RMA;
      EmitTN3270(&cmd, 1, 1);
      AID = AID_NO;
      break;
    case 8:             // Backspace
      if ((i = CurrentField(BA)) != -1) {
        FA = Attr[i] & 0x7F;
        if (!FA_PROTECTED(FA)) {
          BA--;
          if (BA < 0) BA = BufferSize-1;
          ToAttr(BA, 0);
          ToBuffer(ascii2ebcdic(' '));
          EmitChar3270(' ', FA);
          BA--;
          if (BA < 0) BA = BufferSize-1;
          SetLinear(BA);
          CursorTN3270(1);
        }
      }
      break;
    default:
      if ((i = CurrentField(BA)) != -1) {
        FA = Attr[i] & 0x7F;
        if (!FA_PROTECTED(FA)) {
          ToAttr(BA, 0);
          ToBuffer(ascii2ebcdic(c));
          EmitChar3270(c, FA);
          CursorTN3270(1);
          ToAttr(i, FA_SETMDT(FA) | 0x80);
        } else {
        }
      }
  }

  return 0;
}

Err SendAID(UInt16 c) {
  AID = c;
  switch (AID) {
    case AID_PA1:
    case AID_PA2:
    case AID_PA3:
    case AID_CLEAR:
      SendTelnet(&AID, 1, true);
      break;
    default:
      SendTelnet(&AID, 1, false);
      MakeBA_12A(BA, codBA);
      SendTelnet(codBA, 2, true);
  }

  return 0;
}

Err SendSpecial(UInt16 c) {
  switch (c) {
    case keyLTAB:
      break;
    case keyRTAB:
      break;
    case keyHOME:
      if ((BA = FirstField()) != -1) {
        // salta o Field Attribute
        BA = (BA + 1) % BufferSize;
      } else {
        BA = 0;
      }
      CursorTN3270(0);
      SetLinear(BA);
      break;
  }

  return 0;
}

// a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  p  q  r  s  t  u  v  w  x  y  z
// 81 82 83 84 85 86 87 88 89 91 92 93 94 95 96 97 98 99 a2 a3 a4 a5 a6 a7 a8 a9

// A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z
// c1 c2 c3 c4 c5 c6 c7 c8 c9 d1 d2 d3 d4 d5 d6 d7 d8 d9 e2 e3 e4 e5 e6 e7 e8 e9

static UInt8 ascii2ebcdic(UInt8 c) {
  return c ? Ascii2Ebcdic[c-1] : 0;
}

static UInt8 ebcdic2ascii(UInt8 c) {
  return c ? Ebcdic2Ascii[c-1] : 0;
}

static void MakeBA_12A(UInt16 ba, UInt8 *b) {
  // fedcba98 76543210
  // xxxxnnnn nnnnnnnn

  b[0] = 0x40 | ((ba & 0x0fc0) >> 6);
  b[1] = ba & 0x3f;
}

static UInt8 FromBuffer(Int16 i) {           
  if (i >= BufferSize) {
    i = i % BufferSize;
  }

  return Buffer[i];
}

static void ToAttr(Int16 i, UInt8 c) {
  DmWrite(Buffer, BufferSize + i, &c, 1);
}

static Int16 ToBuffer(UInt8 c) {
  DmWrite(Buffer, BA++, &c, 1);
  if (BA == BufferSize) {
    BA = 0;
  }

  return BA;
}

static Int16 FirstField(void) {
  Int16 i;

  // procura o primeiro campo nao protegido

  for (i = 0; i < BufferSize; i++) {
    if (Attr[i] && !FA_PROTECTED(Attr[i])) {
      return i;
    }
  }

  // nao ha' campos nao protegidos
  return -1;
}

static Int16 NextField(Int16 ba) {
  Int16 i;

  for (i = 0; i < BufferSize; i++) {
    ba++;
    if (ba == BufferSize) ba = 0;
    if (Attr[ba]) return ba;
  }

  return -1;
}

static Int16 CurrentField(Int16 ba) {
  Int16 i;

  for (i = 0; i < BufferSize; i++) {
    ba = ba -1;
    if (ba < 0) ba = BufferSize-1;
    if (Attr[ba]) return ba;
  }

  return -1;
}

/*
static UInt16 FromFields(Int16 i) {
  return (((UInt16)Fields[2*i]) << 8) | (UInt16)Fields[2*i+1];
}

static void StartField(void) {
  UInt8 b;

  if (ifields < 2*FieldsSize) {
    b = BA >> 8;
    DmWrite(Fields, 2*ifields, &b, 1);
    b = BA & 0x00ff;
    DmWrite(Fields, 2*ifields+1, &b, 1);
    ifields++;
  }
}

static void EndField(void) {
  UInt16 ba, len;
  UInt8 b;

  if (ifields % 2) {
    ba = FromFields(ifields-1);
    len = (BA < ba) ? BA + BufferSize - ba : BA - ba;
    b = len >> 8;
    DmWrite(Fields, 2*ifields, &b, 1);
    b = len & 0x00ff;
    DmWrite(Fields, 2*ifields+1, &b, 1);
    ifields++;
  }
}

static Int16 FirstField(void) {
  Int16 i;

  // procura o primeiro campo nao protegido

  for (i = 0; i < ifields; i += 2) {
    if (!FA_PROTECTED(FromBuffer(FromFields(i)))) {
      return i;
    }
  }

  // nao ha' campos nao protegidos

  return -1;
}

static Int16 CurrentField(void) {
  Int16 i;
  UInt16 ba;

  if (!ifields)
    return -1;

  for (i = ifields-2; i >= 0; i -= 2) {
    ba = FromFields(i);
    if (ba == BA)
      return -1;
    if (ba < BA)
      return FA_PROTECTED(FromBuffer(ba)) ? -1 : i;
  }

  return -1;
}
*/
