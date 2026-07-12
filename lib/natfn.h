#ifndef NATFN_H
#define NATFN_H
#include "function.h"

Value _nat_scan(VM*);
Value _nat_scan_ln(VM*);
Value _nat_scan_num(VM*);
Value _nat_clock(VM*);

void defineNativeFunctions(void*, void (*defineNative)(void*, const char*, int, NativeFn));

#endif