// Prototipos

void InitTelnet(Char *, Short, Short);
void EmitTelnet(UChar *, Int);
void SendTelnet(UChar *, Int, Short);


// Constantes

#define TELNET_xEOF     236
#define TELNET_SUSP     237
#define TELNET_ABORT    238
#define TELNET_EOR      239
#define TELNET_SE       240
#define TELNET_NOP      241
#define TELNET_DM       242
#define TELNET_BRK      243
#define TELNET_IP       244
#define TELNET_AO       245
#define TELNET_AYT      246
#define TELNET_EC       247
#define TELNET_EL       248
#define TELNET_GA       249
#define TELNET_SB       250
#define TELNET_WILL     251
#define TELNET_WONT     252
#define TELNET_DO       253
#define TELNET_DONT     254
#define TELNET_IAC      255

#define TELNET_SEND	1
#define TELNET_IS	0

#define TELOPT_BINARY		0
#define TELOPT_ECHO		1
#define TELOPT_SGA		3
#define TELOPT_TM		6
#define TELOPT_TTYPE		24
#define TELOPT_EOR		25
#define TELOPT_NAWS		31
#define TELOPT_LINEMODE		34
