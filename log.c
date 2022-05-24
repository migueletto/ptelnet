#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "const.h"
#include "ptelnet.h"
#include "log.h"
#include "memo.h"
#include "db.h"
#include "misc.h"
#include "error.h"

#define LF 10

DmOpenRef OpenLog(Word cat) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  DmOpenRef dbRef;

  p->log_index = 0;
  p->log_rec = NULL;
  p->log_n = 0;
  p->log_nhex = 0;
  p->log_max = memoRecSize;
  p->log_category = cat;
  p->log_header[0] = 0;
  p->log_recn = 0;
  p->log_mode = 0;

  if ((dbRef = DbOpenByName(MemoDBName, dmModeReadWrite)) == NULL)
    return NULL;

  return dbRef;
}

void CloseLog(DmOpenRef dbRef) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();

  if (dbRef) {
    if (p->log_rec)
      DbCloseRec(dbRef, p->log_index, p->log_rec);
    DbClose(dbRef);
    p->log_rec = NULL;
  }
}

Err LogByte(DmOpenRef dbRef, UChar b) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  Char hex[10];
  CharPtr s;
  Int m, i;
  Err err;

  if (!dbRef) return -1;

  if (!p->log_rec || (p->log_mode && p->log_n >= p->log_max-4) || (!p->log_mode && p->log_n == p->log_max)) {
    p->log_recn++;
    p->log_n = 0;
    p->log_nhex = 0;
    if (p->log_rec)
      DbCloseRec(dbRef, p->log_index, p->log_rec);
    p->log_index = 0;
    if ((p->log_rec = DbCreateRec(dbRef, &p->log_index, memoRecSize, p->log_category)) == NULL)
      return -1;
    if (p->log_header[0]) {
      StrPrintF(p->log_buf, "%s%d%c", p->log_header, p->log_recn, LF);
      p->log_n = StrLen(p->log_buf);
      if ((err = DmCheckAndWrite(p->log_rec, 0, p->log_buf, p->log_n)) != 0) {
        ErrorDialog("Write log", err);
        p->log_n = 0;
        return -1;
      }
    }
  }

  if (p->log_mode) {	// Hex
    StrPrintF(hex, "%x%c", b, LF);
    s = StrLen(hex) == 5 ? &hex[1] : &hex[5];
    s[0] = ' ';
    i = (p->log_nhex % 8) ? 0 : 1;
    m = (p->log_nhex && !((p->log_nhex+1) % 8)) ? 3 : 2;
    m += 1-i;
    if ((err = DmCheckAndWrite(p->log_rec, p->log_n, &s[i], m)) != 0) {
      ErrorDialog("Write log", err);
      return -1;
    }
    p->log_n += m;
    p->log_nhex++;
  } else {	// ASCII
    if ((err = DmCheckAndWrite(p->log_rec, p->log_n, &b, 1)) != 0) {
      ErrorDialog("Write log", err);
      return -1;
    }
    p->log_n++;
  }

  return 0;
}

Err LogBytes(DmOpenRef dbRef, UCharPtr buf, UInt n) {
  Int i;

  for (i = 0; i < n; i++) {
    if (LogByte(dbRef, buf[i]) != 0) {
      return -1;
    }
  }

  return 0;
}

void LogSetHeader(CharPtr h) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();

  if (!h) {
    p->log_header[0] = 0;
  } else {
    StrNCopy(p->log_header, h, sizeof(p->log_header)-1);
  }
}

void LogSetMode(UChar m) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  p->log_mode = m;
}
