/**
 * \file
 * \brief Frame properties
 */

#include <svr.h>

/**
 * \defgroup FrameProperties Frame properties
 * \ingroup Misc
 * \brief Frame properties
 * \{
 */

/**
 * \brief Allocate a new FrameProperties object
 *
 * Create a new FrameProperties object with default parameters (8-bit depth, 3
 * channels, 0 pixel width and 0 pixel height)
 *
 * \return FrameProperties object
 */
SVR_FrameProperties* SVR_FrameProperties_new(void) {
    SVR_FrameProperties* properties = malloc(sizeof(SVR_FrameProperties));

    properties->height = 0;
    properties->width = 0;
    properties->depth = 8;
    properties->channels = 3;

    return properties;
}


/**
 * \brief Create a FramePropeties object from a string
 *
 * Convert a string in the form "width,height,depth,channels" to a
 * FramePropreties with the same properties. Any number of properties may be
 * ommitted from the end of the string and the defaults for a FrameProperties
 * object will be used instead.
 *
 * \param string String in the form "width,height,depth,channels"
 * \return New FrameProperties object with properties given by the string
 */
SVR_FrameProperties* SVR_FrameProperties_fromString(const char* string) {
    SVR_FrameProperties* properties = SVR_FrameProperties_new();
    char* copy = strdup(string);
    char* item = copy;
    int i = 0;

    item = strtok(item, ",");
    if(item) {
        do {
            switch(i) {
            case 0:
                properties->width = atoi(item);
                break;

            case 1:
                properties->height = atoi(item);
                break;

            case 2:
                properties->depth = atoi(item);
                break;

            case 3:
                properties->channels = atoi(item);
                break;

            default:
                break;
            }

            i++;
        } while((item = strtok(NULL, ",")) && i < 4);
    }

    free(copy);
    return properties;
}

/**
 * \brief Destroy a FrameProperties object
 *
 * Destroy/free a FrameProperties object
 *
 * \param properties Object to destroy
 */
void SVR_FrameProperties_destroy(SVR_FrameProperties* properties) {
    free(properties);
}

/**
 * \brief Clone an existing FrameProperties object
 *
 * Create a new FrameProperties object with the same properties as the given
 * FrameProperties object
 *
 * \param original_properties FrameProperties object to clone
 * \return A new FrameProperties object
 */
SVR_FrameProperties* SVR_FrameProperties_clone(SVR_FrameProperties* original_properties) {
    SVR_FrameProperties* cloned_properties = malloc(sizeof(SVR_FrameProperties));
    memcpy(cloned_properties, original_properties, sizeof(SVR_FrameProperties));
    return cloned_properties;
}

/**
 * \brief Create an IplImage from a FrameProperties object
 *
 * Create an IplImage with properties (height, width, depth, channels) derived
 * from the given FrameProperties object.
 *
 * \param properties A FrameProperties object
 * \return A new IplImage with the same properties as the given FrameProperties
 * object
 */
IplImage* SVR_FrameProperties_imageFromProperties(SVR_FrameProperties* properties) {
    return cvCreateImage(cvSize(properties->width,
                                properties->height),
                         properties->depth,
                         properties->channels);
}

/** \} */
