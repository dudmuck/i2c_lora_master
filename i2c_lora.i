%module i2c_lora

%inline %{
    typedef unsigned char uint8_t;
%}

%{
/* Includes the header in the wrapper code */
#include "i2c_lora.h"
%}

/* Parse the header file to generate wrappers */
%include "i2c_lora.h"
