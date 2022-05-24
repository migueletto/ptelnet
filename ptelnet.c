#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "sec.h"
#include "const.h"
#include "ptelnet.h"
#include "network.h"
#include "telnet.h"
#include "screen.h"
#include "vt100.h"
#include "tn3270.h"
#include "serial.h"
#include "memo.h"
#include "log.h"
#include "misc.h"
#include "error.h"

static Err StartApplication(void);
static void EventLoop(void);
static void StopApplication(void);

static void ReturnToMainForm(void);
static Boolean HardbuttonHandleEvent(EventPtr);
static Boolean ApplicationHandleEvent(EventPtr);
static Boolean MainFormHandleEvent(EventPtr);
static Boolean KbdFormHandleEvent(EventPtr);
static Boolean KbdVT100FormHandleEvent(EventPtr);
static Boolean Kbd3270FormHandleEvent(EventPtr);
static Boolean NetworkFormHandleEvent(EventPtr);
static Boolean SerialFormHandleEvent(EventPtr);
static Boolean TerminalFormHandleEvent(EventPtr);
static Boolean MacrosFormHandleEvent(EventPtr);
static Boolean LogFormHandleEvent(EventPtr);
static Boolean PasteMFormHandleEvent(EventPtr);
static Boolean AboutFormHandleEvent(EventPtr);
static Err PhysOnline(void);
static void PhysOffline(void);
static Int Receive(void);
static Err Send(UShort);
static Char ControlChar(Char c);
static Err SendString(CharPtr);
static Int PhysReceive(UChar *, Int, Err *);
static Err PhysBreak(void);
static void RetrieveVersion(Char *);
static void CopyPrefs(void);
static void SavePrefs(void);
static void MacrosToFields(FormPtr, PtelnetPreferencesType *, Short);
static void FieldsToMacros(FormPtr, PtelnetPreferencesType *, Short);
static void MacrosToList(ListPtr, PtelnetPreferencesType *);
static void CloseKeypad(void);
static UChar HexDigit(Char);
static void ResizeForm(FormPtr frm);

static PtelnetPreferencesType prefs, prefs_aux;

#define msgNetworkLibrary "Net.lib"

#ifndef UNIX
static PtelnetGlobalsType globals;

int PtelnetNewGlobals(void) {
  MemSet(&globals, sizeof(PtelnetGlobalsType), 0);
  return 0;
}

PtelnetGlobalsType *PtelnetGetGlobals(void) {
  return &globals;
}

int PtelnetFreeGlobals(void) {
  return 0;
}
#endif

ULong PilotMain(UShort cmd, MemPtr cmdPBP, UShort launchFlags) {
  Err err = sysErrParamErr;
 
  if (cmd == sysAppLaunchCmdNormalLaunch) {
    PtelnetNewGlobals();
    if ((err = StartApplication()) == 0) {
      EventLoop();
      StopApplication();
    }
    PtelnetFreeGlobals();
  }
 
  return err;
}

static Err StartApplication(void) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  Word ROMVerMajor, ROMVerMinor;
  DWord dwVersion;
  Int i, j;
  Err err;

  RetrieveVersion(p->ptVersion);
  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &dwVersion);
  ROMVerMajor = sysGetROMVerMajor(dwVersion);
  ROMVerMinor = sysGetROMVerMinor(dwVersion);

  if ((10*ROMVerMajor + ROMVerMinor) < 35) {
    FrmCustomAlert(VersionAlert, p->ptVersion, "", "");
    return -1;
  }

  if ((err = SysLibFind(msgNetworkLibrary, &p->AppNetRefnum)) != 0) {
    ErrorDialog(msgNetworkLibrary, err);
    return -1;
  }

  p->numDevices = MAX_DEVICES;
  if (SerialList(&p->deviceNames[0], p->deviceCreators, &p->numDevices) != 0) {
    return -1;
  }
  for (i = 0; i < p->numDevices; i++) {
    p->dnames[i] = p->deviceNames[i];
  }

  p->density = kDensityLow;
  if (FtrGet(sysFtrCreator, sysFtrNumWinVersion, &dwVersion) == 0 && dwVersion >= 4) {
    WinScreenGetAttribute(winScreenDensity, &p->density);
  }

  // zera tudo e depois preenche so' o que for preciso
  MemSet(&prefs, sizeof(prefs), 0);

  if (!PrefGetAppPreferencesV10(AppID, PREFS, &prefs, sizeof(prefs))) {
    prefs.physMode = telnet;
    prefs.serialPort = 0;
    StrCopy(prefs.Host, "127.0.0.1");
    prefs.Port = 23;
    prefs.lookupTimeout = 10;
    prefs.connectTimeout = 10;
    prefs.Font = 1;
    prefs.Cr = 1;
    prefs.Echo = 0;
    prefs.Bits = 8;
    prefs.Parity = 0;
    prefs.StopBits = 1;
    prefs.XonXoff = 0;
    prefs.RtsCts = 1;
    prefs.Baud = 9600;
    prefs.Log = 0;
    prefs.LogCategory = 0;
    prefs.LogHeader = 0;
    prefs.LogMode = 0;
    for (i = 0; i < numMacros; i++) {
      StrPrintF(prefs.MacroName[i], "Macro%d", i+1);
    }
    for (i = 0; i < numMacros; i++) {
      for (j = 0; j < tamMacro; j++) {
        prefs.MacroDef[i][j] = '\0';
      }
    }
    prefs.density = kDensityLow;
    if (FtrGet(sysFtrCreator, sysFtrNumWinVersion, &dwVersion) == 0 && dwVersion >= 4) {
      WinScreenGetAttribute(winScreenDensity, &prefs.density);
    }
    PrefSetAppPreferencesV10(AppID, PREFS, &prefs, sizeof(prefs));
  }

  if (prefs.density > p->density) {
    prefs.density = p->density;
    PrefSetAppPreferencesV10(AppID, PREFS, &prefs, sizeof(prefs));
  }

  InitFonts();

  p->ticksPerSecond = SysTicksPerSecond();
  p->wait = evtWaitForever;
  p->online = false;
  p->controlState = false;
  p->iPaste = p->nPaste = 0;
  p->bufPaste = NULL;
  p->pastingMemo  = true;
  p->dbRef = NULL;

  if (prefs.Log) {
    p->dbRef = OpenLog(0);
    prefs.Log = p->dbRef ? 1 : 0;
  }
  StrPrintF(p->logHeader, "%s log - ", AppName);
  LogSetHeader(prefs.LogHeader ? p->logHeader : NULL);
  LogSetMode(prefs.LogMode);

  FrmGotoForm(MainForm);
  return 0;
}

