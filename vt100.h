// Constantes

#define ESC 27
#define DEL 127

#define VT100_upArrow		'A'
#define VT100_downArrow		'B'
#define VT100_rightArrow	'C'
#define VT100_leftArrow		'D'
#define VT100_PF		'O'
#define VT100_NUM		'p'

// Prototipos

void InitVT100(Short, Short);
void EmitVT100(Char *, Int, Short);
void EmitCharVT100(UChar);
Char *GetKeySeq(UChar);
