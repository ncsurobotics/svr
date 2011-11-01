
#include <svr.h>
#include <seawolf.h>

static Dictionary* encodings = NULL;

SVR_Encoding* SVR_Encoding_getByName(const char* name) {
    return Dictionary_get(encodings, name);
}

int SVR_Encoding_register(SVR_Encoding* encoding) {
    if(encodings == NULL) {
        encodings = Dictionary_new();
    }

    if(Dictionary_exists(encodings, encoding->name)) {
        return -1;
    }

    Dictionary_set(encodings, encoding->name, encoding);

    return 0;
}