static void EventLoop(void) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  EventType event;
  Err err;
 
  do {
    EvtGetEvent(&event, p->wait);
    if (HardbuttonHandleEvent(&event)) continue;
    if (SysHandleEvent(&event)) continue;
    if (MenuHandleEvent(NULL, &event, &err)) continue;
    if (ApplicationHandleEvent(&event)) continue;

    FrmDispatchEvent(&event);

  } while (event.eType != appStopEvent);
}

static void StopApplication(void) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();

  if (p->online)
    PhysOffline();

  if (p->bufPaste) {
    if (p->pastingMemo) {
      MemoCloseListRec(p->dbRef, p->indexPaste, p->bufPaste);
    }
  }

  CloseScreen();
  CloseTN3270();

  if (prefs.Log) {
    CloseLog(p->dbRef);
  } else {
    MemoClose(p->dbRef);
  }

  FrmSaveAllForms();
  FrmCloseAllForms();
}

static void ReturnToMainForm(void) {
  FrmReturnToForm(MainForm);
}

static Err SendTerminal(UInt16 c) {
  Err err;

  if (prefs.physMode == tn3270) {
    err = SendTN3270(c);
  } else {
    err = Send(c);
  }

  return err;
}

static Boolean HardbuttonHandleEvent(EventPtr event) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  Boolean handled;

  handled = false;

  if (p->online && !p->bufPaste && event->eType == keyDownEvent &&
      (event->data.keyDown.modifiers &
       commandKeyMask))
    switch (event->data.keyDown.chr) {
      case hard1Chr:
      case hard2Chr:
      case hard3Chr:
      case hard4Chr:
      case pageUpChr:
      case pageDownChr:
        if (FrmGetActiveFormID() == MainForm) {
          SendTerminal(event->data.keyDown.chr);
          handled = true;
        }
    }

  return handled;
}

static Boolean ApplicationHandleEvent(EventPtr event) {
  FormPtr frm;
  Word formId;
  Boolean handled;

  handled = false;

  if (event->eType == frmLoadEvent) {
    formId = event->data.frmLoad.formID;
    frm = FrmInitForm(formId);
    FrmSetActiveForm(frm);

    switch(formId) {
      case MainForm:
        FrmSetEventHandler(frm, MainFormHandleEvent);
        break;
      case KbdForm:
        FrmSetEventHandler(frm, KbdFormHandleEvent);
        break;
      case KbdVT100Form:
        FrmSetEventHandler(frm, KbdVT100FormHandleEvent);
        break;
      case Kbd3270Form:
        FrmSetEventHandler(frm, Kbd3270FormHandleEvent);
        break;
      case NetworkForm:
        FrmSetEventHandler(frm, NetworkFormHandleEvent);
        break;
      case SerialForm:
        FrmSetEventHandler(frm, SerialFormHandleEvent);
        break;
      case TerminalForm:
        FrmSetEventHandler(frm, TerminalFormHandleEvent);
        break;
      case MacrosForm:
        FrmSetEventHandler(frm, MacrosFormHandleEvent);
        break;
      case LogForm:
        FrmSetEventHandler(frm, LogFormHandleEvent);
        break;
      case PasteMForm:
        FrmSetEventHandler(frm, PasteMFormHandleEvent);
        break;
      case AboutForm:
        FrmSetEventHandler(frm, AboutFormHandleEvent);
    }
    handled = true;
  }

  return handled;
}

static void AlignBottom(FormPtr frm, UInt16 objIndex, UInt32 sheight) {
  RectangleType rect;
  Int16 d;

  FrmGetObjectBounds(frm, objIndex, &rect);
  d = sheight - 160;
  if (d > 0) {
    rect.topLeft.y += d;
    FrmSetObjectBounds(frm, objIndex, &rect);
  }
}

static void AlignControls(FormPtr frm) {
  UInt32 sheight;

  WinScreenMode(winScreenModeGet, NULL, &sheight, NULL, NULL);

  AlignBottom(frm, FrmGetObjectIndex(frm, onlineCtl), sheight);
  AlignBottom(frm, FrmGetObjectIndex(frm, controlCtl), sheight);
  AlignBottom(frm, FrmGetObjectIndex(frm, escCtl), sheight);
  AlignBottom(frm, FrmGetObjectIndex(frm, kbdCtl), sheight);
  AlignBottom(frm, FrmGetObjectIndex(frm, padCtl), sheight);
  AlignBottom(frm, FrmGetObjectIndex(frm, macrosCtl), sheight);
  AlignBottom(frm, FrmGetObjectIndex(frm, macroList), sheight);
}

#define AlphaSmartSysFtrID 'WTAP'
#define ScrnFtrNum 2

static void ResizeForm(FormPtr frm) {
  UInt32 version, swidth, sheight;
  RectangleType rect;

  if (FtrGet(AlphaSmartSysFtrID, ScrnFtrNum, &version) == 0) {
    WinScreenMode(winScreenModeGet, &swidth, &sheight, NULL, NULL);
    rect.topLeft.x = 0;
    rect.topLeft.y = 0;
    rect.extent.x = swidth;
    rect.extent.y = sheight;
    WinSetWindowBounds(FrmGetWindowHandle(frm), &rect);
  }
}

