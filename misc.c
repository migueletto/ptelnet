#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "misc.h"

void FldInsertDec(VoidPtr v, UShort id, ULong hex) {
  Char aux[32];
  FormPtr frm = (FormPtr)v;

  FieldPtr fld;
  fld = (FieldPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, id));
  FldDelete(fld, 0, FldGetTextLength(fld));
  StrIToA(aux, hex);
  FldInsert(fld, aux, StrLen(aux));
}

void FldInsertStr(VoidPtr v, UShort id, CharPtr str) {
  FieldPtr fld;
  Word index, len;
  FormPtr frm = (FormPtr)v;

  index = FrmGetObjectIndex(frm, id);
  fld = (FieldPtr)FrmGetObjectPtr(frm, index);
  len = FldGetTextLength(fld);
  FldDelete(fld, 0, len);
  if (str && str[0]) {
    FldInsert(fld, str, StrLen(str));
  } else {
    FldInsert(fld, "", 0);
  }
}

Err DmCheckAndWrite(VoidPtr dst, ULong offset, VoidPtr src, ULong bytes) {
  Err err;

  if ((err = DmWriteCheck(dst, offset, bytes)) != 0) {
    return err;
  }
  return DmWrite(dst, offset, src, bytes);
}
