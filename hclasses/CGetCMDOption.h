#ifndef CGetCMDOption_H
#define CGetCMDOption_H
#include "CDefinitions.h"
#include "CDirReader.h"
#include "CDebugger.h"
int getCMDOption(const char * pszOption, char *psz_value, const char * psz_default, int argc, const char *argv[]);
int getCMDOption(const char * pszOption,  CDirReader*dirlist, int argc, const char *argv[]);
void CMDSetHelpText(const char * help);
void CMDPrintHelp();
#endif

