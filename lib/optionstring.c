/**
 * \file
 * \brief Option string
 */

#include <svr.h>
#include <ctype.h>

static int svr_parse_error_position = -1;

static void consume_whitespace(char* descriptor, int* offset);
static char* get_value(char* descriptor, int* offset);
static char* get_identifier(char* descriptor, int* offset);

/**
 * \defgroup OptionString Option string
 * \ingroup Util
 * \brief Parse a specially formatted option string
 * \{
 */

/**
 * \brief Parse an option string
 *
 * An option string is a string of the form
 *
 * &lt;object&gt;:&lt;option&gt;=&lt;value&gt;,&lt;option&gt;=&lt;value&gt;,...
 *
 * It is meant to encode a reference to some sort of object or type (&lt;object&gt;)
 * along with a set of options or parameters for that type. Only the &lt;object&gt;
 * part of the string is required. &lt;object&gt; and all &lt;option&gt;s should be
 * alphanumeric + '_'. &lt;values&gt; can contain any value except NULL and ',' though
 * trailing and leading whitespace will be removed. The parsed string is
 * returned as a Dictionary mapping options to values. The special key "%name"
 * points to the value of &lt;object&gt;.
 */
Dictionary* SVR_parseOptionString(const char* s) {
    Dictionary* options;
    char* descriptor;
    char* identifier;
    char* value;
    int offset;
    int end;

    options = Dictionary_new();
    descriptor = strdup(s);
    offset = 0;

    /* Save the descriptor to the options dictionary. This way we can use the
       memory allocated to store the descriptor string instead of allocating
       space for each value in the dictionary */
    Dictionary_set(options, "%descriptor", descriptor);

    /* Read the object name */
    identifier = get_identifier(descriptor, &offset);
    end = offset;

    /* No options part, just object name */
    consume_whitespace(descriptor, &offset);
    if(descriptor[offset] == '\0') {
        descriptor[end] = '\0';
        Dictionary_set(options, "%name", identifier);
        return options;
    }

    /* If the string is not just the object name then it must have a ':'
       seperating the object name from the options */
    if(descriptor[offset] != ':') {
        goto error;
    }

    /* Terminate the object name and store it */
    descriptor[end] = '\0';
    Dictionary_set(options, "%name", identifier);
    offset++;

    while(descriptor[offset] != '\0') {
        identifier = get_identifier(descriptor, &offset);
        end = offset;

        if(identifier == NULL) {
            if(descriptor[offset] == '\0') {
                /* End of input */
                break;
            } else {
                /* Unexpected character */
                goto error;
            }
        }

        consume_whitespace(descriptor, &offset);
        if(descriptor[offset] != '=') {
            goto error;
        }
        offset++;

        descriptor[end] = '\0';
        value = get_value(descriptor, &offset);
        end = offset;

        /* No value */
        if(value == NULL) {
            goto error;
        }

        consume_whitespace(descriptor, &offset);
        if(descriptor[offset] == '\0') {
            descriptor[end] = '\0';
            Dictionary_set(options, identifier, value);
            break;
        }

        /* Unexpected character (this shouldn't be able to happen) */
        if(descriptor[offset] != ',') {
            goto error;
        }
        offset++;

        consume_whitespace(descriptor, &offset);
        if(descriptor[offset] == '\0') {
            /* Trailing ',' */
            goto error;
        }

        descriptor[end] = '\0';
        Dictionary_set(options, identifier, value);
    }

    svr_parse_error_position = -1;
    return options;

 error:
    svr_parse_error_position = offset;
    Dictionary_destroy(options);
    free(descriptor);
    return NULL;
}

void SVR_freeParsedOptionString(Dictionary* options) {
    char* parse_copy = Dictionary_get(options, "%descriptor");

    free(parse_copy);
    Dictionary_destroy(options);
}

int SVR_getOptionStringErrorPosition(void) {
    return svr_parse_error_position;
}

static void consume_whitespace(char* descriptor, int* offset) {
    while(isspace(descriptor[*offset])) {
        (*offset)++;
    }
}

static char* get_value(char* descriptor, int* offset) {
    int start;
    int end;

    consume_whitespace(descriptor, offset);
    start = *offset;
    end = start;

    /* Continue until a , or end of the string */
    while(strchr(",", descriptor[*offset]) == NULL) {
        (*offset)++;

        /* end always is the index of the last non-whitespace character */
        if(!isspace(descriptor[*offset])) {
            end = (*offset);
        }
    }

    /* Rewind offset to before any trailing whitespace */
    (*offset) = end;

    if(start == end) {
        /* No value */
        return NULL;
    }

    return descriptor + start;
}

static char* get_identifier(char* descriptor, int* offset) {
    int start;

    consume_whitespace(descriptor, offset);
    start = *offset;

    while(isalnum(descriptor[*offset]) || descriptor[*offset] == '_') {
        (*offset)++;
    }

    /* Empty/invalid */
    if(*offset == start) {
        return NULL;
    }

    return descriptor + start;
}

/** \} */
