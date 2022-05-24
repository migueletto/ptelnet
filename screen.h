#define bottomLine	149
#define m_reverse     0x1
#define m_underscore  0x2
#define m_highlight   0x4

#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7
#define COLOR_SIZE    8

void InitFonts(void);
Err SetCharset(Short, ULong);
Err InitScreen(UInt16 f, UInt16 density, UInt16 cols);
void SetFont(UInt16 f, UInt16 density);
void CloseScreen(void);
void ScreenDefaultColors(void);
Short GetRows(void);
Short GetCols(void);
void SwitchScreen(Short);
Boolean DoSwitch(Short, Short);
void Cursor(Int);
void Home(void);
Short GetX(void);
Short GetY(void);
Short GetLinear(void);
void SetLinear(Short);
void SetX(Short);
void SetY(Short);
void SaveCursor(void);
void RestoreCursor(void);
void IncY(void);
void DecY(void);
void IncX(Short);
void DecX(void);
void Underscore(Short);
Short IsUnderscore(void);
void Reverse(Short);
Short IsReverse(void);
void ClearScreen(void);
void ClearScreenToEnd(void);
void ClearScreenFromBegin(void);
void ClearLine(void);
void ClearLineToEnd(void);
void ClearLineFromBegin(void);
void ScrollUp(void);  
void ScrollDown(void);
void ChangeScrollingRegion(Int, Int);
void DrawChar(Short, UShort, Short, Short);
void DrawSeparator(void);
void DeleteChar(void);
void SetForegroundColor(UShort);
void SetBackgroundColor(UShort);
Int16 FontWidth(void);
Int16 FontHeight(void);