static Boolean MainFormHandleEvent(EventPtr event) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  FormPtr frm;
  ListPtr list;
  CharPtr text;
  Boolean handled;
  Short id, value;
  Word index;
  Int16 col, row;
  Int i, k;

  handled = false;

  switch (event->eType) {
    case winDisplayChangedEvent:
      frm = FrmGetActiveForm();
      AlignControls(frm);
      handled = true;
      break;

    case frmOpenEvent:
      frm = FrmGetActiveForm();
      ResizeForm(frm);
      FrmDrawForm(frm);
      FrmSetControlValue(frm, FrmGetObjectIndex(frm, onlineCtl), p->online);

      list = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, macroList));
      for (i = 0; i < numMacros; i++) {
        text = LstGetSelectionText(list, i);
        StrCopy(text, prefs.MacroName[i]);
      }
      LstSetSelection(list, 0);

      DrawSeparator();
      InitScreen(prefs.Font, prefs.density, prefs.physMode == tn3270 ? 80 : 0);
      InitVT100(GetRows(), GetCols());
      Init0TN3270(GetRows(), GetCols());
      handled = true;
      break;

    case nilEvent:
      if (p->online) {
        if (Receive() == -1) {
          PhysOffline();
          frm = FrmGetActiveForm();
          FrmSetControlValue(frm, FrmGetObjectIndex(frm, onlineCtl), 0);
        } else {
          if (prefs.physMode != tn3270) {
            Cursor(-1);
          }
        }
      }
      if (p->bufPaste) {
        if (!p->nPaste) {
          if (p->pastingMemo) {
            MemoCloseListRec(p->dbRef, p->indexPaste, p->bufPaste);
            p->bufPaste = NULL;
            if (!p->online)
            p->wait = p->online ? p->ticksPerSecond / 25 : evtWaitForever;
            if (!prefs.Log) {
              MemoClose(p->dbRef);
              p->dbRef = NULL;
            }
          }
        }
        if (p->nPaste) {
          MemSet(p->buf, CHUNK+1, 0);
          k = CHUNK < p->nPaste ? CHUNK : p->nPaste;
          MemMove(p->buf, &p->bufPaste[p->iPaste], k);
          SendString(p->buf);
          p->iPaste += k;
          p->nPaste -= k;
        }
      }
      handled = true;
      break;

    case penDownEvent:
      if (p->online && prefs.physMode == tn3270) {
        col = event->screenX / (FontWidth() / 2);
        row = event->screenY / (FontHeight() / 2);
        PositionTN3270(col, row);
      }
      handled = false;
      break;

    case menuEvent:
      switch (id = event->data.menu.itemID) {
        case NetworkCmd:
        case SerialCmd:
        case TerminalCmd:
        case MacrosCmd:
        case PasteMCmd:
          FrmPopupForm(id);
          break;
        case LogCmd:
          if (!p->bufPaste)
            FrmPopupForm(id);
          break;
        case AboutCmd:
          //FrmPopupForm(id);
          frm = FrmInitForm(id);
          FrmDoDialog(frm);
          FrmDeleteForm(frm);
          break;
      }
      handled = true;
      break;

    case keyDownEvent:
      if (!p->bufPaste && !(event->data.keyDown.modifiers & commandKeyMask)) {
        SendTerminal(event->data.keyDown.chr);
        handled = true;
      }
      break;

    case ctlSelectEvent:
      switch (event->data.ctlSelect.controlID) {
        case onlineCtl:
          frm = FrmGetActiveForm();
          index = FrmGetObjectIndex(frm, onlineCtl);
          if (p->online) {
            PhysOffline();
            value = 0;
          } else {
            value = PhysOnline() == 0 ? 1 : 0;
          }
          FrmSetControlValue(frm, index, value);
          handled = true;
          break;
        case controlCtl:
          p->controlState = !p->controlState;
          handled = true;
          break;
        case escCtl:
          frm = FrmGetActiveForm();
          FrmSetControlValue(frm, FrmGetObjectIndex(frm, escCtl), 0);
          if (!p->bufPaste)
            Send(ESC);
          handled = true;
          break;
        case kbdCtl:
          if (!p->bufPaste) {
            FrmPopupForm(KbdForm);
          }
          handled = true;
          break;
        case padCtl:
          if (!p->bufPaste) {
            if (prefs.physMode == tn3270) {
              FrmPopupForm(Kbd3270Form);
            } else {
              FrmPopupForm(KbdVT100Form);
            }
          }
          handled = true;
          break;
      }
      break;

    case popSelectEvent:
      switch (event->data.popSelect.listID) {
        case macroList:
          if (p->online) {
            i = event->data.popSelect.selection;
            if (!p->bufPaste && i >= 0 && i < numMacros)
              SendString(prefs.MacroDef[i]);
          }
      }
      handled = true;
      break;

    default:
      break;
  }

  return handled;
}

static Boolean KbdFormHandleEvent(EventPtr event) {
  FormPtr frm, frmM;
  FieldPtr fld;
  Char aux[64];
  Int i;
  Boolean handled;

  handled = false;

  switch (event->eType) {
    case frmOpenEvent:
      frmM = FrmGetFormPtr(MainForm);
      frm = FrmGetActiveForm();
      i = FrmGetObjectIndex(frm, kbdFld);
      fld = (FieldPtr)FrmGetObjectPtr(frm, i);
      FrmDrawForm(frm);
      FrmSetFocus(frm, i);
      SysKeyboardDialog(kbdDefault);
      aux[0] = '\0';
      if (FldGetTextPtr(fld))
        StrCopy(aux, FldGetTextPtr(fld));
      ReturnToMainForm();
      FrmSetControlValue(frmM, FrmGetObjectIndex(frmM, kbdCtl), 0);
      if (aux[0])
        SendString(aux);
      handled = true;

    default:
      break;
  }

  return handled;
}

