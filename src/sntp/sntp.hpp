#ifndef SNTP_HPP
#define SNTP_HPP

#include <ctime>

void sntp_start(void);
time_t sntp_wait_for_sync(void);

#endif // SNTP_HPP
