#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "const.h"
#include "telnet.h"
#include "pdebug.h"

void EmitTerminal(Char *, Int, Short);
Int PhysSend(UChar *, Int, Err *);

static Short telnet_s;
static UChar telnetSB;
static Boolean telnetEor, telnetBinary;
static Char term[16];
static Short rows, cols;

void InitTelnet(Char *t, Short r, Short c)
{
  Err err;
  UChar outbuf[4];

  telnet_s = 1;
  telnetSB = 0;
  telnetEor = false;   
  telnetBinary = false;
  StrCopy(term, t);
  rows = r;
  cols = c;

  // Gambiarra: achar outro jeito de saber se devo enviar WILL ECHO ou nao
  if (term[0] == 'v') {
    outbuf[0] = TELNET_IAC;
    outbuf[1] = TELNET_WILL;
    outbuf[2] = TELOPT_ECHO;
    PhysSend(outbuf, 3, &err);
#ifdef DEBUG
    DebugString("> WILL ECHO");
#endif
  }
}

void EmitTelnet(UChar *buf, Int n)
{
  Err err;
  Int i, k, t;
  UChar c, outbuf[32], inbuf[tamBuf];

  for (i = 0, k = 0; i < n; i++) {
    c = buf[i];
    switch (telnet_s) {
      case 1:
        if (c == TELNET_IAC) {
          telnet_s = 2;
          break;
        }
pass:
        switch (c) {
          case '\0':
            if (!telnetBinary)
              break;
            // else fall-through
          default:
            if (k < tamBuf)
              inbuf[k++] = c;
        }
        break;

      case 2:
        switch (c) {
          case TELNET_IAC:
            telnet_s = 1;
            goto pass;
          case TELNET_NOP:
          case TELNET_DM:
          case TELNET_BRK:
          case TELNET_IP:
          case TELNET_AO:
          case TELNET_AYT:
          case TELNET_GA:
          case TELNET_EC:
          case TELNET_EL:
            telnet_s = 1;
#ifdef DEBUG
            DebugByte("< TELNET ", c);
#endif
            break;
          case TELNET_WILL:
            telnet_s = 5;
            break;
          case TELNET_WONT:
            telnet_s = 6;
            break;
          case TELNET_DO:
            telnet_s = 7;
            break;
          case TELNET_DONT:
            telnet_s = 3;
            break;
          case TELNET_SB:
            telnet_s = 4;
            break;
          case TELNET_SE:
            if (telnetSB == TELOPT_TTYPE) {
              outbuf[0] = TELNET_IAC;
              outbuf[1] = TELNET_SB;
              outbuf[2] = TELOPT_TTYPE;
              outbuf[3] = TELNET_IS;
              for (t = 4; term[t-4]; t++)
                outbuf[t] = term[t-4];
              outbuf[t++] = TELNET_IAC;
              outbuf[t++] = TELNET_SE;
              PhysSend(outbuf, t, &err);
              telnetSB = 0;
#ifdef DEBUG
              DebugByte("> SB ", TELOPT_TTYPE);
#endif
            }
            telnet_s = 1;
            break;
          case TELNET_EOR:
            if (telnetEor) {
              if (k) {
                EmitTerminal((Char *)inbuf, k, 1);
              }
              k = 0;
            }
#ifdef DEBUG
            DebugString("< EOR");
#endif
            telnet_s = 1;
            break;
          default:
            telnet_s = 1;
            break;
        }
        break;

      case 7:  // DO
#ifdef DEBUG
        DebugByte("< DO ", c);
#endif
        switch (c) {
          case TELOPT_EOR:
            telnetEor = true;
            outbuf[1] = TELNET_WILL;
            break;
          case TELOPT_BINARY:
            telnetBinary = true;
            outbuf[1] = TELNET_WILL;
            break;
          case TELOPT_ECHO:
          case TELOPT_TTYPE:
          case TELOPT_SGA:
            outbuf[1] = TELNET_WILL;
            break;
          case TELOPT_NAWS:
#ifdef DEBUG
            DebugByte("> WILL ", c);
#endif
            outbuf[0] = TELNET_IAC;
            outbuf[1] = TELNET_WILL;
            outbuf[2] = c;
            outbuf[3] = TELNET_IAC;
            outbuf[4] = TELNET_SB;
            outbuf[5] = c;
            outbuf[6] = 0;
            outbuf[7] = cols;
            outbuf[8] = 0;
            outbuf[9] = rows;
            outbuf[10] = TELNET_IAC;
            outbuf[11] = TELNET_SE;
            PhysSend(outbuf, 12, &err);
            telnet_s = 1;
            continue;
          default:
            outbuf[1] = TELNET_WONT;
        }
        outbuf[0] = TELNET_IAC;
        outbuf[2] = c;
        PhysSend(outbuf, 3, &err);
        telnet_s = 1;
#ifdef DEBUG
        if (outbuf[1] == TELNET_WILL)
          DebugByte("> WILL ", c);
        else
          DebugByte("> WONT ", c);
#endif
        break;

      case 5:  // WILL
#ifdef DEBUG
        DebugByte("< WILL ", c);
#endif
        switch (c) {
          case TELOPT_EOR:
          case TELOPT_ECHO:
          case TELOPT_BINARY:
            outbuf[1] = TELNET_DO;
            break;
          default:
            outbuf[1] = TELNET_DONT;
        }
        outbuf[0] = TELNET_IAC;
        outbuf[2] = c;
        PhysSend(outbuf, 3, &err);
        telnet_s = 1;
#ifdef DEBUG
        if (outbuf[1] == TELNET_DO)
          DebugByte("> DO ", c);
        else
          DebugByte("> DONT ", c);
#endif
        break;

      case 3:  // DONT
        telnet_s = 1;
#ifdef DEBUG
        DebugByte("< DONT ", c);
#endif
        break;

      case 6:  // WONT
#ifdef DEBUG
        DebugByte("< WONT ", c);
#endif
        switch (c) {
          case TELOPT_BINARY:
          case TELOPT_EOR:
            outbuf[0] = TELNET_IAC;
            outbuf[1] = TELNET_DONT;
            outbuf[2] = c;
            PhysSend(outbuf, 3, &err);
            if (c == TELOPT_EOR)
              telnetEor = false;
            else if (c == TELOPT_BINARY)
              telnetBinary = false;
#ifdef DEBUG
            DebugByte("> DONT ", c);
#endif
        }
        telnet_s = 1;
        break;

      case 4:  // SB
#ifdef DEBUG
        DebugByte("< SB ", c);
#endif
        switch (c) {
          case TELOPT_TTYPE:
            telnet_s = 8;
            break;
          default:
            telnet_s = 9;
        }
        break;

      case 8:  // SB TTYPE
        if (c == TELNET_SEND)
          telnetSB = TELOPT_TTYPE;
        telnet_s = 1;
        break;

      case 9:  // SB other
        telnet_s = 1;
        break;
    }
  }

  if (k) {
    EmitTerminal((Char *)inbuf, k, 0);
  }
}

void SendTelnet(UChar *buf, Int n, Short done)
{
  UChar outbuf[4];
  Int i, k;
  Err err;

  outbuf[0] = TELNET_IAC;

  for (i = 0, k = 0; i < n; i++) {
    if (buf[i] == TELNET_IAC) {
      if ((i-k) > 0) {
        PhysSend(&buf[k], i-k, &err);
      }
      outbuf[1] = TELNET_IAC;
      PhysSend(outbuf, 2, &err);
      k = i+1;  
    }
  }
              
  if (i > k) {
    PhysSend(&buf[k], i-k, &err);
  }

  if (telnetEor && done) {
    outbuf[1] = TELNET_EOR;
    PhysSend(outbuf, 2, &err);
  }
}
