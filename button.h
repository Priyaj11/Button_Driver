#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

/* The gestures the driver can report */
typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_SINGLE,
    BUTTON_EVENT_DOUBLE,
    BUTTON_EVENT_LONG
} ButtonEvent;

void        Button_Init(void);     /* call once at startup            */
void        Button_Update(void);   /* call often (loop / after IRQ)   */
ButtonEvent Button_GetEvent(void); /* returns one event, then clears  */
uint8_t     Button_IsActive(void); /* 1 while a press is being judged */

#endif /* BUTTON_H */