static UInt8 utf_state = 0;

static void EmitVT100Aux(Char *s, Int n) {
  Int16 i;
  UInt8 b;

  for (i = 0; i < n; i++) {
    b = s[i];

    switch (utf_state) {
      case 0:
        if (b < 0x80) {
          EmitVT100(&b, 1, 1);
        } else if (b >= 0xC0) {
          utf_state = b;
        }
        break;
      case 0xC2:
        if (b >= 0x80 && b < 0xC0) {
          EmitVT100(&b, 1, 1);
        }
        utf_state = 0;
        break;
      case 0xC3:
        if (b >= 0x80 && b < 0xC0) {
          b += 0x40;
          EmitVT100(&b, 1, 1);
        }
        utf_state = 0;
        break;
      default:
        utf_state = 0;
        break;
    }
  }
}

static Boolean KbdVT100FormHandleEvent(EventPtr event) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  FormPtr frm;
  UShort code;
  Boolean handled;
  UChar c;
  Char *s;
  Short n;

  handled = false;

  switch (event->eType) {

    case frmOpenEvent:
      frm = FrmGetActiveForm();
      FrmDrawForm(frm);
      handled = true;
      break;

    case ctlSelectEvent:
      switch (code = event->data.ctlSelect.controlID) {
        case cancelBtn:
          CloseKeypad();
          handled = true;
          break;
        case keyDel:
          CloseKeypad();
          Send(DEL);
          break;
        case keyBrk:
          CloseKeypad();
          if (p->online)
            PhysBreak();
          break;
        default:
          c = code & 0x00ff;
          CloseKeypad();
          s = GetKeySeq(c);
          n = StrLen(s);
          if (prefs.Echo)
            EmitVT100Aux(s, n);
          handled = true;
      }
      break;

    default:
      break;
  }

  return handled;
}

static Boolean Kbd3270FormHandleEvent(EventPtr event) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  FormPtr frm;
  UShort code;
  Boolean handled;

  handled = false;

  switch (event->eType) {
    case frmOpenEvent:
      frm = FrmGetActiveForm();
      FrmDrawForm(frm);
      handled = true;
      break;

    case ctlSelectEvent:
      switch (code = event->data.ctlSelect.controlID) {
        case cancelBtn:
          CloseKeypad();
          handled = true;
          break;
        case keyLTAB:
        case keyRTAB:
        case keyHOME:
          CloseKeypad();
          SendSpecial(code);
          handled = true;
          break;
        default:
          CloseKeypad();
          code &= 0x00ff;
          if (p->online)
            SendAID(code);
          handled = true;
      }

    default:
      break;
  }

  return handled;
}

static void CloseKeypad() {
  FormPtr frmM;

  ReturnToMainForm();
  frmM = FrmGetFormPtr(MainForm);
  FrmSetControlValue(frmM, FrmGetObjectIndex(frmM, padCtl), 0);
}

static Boolean NetworkFormHandleEvent(EventPtr event) {
  FormPtr frm;
  FieldPtr fld;
  Boolean handled;

  handled = false;

  switch (event->eType) {

    case frmOpenEvent:
      CopyPrefs();
      frm = FrmGetActiveForm();
      FldInsertStr((VoidPtr)frm, hostFld, prefs_aux.Host);
      FldInsertDec((VoidPtr)frm, portFld, prefs_aux.Port);
      FldInsertDec((VoidPtr)frm, ltimeFld, prefs_aux.lookupTimeout);
      FldInsertDec((VoidPtr)frm, ctimeFld, prefs_aux.connectTimeout);
      FrmDrawForm(frm);
      FrmSetFocus(frm, FrmGetObjectIndex(frm, hostFld));
      handled = true;
      break;

    case ctlSelectEvent:
      switch (event->data.ctlSelect.controlID) {
        case okBtn:
          frm = FrmGetActiveForm();
          fld = (FieldPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, hostFld));
          StrCopy(prefs_aux.Host, FldGetTextPtr(fld));
          fld = (FieldPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, portFld));
          prefs_aux.Port = StrAToI(FldGetTextPtr(fld));
          fld = (FieldPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, ltimeFld));
          prefs_aux.lookupTimeout = StrAToI(FldGetTextPtr(fld));
          if (!prefs_aux.lookupTimeout)
            prefs_aux.lookupTimeout = 1;
          fld = (FieldPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, ctimeFld));
          prefs_aux.connectTimeout = StrAToI(FldGetTextPtr(fld));
          if (!prefs_aux.connectTimeout)
            prefs_aux.connectTimeout = 1;
          SavePrefs();
          // fall-through
        case cancelBtn:
          ReturnToMainForm();
          handled = true;
      }
      break;

    default:
      break;
  }

  return handled;
}

