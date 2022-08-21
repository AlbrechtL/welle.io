
// Fallback if git hash macro is not defined
#ifndef GITHASH
#pragma message "Git hash is not defined! Set it to \"unknown\""
#define GITHASH "unknown"
#endif

// Fail if CURRENT_VERSION is not defined
#ifndef CURRENT_VERSION
#pragma message "CURRENT_VERSION is not defined! Set it in '_current_version' file"
#endif
