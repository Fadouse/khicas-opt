/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "giacPCH.h"
#include "usual.h"
#include "unary.h"
#include "ai_usb.h"
#include "usb_device.h"
#include "textGUI.hpp"
#include "graphicsProvider.hpp"

namespace giac {
  static void finish_keys() {
    // USB polling and the result viewer must not leak their EXIT/EXE press
    // into the next console action. A held key is consumed until release.
    int column, row;
    unsigned short key;
    do {
      GetKeyWait_OS(&column, &row, KEYWAIT_HALTOFF_TIMEROFF, 0, 1, &key);
    } while (usb_device_cancelled() || usb_device_exe());
    for (unsigned i = 0; i < 32; ++i)
      if (GetKeyWait_OS(&column, &row, KEYWAIT_HALTOFF_TIMEROFF, 0, 1, &key) != KEYREP_KEYEVENT)
        break;
  }
  gen _ai(const gen & arg, GIAC_CONTEXT) {
    if (arg.type == _STRNG && arg.subtype == -1)
      return arg;
    const std::string request = arg.type == _STRNG ? *arg._STRNGptr : arg.print(contextptr);
    if (request.empty() || request.size() > KHICAS_AI_REQUEST_MAX)
      return gensizeerr("ai: request must be 1..1024 bytes");
    char reply[KHICAS_AI_REPLY_MAX + 1];
    Bdisp_AllClr_VRAM();
    mPrintXY(1, 1, (char *)"AI over USB", 0, TEXT_COLOR_BLUE);
    mPrintXY(1, 3, (char *)"Waiting for host...", 0, TEXT_COLOR_BLACK);
    mPrintXY(1, 5, (char *)"EXIT: cancel", 0, TEXT_COLOR_BLACK);
    Bdisp_PutDisp_DD();
    int status = khicas_ai_exchange(request.c_str(), request.size(), reply);
    if (status == KHICAS_AI_CANCELLED) {
      // Nonblocking GetKeyWait reports the previous key during raw USB polling.
      // A bounded normal wait after restoring USB consumes the pending EXIT.
      int column, row;
      unsigned short key;
      GetKeyWait_OS(&column, &row, KEYWAIT_HALTON_TIMERON, 1, 1, &key);
    }
    finish_keys();
    if (status < 0)
      return string2gen(reply, false);
    textArea text;
    text.title = status ? "AI backend error" : "AI: brief solution";
    text.allowEXE = true;
    add(&text, std::string(reply));
    doTextArea(&text);
    finish_keys();
    return string2gen(reply, false);
  }
  static const char ai_s[] = "ai";
  static define_unary_function_eval_quoted(__ai, &_ai, ai_s);
  define_unary_function_ptr5(at_ai, alias_at_ai, &__ai, _QUOTE_ARGUMENTS, true);
} // namespace giac