static Boolean SerialFormHandleEvent(EventPtr event) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  FormPtr frm;
  ListPtr list;
  UInt16 objIndex;
  Short id;
  Int i;   
  Boolean handled;

  handled = false;
      
  switch (event->eType) {
    case frmOpenEvent:
      CopyPrefs();
      frm = FrmGetActiveForm();
      switch (prefs_aux.Baud) {
        case 300:    i = 0; break;
        case 1200:   i = 1; break;
        case 2400:   i = 2; break;
        case 4800:   i = 3; break;
        case 9600:   i = 4; break;
        case 19200:  i = 5; break;
        case 38400:  i = 6; break;
        case 57600:  i = 7; break;
        case 115200: i = 8; break;
        default:     i = -1;
      }
      if (i != -1) {
        list = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, baudList));
        LstSetSelection(list, i);
        CtlSetLabel((ControlPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, baudCtl)), LstGetSelectionText(list, i));
      }

      objIndex = FrmGetObjectIndex(frm, portList);
      list = (ListPtr)FrmGetObjectPtr(frm, objIndex);
      LstSetHeight(list, p->numDevices);
      LstSetListChoices(list, p->dnames, p->numDevices);
      LstSetSelection(list, prefs_aux.serialPort);
      CtlSetLabel((ControlPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, portCtl)), LstGetSelectionText(list, prefs_aux.serialPort));

      FrmSetControlValue(frm, FrmGetObjectIndex(frm, paritySelNone + prefs_aux.Parity), 1);
      FrmSetControlValue(frm, FrmGetObjectIndex(frm, bitsSel5 + prefs_aux.Bits - 5), 1);
      FrmSetControlValue(frm, FrmGetObjectIndex(frm, stopBitsSel1 + prefs_aux.StopBits - 1), 1);
      FrmSetControlValue(frm, FrmGetObjectIndex(frm, xonXoffCtl), prefs_aux.XonXoff);
      FrmSetControlValue(frm, FrmGetObjectIndex(frm, rtsCtsCtl), prefs_aux.RtsCts);
      FrmDrawForm(frm);
      handled = true;
      break;

    case ctlSelectEvent:
      switch (id = event->data.ctlSelect.controlID) {
        case okBtn:
          SavePrefs();
          // fall-through
        case cancelBtn:
          ReturnToMainForm();
          handled = true;
          break;
        case paritySelNone:
        case paritySelEven:
        case paritySelOdd:
          prefs_aux.Parity = id - paritySelNone;
          handled = true;
          break;
        case bitsSel5:
        case bitsSel6:
        case bitsSel7:
        case bitsSel8:
          prefs_aux.Bits = id - bitsSel5 + 5;
          handled = true;
          break;
        case stopBitsSel1:
        case stopBitsSel2:
          prefs_aux.StopBits = id - stopBitsSel1 + 1;
          handled = true;
          break;
        case xonXoffCtl:
          prefs_aux.XonXoff = 1-prefs_aux.XonXoff;
          handled = true;
          break;
        case rtsCtsCtl:
          prefs_aux.RtsCts = 1-prefs_aux.RtsCts;
          handled = true;
          break;
      }
      break;

    case popSelectEvent: 
      switch (event->data.popSelect.listID) {
        case baudList:
          switch (event->data.popSelect.selection) {
            case 0: prefs_aux.Baud = 300; break;
            case 1: prefs_aux.Baud = 1200; break;
            case 2: prefs_aux.Baud = 2400; break;
            case 3: prefs_aux.Baud = 4800; break;
            case 4: prefs_aux.Baud = 9600; break;
            case 5: prefs_aux.Baud = 19200; break;
            case 6: prefs_aux.Baud = 38400; break;
            case 7: prefs_aux.Baud = 57600; break;
            case 8: prefs_aux.Baud = 115200; break;
          }
          handled = false;
      } 
      break;

    default:
      break;
  }

  return handled;
}

static Boolean TerminalFormHandleEvent(EventPtr event) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  FormPtr frm;
  Short id;
  Boolean handled;

  handled = false;
        
  switch (event->eType) {
    case frmOpenEvent:
      CopyPrefs();
      frm = FrmGetActiveForm();
      FrmSetControlValue(frm, FrmGetObjectIndex(frm, serialSel + prefs_aux.physMode), 1);
      FrmSetControlValue(frm, FrmGetObjectIndex(frm, echoCtl), prefs_aux.Echo);
      FrmSetControlValue(frm, FrmGetObjectIndex(frm, crSel + prefs_aux.Cr), 1);

      switch (prefs_aux.Font) {
        case 0:
          FrmSetControlValue(frm, FrmGetObjectIndex(frm, sel4x6), 1);
          FrmSetControlValue(frm, FrmGetObjectIndex(frm, hsel6x6), 1);
          break;
        case 1:
          FrmSetControlValue(frm, FrmGetObjectIndex(frm, sel5x9), 1);
          FrmSetControlValue(frm, FrmGetObjectIndex(frm, hsel6x10), 1);
          break;
        case 2:
          FrmSetControlValue(frm, FrmGetObjectIndex(frm, sel6x6), 1);
          FrmSetControlValue(frm, FrmGetObjectIndex(frm, hsel8x14), 1);
          break;
      }

      if (p->density < kDensityDouble) {
        FrmHideObject(frm, FrmGetObjectIndex(frm, hsel6x6));
        FrmHideObject(frm, FrmGetObjectIndex(frm, hsel6x10));
        FrmHideObject(frm, FrmGetObjectIndex(frm, hsel8x14));
      } else {
        FrmHideObject(frm, FrmGetObjectIndex(frm, sel4x6));
        FrmHideObject(frm, FrmGetObjectIndex(frm, sel5x9));
        FrmHideObject(frm, FrmGetObjectIndex(frm, sel6x6));
      }

      FrmDrawForm(frm);
      p->fontChanged = false;
      handled = true;
      break;

    case ctlSelectEvent:
      switch (id = event->data.ctlSelect.controlID) {
        case echoCtl:
          prefs_aux.Echo ^= 1;
          handled = true;
          break;
        case crSel:
        case lfSel:
        case crlfSel:
          prefs_aux.Cr = id - crSel;
          handled = true;
          break;
        case sel4x6:
        case hsel6x6:
          prefs_aux.Font = 0;
          handled = true;
          break;
        case sel5x9:
        case hsel6x10:
          prefs_aux.Font = 1;
          handled = true;
          break;
        case sel6x6:
        case hsel8x14:
          prefs_aux.Font = 2;
          handled = true;
          break;
        case serialSel:
          prefs_aux.physMode = serial;
          handled = true;
          break;
        case telnetSel:
          prefs_aux.physMode = telnet;
          handled = true;
          break;
        case tn3270Sel:
          prefs_aux.physMode = tn3270;
          handled = true;
          break;
        case okBtn:
          if (prefs_aux.Font != prefs.Font || prefs_aux.density != prefs.density) {
            p->fontChanged = true;
          }
          SavePrefs();
          // fall-through
        case cancelBtn:
          ReturnToMainForm();
          if (p->fontChanged) {
            InitScreen(prefs.Font, prefs.density, prefs.physMode == tn3270 ? 80 : 0);
            InitVT100(GetRows(), GetCols());
            InitTN3270(GetRows(), GetCols());
            ClearScreen();
            Home();
          }
          handled = true;
      }
      break;

    default:
      break;
  }

  return handled;
}

