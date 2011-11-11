
#ifndef __SVR_OPTIONSTRING_H
#define __SVR_OPTIONSTRING_H

Dictionary* SVR_parseOptionString(const char* s);
void SVR_freeParsedOptionString(Dictionary* options);
int SVR_getOptionStringErrorPosition(void);

#endif // #ifndef __SVR_OPTIONSTRING_H
