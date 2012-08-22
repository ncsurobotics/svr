
#ifndef __SVR_SERVER_SOURCES_H
#define __SVR_SERVER_SOURCES_H

extern SVRD_SourceType SVR_SOURCE(test);
extern SVRD_SourceType SVR_SOURCE(cam);
extern SVRD_SourceType SVR_SOURCE(file);

#ifdef __SW_Linux__
extern SVRD_SourceType SVR_SOURCE(v4l);
#endif

#endif // #ifndef __SVR_SERVER_SOURCES_H