static Boolean MacrosFormHandleEvent(EventPtr event)
{
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  FormPtr frm, frmM;
  ListPtr list;
  ControlPtr ctl;
  Boolean handled;
  Word index;

  handled = false;
        
  switch (event->eType) {
    case frmOpenEvent:
      CopyPrefs();
      frm = FrmGetActiveForm();
      MacrosToFields(frm, &prefs_aux, 0);
      index = FrmGetObjectIndex(frm, upBtn);
      ctl = (ControlPtr)FrmGetObjectPtr(frm, index);
      CtlSetEnabled(ctl, false);
      p->iMacro = 0;
      FrmDrawForm(frm);
      FrmSetFocus(frm, FrmGetObjectIndex(frm, macroName));
      handled = true;
      break;

    case ctlSelectEvent:
      switch (event->data.ctlSelect.controlID) {
        case okBtn:
          frm = FrmGetActiveForm();
          frmM = FrmGetFormPtr(MainForm);
          index = FrmGetObjectIndex(frmM, macroList);
          list = (ListPtr)FrmGetObjectPtr(frmM, index);
          FieldsToMacros(frm, &prefs_aux, p->iMacro);
          MacrosToList(list, &prefs_aux);
          SavePrefs();
          // fall-through
        case cancelBtn:
          ReturnToMainForm();
          handled = true;
          break;

        case upBtn:
          frm = FrmGetActiveForm();

          index = FrmGetObjectIndex(frm, upBtn);
          ctl = (ControlPtr)FrmGetObjectPtr(frm, index);
          CtlSetLabel(ctl, "\003");
          CtlSetEnabled(ctl, false);

          index = FrmGetObjectIndex(frm, downBtn);
          ctl = (ControlPtr)FrmGetObjectPtr(frm, index);
          CtlSetLabel(ctl, "\002");
          CtlSetEnabled(ctl, true);

          p->iMacro = 0;
          FieldsToMacros(frm, &prefs_aux, numMacros/2);
          MacrosToFields(frm, &prefs_aux, 0);

          index = FrmGetObjectIndex(frm, macroName);
          FrmSetFocus(frm, index);

          handled = true;
          break;

        case downBtn:
          frm = FrmGetActiveForm();

          index = FrmGetObjectIndex(frm, downBtn);
          ctl = (ControlPtr)FrmGetObjectPtr(frm, index);
          CtlSetLabel(ctl, "\004");
          CtlSetEnabled(ctl, false);

          index = FrmGetObjectIndex(frm, upBtn);
          ctl = (ControlPtr)FrmGetObjectPtr(frm, index);
          CtlSetLabel(ctl, "\001");
          CtlSetEnabled(ctl, true);

          p->iMacro = numMacros/2;
          FieldsToMacros(frm, &prefs_aux, 0);
          MacrosToFields(frm, &prefs_aux, numMacros/2);

          index = FrmGetObjectIndex(frm, macroName);
          FrmSetFocus(frm, index);

          handled = true;
      }
      break;

    default:
      break;
  }

  return handled;
}

static Boolean LogFormHandleEvent(EventPtr event)
{
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  FormPtr frm;
  Boolean handled;

  handled = false;

  switch (event->eType) {
    case frmOpenEvent:
      CopyPrefs();
      frm = FrmGetActiveForm();
      FrmSetControlValue(frm, FrmGetObjectIndex(frm, logCtl), prefs_aux.Log);
      FrmSetControlValue(frm,
        FrmGetObjectIndex(frm, logHeaderCtl), prefs_aux.LogHeader);
      FrmSetControlValue(frm,
        FrmGetObjectIndex(frm, logModeCtl), prefs_aux.LogMode);
      FrmDrawForm(frm);
      handled = true;
      break;

    case ctlSelectEvent:
      switch (event->data.ctlSelect.controlID) {
        case okBtn:
          if (prefs_aux.Log != prefs.Log) {
            if (prefs_aux.Log) {
              p->dbRef = OpenLog(0);
              prefs_aux.Log = p->dbRef ? 1 : 0;
            }
            else {
              CloseLog(p->dbRef);
              p->dbRef = NULL;
            }
          }
          StrPrintF(p->logHeader, "%s Log - ", AppName);
          LogSetHeader(prefs_aux.LogHeader ? p->logHeader : NULL);
          LogSetMode(prefs_aux.LogMode);
          SavePrefs();
          // fall-through
        case cancelBtn:
          ReturnToMainForm();
          handled = true;
          break;
        case logCtl:
          prefs_aux.Log = !prefs_aux.Log;
          handled = true;
          break;
        case logHeaderCtl:
          prefs_aux.LogHeader = !prefs_aux.LogHeader;
          handled = true;
          break;
        case logModeCtl:
          prefs_aux.LogMode = !prefs_aux.LogMode;
          handled = true;
      }
      break;

    default:
      break;
  }

  return handled;
}

