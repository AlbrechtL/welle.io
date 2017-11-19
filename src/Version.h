#ifndef VERSION_H
#define VERSION_H

// Fallback if git hash macro is not defined
#ifndef GITHASH
#pragma message "Git hash is not defined! Set it to \"unknown\""
#define GITHASH "unknown"
#endif

#define CURRENT_VERSION "2.0-alpha"

#endif // VERSION_H
