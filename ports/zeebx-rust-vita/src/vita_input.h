#pragma once
#include "zeebx_bridge.h"
#include <psp2/ctrl.h>
void vita_input_init(void);
ZeebxVitaPad vita_input_read(void);
int vita_input_should_exit(void);
