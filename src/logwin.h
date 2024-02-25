/* Log into separate window */

#include "logwin.hxx"

#define log_printf(...) bmutex_lock(log_mutex); stm_printf(log_stream, __VA_ARGS__); bmutex_unlock(log_mutex)

LogWin* logwin_create(void);

void logwin_show(LogWin* log, const V2Df pos);

void logwin_hide(LogWin*);

bool_t logwin_is_open(void);

void logwin_destroy(LogWin**);

void logwin_update(void);