static Boolean PasteMFormHandleEvent(EventPtr event)
{
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  FormPtr frm;
  Boolean handled;
  static ListPtr list;
  static Int numRecs;

  handled = false;

  switch (event->eType) {
    case frmOpenEvent:
      frm = FrmGetActiveForm();
      numRecs = 0;
      list = (ListPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, memoList));
      if (p->dbRef || (p->dbRef = MemoOpen(dmModeReadWrite)) != NULL)
        numRecs = MemoGetList(p->dbRef, list);
      FrmDrawForm(frm);
      handled = true;
      break;

    case ctlSelectEvent:
      switch (event->data.ctlSelect.controlID) {
        case okBtn:
          if (p->dbRef && numRecs) {
            p->indexPaste = LstGetSelection(list);
            p->bufPaste = MemoOpenListRec(p->dbRef, p->indexPaste);
            p->nPaste = p->iPaste = 0;
            if (p->bufPaste) {
              p->wait = 0;
              p->nPaste = StrLen(p->bufPaste);
              p->pastingMemo = true;
            }
          }
          MemoFreeList();
          ReturnToMainForm();
          handled = true;
          break;
        case cancelBtn:
          if (p->dbRef) {
            MemoFreeList();
            p->bufPaste = NULL;
            if (!prefs.Log) {
              MemoClose(p->dbRef);
              p->dbRef = NULL;
            }
          }
          ReturnToMainForm();
          handled = true;
      }
      break;

    default:
      break;
  }

  return handled;
}

static Boolean AboutFormHandleEvent(EventPtr event) {
  FormPtr frm;
  Boolean handled;

  handled = false;

  switch (event->eType) {
    case frmOpenEvent:
      frm = FrmGetActiveForm();
      FrmDrawForm(frm);
      handled = true;
      break;

    case ctlSelectEvent:
      switch (event->data.ctlSelect.controlID) {
        case okBtn:
          ReturnToMainForm();
          handled = true;
      }
      break;

    default:
      break;
  }

  return handled;
}

static void RetrieveVersion(Char *ptVersion)
{
  VoidHand h;
  CharPtr s;

  if ((h = DmGetResource(verRsc, appVersionID)) != NULL) {
    if ((s = MemHandleLock(h)) != NULL) {
      StrCopy(ptVersion, s);
      MemHandleUnlock(h);
    }
    else
      StrCopy(ptVersion, "?.?");
    DmReleaseResource(h);
  } 
  else
    StrCopy(ptVersion, "?.?");
}

static Err PhysOnline(void) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  Err err;

  switch (prefs.physMode) {
    case serial:
      err = SerialOnline(&p->AppSerRefnum, prefs.Baud, prefs.Bits, prefs.Parity, prefs.StopBits, prefs.XonXoff, prefs.RtsCts, prefs.RtsCts, prefs.serialPort);
      InitVT100(GetRows(), GetCols());
      break;
    case telnet:
      err = NetworkOnline(p->AppNetRefnum, prefs.Host, prefs.Port, prefs.lookupTimeout, prefs.connectTimeout);
      InitTelnet("vt100", GetRows(), GetCols());
      InitVT100(GetRows(), GetCols());
      break;
    case tn3270:
      /*
      The -2 following 3278 designates the alternate screen size.  3270
      terminals have the ability to switch between the standard (24x80)
      screen size and an alternate screen size.  Model -2 is 24x80 which
      is the same as the standard size.  Model -3 is 32x80, model -4 is
      43x80 and model -5 is 27x132.
      */
      err = NetworkOnline(p->AppNetRefnum, prefs.Host, prefs.Port, prefs.lookupTimeout, prefs.connectTimeout);
      InitTelnet("IBM-3279-2-E", GetRows(), GetCols());
      InitTN3270(GetRows(), GetCols());
      break;
    default:
      err = -1;
  }

  if (!err) {
    p->wait = p->ticksPerSecond / 25;
    p->online = true;
  }

  return err;
}

static void PhysOffline(void) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();

  switch (prefs.physMode) {
    case telnet:
      NetworkOffline(p->AppNetRefnum);
      break;
    case tn3270:
      NetworkOffline(p->AppNetRefnum);
      break;
    case serial:
      SerialOffline(p->AppSerRefnum);
      break;
  }

  p->wait = evtWaitForever;
  p->online = false;

  if (p->bufPaste) {
    if (p->pastingMemo) {
      MemoCloseListRec(p->dbRef, p->indexPaste, p->bufPaste);
      p->bufPaste = NULL;
      if (!prefs.Log) {
        MemoClose(p->dbRef);
        p->dbRef = NULL;
      }
    }
  }

  Cursor(0);
}

static Int Receive(void)
{
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  Err err;
  Short n;
  Word num;

  if (!p->online)
    return -1;

  if ((n = PhysReceive(p->inbuf, tamBuf, &err)) == -1) {
    if (err == netErrTimeout || err == serErrTimeOut)
      return 0;
    num = err == serErrLineErr ? SerialGetStatus(p->AppSerRefnum) : 0;
    ErrorDialogEx("Receive failed", err, num);
    return -1;
  }

  if (!n && prefs.physMode != serial) {
    /* Socket foi fechado remotamente */
    ErrorDialog("Connection closed", 0);
    return -1;
  }

  switch (prefs.physMode) {
    case serial:
      EmitVT100Aux((Char *)p->inbuf, n);
      break;
    case telnet:
      EmitTelnet(p->inbuf, n);
      break;
    case tn3270:
      EmitTelnet(p->inbuf, n);
      break;
  }

  if (prefs.Log)
    prefs.Log = LogBytes(p->dbRef, p->inbuf, n) == 0 ? 1 : 0;

  return n;
}

static Int PhysReceive(UChar *buf, Int tam, Err *err)
{
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  EvtResetAutoOffTimer();

  switch (prefs.physMode) {
    case telnet:
      return NetworkReceive(p->AppNetRefnum, buf, tam, err);
    case tn3270:
      return NetworkReceive(p->AppNetRefnum, buf, tam, err);
    case serial:
      return SerialReceive(p->AppSerRefnum, buf, tam, err);
  }

  *err = 0;
  return -1;
}

