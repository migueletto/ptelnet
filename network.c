#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "const.h"
#include "ptelnet.h"
#include "network.h"
#include "error.h"

Err NetworkOnline(UShort AppNetRefnum, Char *host, UShort port, UShort lookupTimeout, UShort connectTimeout) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  DWord dwVersion;
  Err err, ifErr;
  NetHostInfoPtr hinfop;
  UShort len;
  UInt8 aiu;	// allInterfacesUp

  FtrGet(sysFtrCreator, sysFtrNumROMVersion, &dwVersion);
  p->AppNetTimeout = p->ticksPerSecond/4;
  aiu = 0;

  if (((err = NetLibOpen(AppNetRefnum, &ifErr)) != 0 &&
       err != netErrAlreadyOpen) || ifErr) {
    if (ifErr != 0)
      err = ifErr;
    ErrorDialog("Could not open NetLib", err);
    return -1;
  }

  if (((err = NetLibConnectionRefresh(AppNetRefnum, true, &aiu, &ifErr)) != 0 &&
       err != netErrAlreadyOpen) || ifErr) {
    if (ifErr != 0)
      err = ifErr;
    NetLibClose(AppNetRefnum, true);
    ErrorDialog("Could not open NetLib", err);
    return -1;
  }

  if ((p->sock = NetLibSocketOpen(AppNetRefnum, netSocketAddrINET,
                netSocketTypeStream, 0, p->AppNetTimeout, &err)) == -1) {
    ErrorDialog("Socket creation failed", err);
    NetLibClose(AppNetRefnum, false);
    return -1;
  }

  len = sizeof(NetSocketAddrINType);
  p->addr.family = netSocketAddrINET;
  p->addr.port = NetHToNS(port);

  p->addr.addr = NetLibAddrAToIN(AppNetRefnum, host);
  if (p->addr.addr == -1) {
    if ((hinfop = NetLibGetHostByName(AppNetRefnum, host, &p->hinfo,
          lookupTimeout*p->ticksPerSecond, &err)) == 0) {
      ErrorDialog("DNS lookup failed", err);
      NetLibSocketClose(AppNetRefnum, p->sock, -1, &err);
      NetLibClose(AppNetRefnum, false);
      return -1;
    }
    p->addr.addr = p->hinfo.address[0];
    //addr.addr = NetHToNL(*((NetIPAddr *)(hinfop->addrListP[0])));
  } else {
  }

  if (NetLibSocketConnect(AppNetRefnum, p->sock, (NetSocketAddrType *)&p->addr, len,
        connectTimeout*p->ticksPerSecond, &err) == -1) {
    ErrorDialog("Connection failed", err);
    NetLibSocketClose(AppNetRefnum, p->sock, -1, &err);
    NetLibClose(AppNetRefnum, false);
    return -1;
  }

  return 0;
}

void NetworkOffline(UShort AppNetRefnum) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  Err err;

  NetLibSocketClose(AppNetRefnum, p->sock, -1, &err);
  NetLibClose(AppNetRefnum, false);
}

Int NetworkReceive(UShort AppNetRefnum, UChar *buf, Int tam, Err *err) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  return NetLibReceive(AppNetRefnum, p->sock, buf, tam, 0, 0, 0, p->AppNetTimeout, err);
}

Int NetworkSend(UShort AppNetRefnum, UChar *buf, Int tam, Err *err) {
  PtelnetGlobalsType *p = PtelnetGetGlobals();
  return NetLibSend(AppNetRefnum, p->sock, buf, tam, 0, 0, 0, p->AppNetTimeout, err);
}
