/******************************************************************************
 *
 * Project:  Helper classes
 * Purpose:  Compile-time printf format checking macro
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 ******************************************************************************/

#ifndef PRINTFCHECKMACRO_H
#define PRINTFCHECKMACRO_H

#if defined(__GNUC__) || defined(__clang__)
#define PRINTF_FORMAT_CHECK(format_index, args_index) __attribute__((__format__(printf, format_index, args_index)))
#else
#define PRINTF_FORMAT_CHECK(format_index, args_index)
#endif

#endif
