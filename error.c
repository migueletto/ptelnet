#include <PalmOS.h>
#include <PalmCompatibility.h>

#include "const.h"
#include "error.h"

void ErrorDialog(CharPtr msg, Err err) {
  ErrorDialogEx(msg, err, 0);
}

void ErrorDialogEx(CharPtr msg, Err err, Word num) {
  Char cod[64], s[8];

  if (err) {
    StrPrintF(cod, "\nError code: %d", err);
    s[0] = 0;
    if (num) {
      StrPrintF(s, "(%x)", num);
    }
    FrmCustomAlert(ErrorAlert, msg, cod, s);
  } else {
    FrmCustomAlert(InfoAlert, msg, "", "");
  }
}
