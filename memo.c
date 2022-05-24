#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "const.h"
#include "memo.h"
#include "db.h"
#include "error.h"

#define nameSize    24

static UIntPtr ind;
static CharPtr *name, pool;
static Char buf[nameSize];
static DmSearchStateType search;

DmOpenRef MemoOpen(UInt mode) {
  Err err;
  LocalID dbID;
  UShort cardNo;
  DmOpenRef dbRef;
  ULong numRecs;

  ind = NULL;
  name = NULL;
  pool = NULL;

  if ((err = DmGetNextDatabaseByTypeCreator(true, &search, MemoDBType, MemoAppID, false, &cardNo, &dbID)) != 0) {
    ErrorDialog("Find Memo", err);
    return NULL;
  }

  if (!(dbRef = DmOpenDatabase(cardNo, dbID, mode))) {
    ErrorDialog("Open Memo", DmGetLastErr());
    return NULL;
  }

  numRecs = DmNumRecords(dbRef);

  if (!(numRecs)) {
    return dbRef;
  }

  if ((ind = (UIntPtr)MemPtrNew(numRecs*sizeof(UInt))) == NULL) {
    ErrorDialog("Memory 1", 0);
    DmCloseDatabase(dbRef);
    return NULL;
  }

  return dbRef;
}

Int MemoGetList(DmOpenRef dbRef, ListPtr list) {
  CharPtr rec;
  UInt index;
  Int i, iname, k;
  ULong numRecs;

  if (!dbRef) {
    return -1;
  }

  numRecs = DmNumRecords(dbRef);
  if (numRecs == 0) {
    return 0;
  }

  if ((name = (CharPtr *)MemPtrNew(numRecs*sizeof(CharPtr))) == NULL) {
    ErrorDialog("Memory 2", 0);
    return -1;
  }

  if ((pool = (CharPtr)MemPtrNew(numRecs*nameSize)) == NULL) {
    MemPtrFree(name);
    name = NULL;
    ErrorDialog("Memory 3", 0);
    return -1;
  }

  MemSet(pool, sizeof(pool), 0);
  MemSet(name, sizeof(name), 0);

  for (index = 0, iname = 0, k = 0; index < numRecs; index++) {
    if ((rec = DbOpenRec(dbRef, index)) == NULL) {
      MemPtrFree(name);
      name = NULL;
      ErrorDialog("Memory 4", 0);
      return -1;
    }

    MemSet(buf, sizeof(buf), 0);
    StrNCopy(buf, rec, sizeof(buf)-1);
    DbCloseRec(dbRef, index, rec);

    for (i = 0; buf[i]; i++) {
      if (buf[i] == 10 || buf[i] == 13) {
        buf[i] = 0;
        break;
      }
    }

    ind[iname] = index;

    StrCopy(&pool[k], buf);
    name[iname++] = &pool[k];
    k += nameSize;
  }

  LstSetListChoices(list, name, iname);
  if (iname) {
    LstSetSelection(list, 0);
  }

  return numRecs;
}

void MemoFreeList() {
  if (name) {
    MemPtrFree(name);
    name = NULL;
  }
  if (pool) {
    MemPtrFree(pool);
    pool = NULL;
  }
}

void MemoClose(DmOpenRef dbRef) {
  if (ind) {
    MemPtrFree(ind);
    ind = NULL;
  }
  if (dbRef) {
    DmCloseDatabase(dbRef);
  }
}

CharPtr MemoOpenListRec(DmOpenRef dbRef, UInt index) {
  return DbOpenRec(dbRef, ind[index]);
}

void MemoCloseListRec(DmOpenRef dbRef, UInt index, CharPtr rec) {
  DbCloseRec(dbRef, ind[index], rec);
}
