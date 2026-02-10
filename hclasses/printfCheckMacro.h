#ifndef PRINTFCHECKMACRO_H
#define PRINTFCHECKMACRO_H

#define PRINTF_FORMAT_CHECK(format_index, args_index) __attribute__((__format__(printf, format_index, args_index)))

#endif