DmOpenRef OpenLog(Word);
void CloseLog(DmOpenRef);
Err LogByte(DmOpenRef, UChar);
Err LogBytes(DmOpenRef, UCharPtr, UInt);
void LogSetHeader(CharPtr);
void LogSetMode(UChar);
