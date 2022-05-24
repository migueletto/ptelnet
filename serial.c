#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "const.h"
#include "ptelnet.h"
#include "serial.h"
#include "error.h"

#define MAX_CONNECTIONS	2
#define TAM_SERBUF	4096

static MemHandle BufferH[MAX_CONNECTIONS];
static MemPtr Buffer[MAX_CONNECTIONS];
static UInt16 refnum[MAX_CONNECTIONS];

static void CreatorToString(UInt32 creator, Char *s) {
  s[0] = '(';
  s[1] = (char)(creator >> 24);
  s[2] = (char)(creator >> 16);
  s[3] = (char)(creator >> 8);
  s[4] = (char)creator;
  s[5] = ')';
  s[6] = '\0';
}

void SerialInit(void)
{
  Int16 i;

  for (i = 0; i < MAX_CONNECTIONS; i++)
    refnum[i] = 0;
}

Err SerialList(char deviceNames[][MAX_DEVICENAME], UInt32 *deviceCreators, UInt16 *numDevices)
{
  DeviceInfoType dev;
  UInt16 num, i;
  Err err;

  if ((err = SrmGetDeviceCount(&num)) != 0) {
    ErrorDialog("Serial Manager error", err);
    return err;
  }

  if (num > *numDevices)
    num = *numDevices;

  for (i = 0; i < num; i++) {
    if ((err = SrmGetDeviceInfo(i, &dev)) != 0) {
      ErrorDialog("Serial Manager error", err);
      return err;
    }

    if (dev.serDevPortInfoStr[0]) {
      StrNCopy(deviceNames[i], dev.serDevPortInfoStr, MAX_DEVICENAME);
    } else {
      CreatorToString(dev.serDevCreator, deviceNames[i]);
    }
    deviceCreators[i] = dev.serDevCreator;
  }

  *numDevices = num;
  return 0;
}

Err SerialOnline(UInt16 *AppSerRefnum, UInt32 baud, UInt16 bits, UInt16 parity,
                 UInt16 stopBits, UInt16 xonXoff, UInt16 rts, UInt16 cts,
		 UInt32 device)
{
  UInt32 flags;
  UInt16 len;
  UInt32 value;
  Int16 i;
  Err err;

  for (i = 0; i < MAX_CONNECTIONS; i++)
    if (refnum[i] == 0)
      break;

  if (i == MAX_CONNECTIONS) {
    ErrorDialog("No more connections", 0);
    return -1;
  }

  if ((BufferH[i] = MemHandleNew(TAM_SERBUF+64)) == NULL) {
    ErrorDialog("Could not create handle", 0);
    return -1;
  }
 
  if ((Buffer[i] = MemHandleLock(BufferH[i])) == NULL) {
    MemHandleFree(BufferH[i]);
    ErrorDialog("Could not lock handle", 0);
    return -1;
  }

  err = SrmOpen(device, baud, AppSerRefnum);

  if (err != 0 && err != serErrAlreadyOpen) {
    MemHandleUnlock(BufferH[i]);
    MemHandleFree(BufferH[i]);
    ErrorDialog("Error opening the serial port", err);
    return -1;
  }
 
  flags = 0;
 
  if (xonXoff)
    flags |= srmSettingsFlagXonXoffM;
 
  if (rts) {
    flags |= srmSettingsFlagRTSAutoM;
    flags |= srmSettingsFlagFlowControlIn;
  }

  if (cts)
    flags |= srmSettingsFlagCTSAutoM;
 
  switch (bits) {
    case 5: flags |= srmSettingsFlagBitsPerChar5;
	    break;
    case 6: flags |= srmSettingsFlagBitsPerChar6;
	    break;
    case 7: flags |= srmSettingsFlagBitsPerChar7;
	    break;
    case 8: flags |= srmSettingsFlagBitsPerChar8;
  }
 
  flags |= srmSettingsFlagParityOnM;
 
  switch (parity) {
    case 0: flags &= ~srmSettingsFlagParityOnM;
	    break;
    case 1: flags |= srmSettingsFlagParityEvenM;
	    break;
    case 2: flags &= ~srmSettingsFlagParityEvenM;
  }
 
  switch (stopBits) {
    case 1: flags |= srmSettingsFlagStopBits1;
	    break;
    case 2: flags |= srmSettingsFlagStopBits2;
  }

  len = sizeof(flags);
  err = SrmControl(*AppSerRefnum, srmCtlSetFlags, &flags, &len);

  value = srmDefaultCTSTimeout;
  len = sizeof(value);
  if (err == 0)
    err = SrmControl(*AppSerRefnum, srmCtlSetCtsTimeout, &value, &len);

  if (err != 0) {
    MemHandleUnlock(BufferH[i]);
    MemHandleFree(BufferH[i]);
    SrmClose(*AppSerRefnum);
    ErrorDialog("Error configuring the serial port", err);
    return -1;
  }
 
  err = SrmSetReceiveBuffer(*AppSerRefnum, Buffer[i], TAM_SERBUF+64);

  if (err != 0) {
    MemHandleUnlock(BufferH[i]);
    MemHandleFree(BufferH[i]);
    SrmClose(*AppSerRefnum);
    ErrorDialog("Error setting the serial buffer", err);
    return -1;
  }
 
  refnum[i] = *AppSerRefnum;

  return 0;
}

void SerialOffline(UInt16 AppSerRefnum)
{
  Int16 i;

  for (i = 0; i < MAX_CONNECTIONS; i++)
    if (refnum[i] == AppSerRefnum)
      break;

  if (i == MAX_CONNECTIONS)
    return;

  refnum[i] = 0;

  SrmSetReceiveBuffer(AppSerRefnum, NULL, 0);
  SrmClose(AppSerRefnum);

  MemHandleUnlock(BufferH[i]);
  MemHandleFree(BufferH[i]);
}

Int16 SerialReceiveWait(UInt16 AppSerRefnum, UInt8 *buf, Int16 tam, Int32 wait, Err *err)
{
  UInt32 n;

  SrmClearErr(AppSerRefnum);
  if ((*err = SrmReceiveWait(AppSerRefnum, 1, wait)) != 0)
    return -1;
 
  SrmClearErr(AppSerRefnum);
  SrmReceiveCheck(AppSerRefnum, &n);
  if (n > tam)
    n = tam;
 
  SrmClearErr(AppSerRefnum);
  return SrmReceive(AppSerRefnum, buf, n, -1, err);
}

Int16 SerialReceive(UInt16 AppSerRefnum, UInt8 *buf, Int16 tam, Err *err)
{
  return SerialReceiveWait(AppSerRefnum, buf, tam, SysTicksPerSecond()/4, err);
}

Int16 SerialSend(UInt16 AppSerRefnum, UInt8 *buf, Int16 tam, Err *err)
{
  return SrmSend(AppSerRefnum, buf, tam, err);
}

UInt16 SerialGetStatus(UInt16 AppSerRefnum)
{
  UInt16 lineErrs;
  UInt32 status;

  return SrmGetStatus(AppSerRefnum, &status, &lineErrs) ? 0 : lineErrs;
}

Err SerialBreak(UInt16 AppSerRefnum)
{
  SrmControl(AppSerRefnum, srmCtlStartBreak, NULL, NULL);
  SysTaskDelay(3*SysTicksPerSecond()/10);	// 300 milissegundos
  SrmControl(AppSerRefnum, srmCtlStopBreak, NULL, NULL);

  return 0;
}
