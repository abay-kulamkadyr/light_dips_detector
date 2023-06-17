#ifndef _UDP_LISTENER_H_
#define _UDP_LISTENER_H_
void UDP_Init(void);
void UDP_Destroy(void);
struct options
{
    bool option_help;
    bool option_count;
    bool option_getN;
    bool option_length;
    bool option_history;
    bool option_stop;
    bool option_repeatLast;
};
#endif