Int PhysSend(UChar *buf, Int tam, Err *err)
{
  PtelnetGlobalsType *p = PtelnetGetGlobals();

  switch (prefs.physMode) {
    case telnet:
      return NetworkSend(p->AppNetRefnum, buf, tam, err);
    case tn3270:
      return NetworkSend(p->AppNetRefnum, buf, tam, err);
    case serial:
      return SerialSend(p->AppSerRefnum, buf, tam, err);
  }

  *err = 0;
  return -1;
}

static Err PhysBreak()
{
  PtelnetGlobalsType *p = PtelnetGetGlobals();

  switch (prefs.physMode) {
    case telnet:
    case tn3270:
      return 0;
    case serial:
      return SerialBreak(p->AppSerRefnum);
  }

  return -1;
}

void EmitTerminal(Char *buf, Int n, Short done) {
  if (prefs.physMode == tn3270) {
    EmitTN3270(buf, n, done);
  } else {
    EmitVT100Aux(buf, n);
  }
}

static Err Send(UShort c1)
{
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  FormPtr frm;
  CharPtr s;
  Char c;
  Short n;
  Err err;

  s = NULL;

  switch (c1) {
    case pageUpChr:
      s = GetKeySeq(VT100_upArrow);
      break;
    case pageDownChr:
      s = GetKeySeq(VT100_downArrow);
      break;
    case hard2Chr:
      s = GetKeySeq(VT100_leftArrow);
      break;
    case hard3Chr:
      s = GetKeySeq(VT100_rightArrow);
      break;
    case hard4Chr:
      // fall-through
    case '\n':
    case '\r':
      switch (prefs.Cr) {
        case 0: s = "\r"; break;
        case 1: s = "\n"; break;
        case 2: s = "\r\n";
      }
      break;
    case hard1Chr:
      c1 = ' ';
      // fall-through
    default:
      c = c1;
      if (p->controlState) {
        c = ControlChar(c);
        frm = FrmGetFormPtr(MainForm);
        FrmSetControlValue(frm, FrmGetObjectIndex(frm, controlCtl), 0);
        p->controlState = false;
      }
      if (p->online && PhysSend((UChar *)&c, 1, &err) != 1)
        return -1;
      if (prefs.Echo) {
        EmitTerminal(&c, 1, 1);
      }

      return 0;
  }

  if (s) {
    n = StrLen(s);
    if (p->online && PhysSend((UChar *)s, n, &err) != n)
      return -1;
    if (prefs.Echo) {
      EmitTerminal(s, n, 1);
    }
  }

  return 0;
}

static Char ControlChar(Char c)
{
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 1;
  if (c >= '@' && c <= '_')
    return c - '@';
  return c;
}

static Err SendString(CharPtr s)
{
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  Char c;
  UChar hex;
  Int i, estado;

  for (i = 0, estado = 0, hex = 0; s[i]; i++) {
    c = s[i];
    switch (estado) {
      case 0:  // normal
        switch (c) {
          case '^':
            estado = 1;
            continue;
          case '\\':
            estado = 2;
            continue;
        }
        break;
      case 1:  // chapeu
        estado = 0;
        if (c == ',') {
          SysTaskDelay(p->ticksPerSecond);
          continue;
        }
        c = ControlChar(c);
        break;
      case 2:  // barra
        switch (c) {
          case 'r':
            SendTerminal('\n');
            estado = 0;
            continue;
          case 't':
            SendTerminal('\t');
            estado = 0;
            continue;
          case 'x':
            estado = 3;
            continue;
        }
        break;
      case 3:  // barra x, 1# digito
        hex = HexDigit(c);
        estado = 4;
        continue;
      case 4:  // barra x, 2# digito
        hex = (hex << 4) | HexDigit(c);
        estado = 0;
        c = hex;
    }

    if (SendTerminal(c) == -1)
      return -1;
  }

  return 0;
}

static UChar HexDigit(Char c)
{
  UChar digit = 0;

  if (c >= '0' && c <= '9') digit = c-'0';
  else if (c >= 'A' && c <= 'F') digit = c-'A'+10;
  else if (c >= 'a' && c <= 'f') digit = c-'a'+10;

  return digit;
}

static void CopyPrefs()
{
  MemMove(&prefs_aux, &prefs, sizeof(prefs_aux));
}

static void SavePrefs()
{
  MemMove(&prefs, &prefs_aux, sizeof(prefs));
  PrefSetAppPreferencesV10(AppID, PREFS, &prefs, sizeof(prefs));
}

static void MacrosToFields(FormPtr frm, PtelnetPreferencesType *p, Short i0)
{
  Int i;
  EventType e;

  for (i = numMacros/2 - 1; i >= 0; i--) {
    FldInsertStr((VoidPtr)frm, macroDef+i, p->MacroDef[i0+i]);
    FldInsertStr((VoidPtr)frm, macroName+i, p->MacroName[i0+i]);
    EvtGetEvent(&e, 0);
    if (e.eType != nilEvent)
      SysHandleEvent(&e);
  }
}

static void FieldsToMacros(FormPtr frm, PtelnetPreferencesType *p, Short i0)
{
  Int i;
  FieldPtr fld;
  CharPtr text;

  for (i = 0; i < numMacros/2; i++) {
    fld = (FieldPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, macroName+i));
    text = FldGetTextPtr(fld);
    if (text != NULL)
      StrNCopy(p->MacroName[i0+i], text, tamName-1);
    else
      p->MacroName[i0+i][0] = '\0';
    fld = (FieldPtr)FrmGetObjectPtr(frm, FrmGetObjectIndex(frm, macroDef+i));
    text = FldGetTextPtr(fld);
    if (text != NULL)
      StrNCopy(p->MacroDef[i0+i], text, tamMacro-1);
    else
      p->MacroDef[i0+i][0] = '\0';
  }
}

static void MacrosToList(ListPtr list, PtelnetPreferencesType *p)
{
  Int i;
  CharPtr text;

  for (i = 0; i < numMacros; i++) {
    text = LstGetSelectionText(list, i);
    StrCopy(text, p->MacroName[i]);
  }
}
