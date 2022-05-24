#define MemoAppID	'memo'
#define MemoDBType	'DATA'
#define MemoDBName	"MemoDB"
#define memoRecSize 	4096

DmOpenRef MemoOpen(UInt);
Int MemoGetList(DmOpenRef, ListPtr);
void MemoFreeList(void);
void MemoClose(DmOpenRef);
CharPtr MemoOpenRec(DmOpenRef, UInt);
CharPtr MemoOpenListRec(DmOpenRef, UInt);
void MemoCloseRec(DmOpenRef, UInt, CharPtr);
void MemoCloseListRec(DmOpenRef, UInt, CharPtr);
CharPtr MemoCreateRec(DmOpenRef, UIntPtr, Word);
