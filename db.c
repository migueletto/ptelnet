#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "db.h"
#include "error.h"

DmOpenRef DbOpen(LocalID dbID, UInt mode) {
  DmOpenRef dbRef;

  if (!(dbRef = DmOpenDatabase(0, dbID, mode))) {
    ErrorDialog("Open DB", DmGetLastErr());
    return NULL;
  }

  return dbRef;
}

DmOpenRef DbOpenByName(CharPtr name, UInt mode) {
  LocalID dbID;

  if ((dbID = DmFindDatabase(0, name)) == 0) {
    ErrorDialog("Find DB", DmGetLastErr());
    return NULL;
  }

  return DbOpen(dbID, mode);
}

void DbClose(DmOpenRef dbRef) {
  if (dbRef)
    DmCloseDatabase(dbRef);
}

CharPtr DbOpenRec(DmOpenRef dbRef, UInt index) {
  VoidHand recH;
  CharPtr rec;

  if (!dbRef)
    return NULL;

  if (!(recH = DmQueryRecord(dbRef, index)))
    return NULL;

  if (!(recH = DmGetRecord(dbRef, index))) {
    ErrorDialog("Get rec", DmGetLastErr());
    return NULL;
  }

  if ((rec = MemHandleLock(recH)) == NULL) {
    ErrorDialog("Lock rec", DmGetLastErr());
    DmReleaseRecord(dbRef, index, false);
    return NULL;
  }

  return rec;
}

void DbCloseRec(DmOpenRef dbRef, UInt index, CharPtr rec) {
  if (dbRef && rec) {
    MemPtrUnlock(rec);
    DmReleaseRecord(dbRef, index, false);
  }
}

CharPtr DbCreateRec(DmOpenRef dbRef, UIntPtr index, UShort size, Word category) {
  VoidHand recH;
  CharPtr rec;
  Err err;

  *index = 0;

  if (!dbRef)
    return NULL;

  if (!(recH = DmNewRecord(dbRef, index, size))) {
    ErrorDialog("New rec", DmGetLastErr());
    return NULL;
  }

  if ((rec = MemHandleLock(recH)) == NULL) {
    ErrorDialog("Lock rec", DmGetLastErr());
    DmReleaseRecord(dbRef, *index, false);
    DmRemoveRecord(dbRef, *index);
    return NULL;
  }

  if ((err = DmSet(rec, 0, size, 0)) != 0) {
    ErrorDialog("Fill rec", err);
    MemHandleUnlock(recH);
    DmReleaseRecord(dbRef, *index, false);
    DmRemoveRecord(dbRef, *index);
    return NULL;
  }

  return rec;
}
