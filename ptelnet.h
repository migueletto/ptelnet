#define AppID	   'Ptln'		//0x50746c6e
#define AppName	 "ptelnet"

#define DBType	 'Data'
#define BufferDB "p3270Buffer"

#define CHUNK        128
#define DOCBUFLEN    257

#define MAX_DEVICES     16
#define MAX_DEVICENAME  32

typedef struct {
  Char ptVersion[8];
  UInt32 density;
  UShort AppNetRefnum;
  UShort AppSerRefnum;
  UInt32 deviceCreators[MAX_DEVICES];
  char deviceNames[MAX_DEVICES][MAX_DEVICENAME];
  char *dnames[MAX_DEVICES];
  UInt16 numDevices;
  UChar inbuf[tamBuf];
  Boolean online;
  Boolean controlState;
  Boolean fontChanged;
  Short iMacro;
  Short wait;
  Word ticksPerSecond;
  Boolean pastingMemo;
  Word iPaste, nPaste;
  Word indexPaste;
  CharPtr bufPaste;
  Char buf[CHUNK+1];
  DmOpenRef dbRef;
  Char logHeader[32];

  NetSocketRef sock;
  Long AppNetTimeout;
  NetSocketAddrINType addr;
  NetHostInfoBufType hinfo;

  Long AppSerTimeout;
  VoidHand BufferH;
  VoidPtr Buffer;

  Char name[dmDBNameLength];

  Word log_category;
  UInt log_index, log_n, log_nhex, log_max, log_recn;
  CharPtr log_rec;
  Char log_header[32], log_buf[40];
  UChar log_mode;

} PtelnetGlobalsType;

typedef struct {
  UChar   physMode;             // serial, telnet
  Char    Host[tamHost];
  UShort  Port;
  UChar   lookupTimeout;        // DNS lookup timeout in seconds
  UChar   connectTimeout;       // connect timeout in seconds
  UChar   Font;                 // 0 = 5x10, 1 = 4x7
  UChar   Cr;                   // 0 = CR, 1 = LF, 2 = CR/LF
  UChar   Echo;                 // 1 = local, 0 = remoto
  UChar   dumbByte0;
  UInt32  density;
  UChar   Bits;                 // 5, 6, 7, ou 8
  UChar   Parity;               // 0 = none, 1 = even, 2 = odd
  UChar   StopBits;             // 1 ou 2
  UChar   XonXoff;
  UChar   RtsCts;
  UInt32  Baud;
  UChar   Pulse;		// 1 = pulse, 0 = tone
  Char    Phone[tamPhone];
  Char    MdmCmd[tamMdmcmd];
  Char    MacroName[numMacros][tamName];
  Char    MacroDef[numMacros][tamMacro];
  UChar   Log;			// 1 = log ativo, 0 = inativo
  UChar   LogCategory;		// log category;
  UChar   LogHeader;		// 1 = loga header, 0 = nao
  UChar   LogMode;		// 1 = Hex, 0 = ASCII
  UInt8   serialPort;
  UChar   dumbByte3, dumbByte4;
  UChar   dumbByte5, dumbByte6, dumbByte7, dumbByte8;
  UInt    dumbInt1, dumbInt2;
} PtelnetPreferencesType;

typedef enum {serial, telnet, tn3270} PhysMode;

Int PhysSend(UChar *, Int, Err *);

int PtelnetNewGlobals(void);
PtelnetGlobalsType *PtelnetGetGlobals(void);
int PtelnetFreeGlobals(void);

void EmitTerminal(Char *buf, Int n, Short done);

#define PREFS	5
