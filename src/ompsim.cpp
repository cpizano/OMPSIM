// OMPSIM.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "OMPSIM.h"


int APIENTRY _tWinMain(HINSTANCE instance,
                       HINSTANCE,
                       LPTSTR cmd_line,
                       int cmd_show) {
  MSG msg;

  while (GetMessage(&msg, NULL, 0, 0)) {
    DispatchMessage(&msg);
  }

  return (int) msg.wParam;
}

