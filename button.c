#include "button.h"
#include "main.h"   /* gives us HAL, GPIOA, GPIO_PIN_0, HAL_GetTick() */

/* ---------------- Tuning (milliseconds) ---------------- */
#define DEBOUNCE_MS     20u     /* noise must settle this long   */
#define LONG_PRESS_MS   1000u   /* hold this long = long press   */
#define DOUBLE_GAP_MS   400u    /* 2nd press must start within    */

/* Wiring: PA0 -> GND with internal pull-up.
   So the pin reads LOW (RESET) when the button is pressed. */
#define BUTTON_PORT     GPIOA
#define BUTTON_PIN      GPIO_PIN_0

/* ---------------- Layer 1: debounce ---------------- */
static uint8_t  stable_pressed = 0;   /* clean state: 1 = pressed */
static uint8_t  last_raw       = 0;
static uint32_t last_change_ms = 0;

/* ---------------- Layer 2: gesture FSM ---------------- */
typedef enum {
    S_IDLE,          /* nothing happening                          */
    S_PRESSED,       /* button down, timing for a long press       */
    S_LONG_HELD,     /* long press already fired, wait for release */
    S_WAIT_SECOND,   /* released once, is a 2nd press coming?       */
    S_WAIT_RELEASE   /* 2nd press down, wait for release then idle */
} FsmState;

static FsmState    state      = S_IDLE;
static uint32_t    press_ms   = 0;
static uint32_t    release_ms = 0;
static ButtonEvent pending    = BUTTON_EVENT_NONE;

static uint8_t ReadRawPressed(void)
{
    /* pressed = LOW because of the pull-up to GND */
    return (HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET) ? 1u : 0u;
}

void Button_Init(void)
{
    last_raw       = ReadRawPressed();
    stable_pressed = last_raw;
    last_change_ms = HAL_GetTick();
    state          = S_IDLE;
    pending        = BUTTON_EVENT_NONE;
}

void Button_Update(void)
{
    uint32_t now = HAL_GetTick();

    /* ---- 1) debounce: turn noisy pin into clean edges ---- */
    uint8_t raw          = ReadRawPressed();
    uint8_t edge_press   = 0;
    uint8_t edge_release = 0;

    if (raw != last_raw) {
        last_raw       = raw;      /* pin moved: (re)start the timer */
        last_change_ms = now;
    } else if (raw != stable_pressed &&
               (now - last_change_ms) >= DEBOUNCE_MS) {
        stable_pressed = raw;      /* held steady long enough: accept */
        if (stable_pressed) edge_press = 1;
        else                edge_release = 1;
    }

    /* ---- 2) gesture state machine ---- */
    switch (state) {
    case S_IDLE:
        if (edge_press) { press_ms = now; state = S_PRESSED; }
        break;

    case S_PRESSED:
        if (edge_release) {
            release_ms = now;
            state = S_WAIT_SECOND;              /* maybe a double is coming */
        } else if ((now - press_ms) >= LONG_PRESS_MS) {
            pending = BUTTON_EVENT_LONG;        /* fire long ONCE */
            state = S_LONG_HELD;
        }
        break;

    case S_LONG_HELD:
        if (edge_release) state = S_IDLE;
        break;

    case S_WAIT_SECOND:
        if (edge_press) {
            pending = BUTTON_EVENT_DOUBLE;      /* 2nd press = double */
            state = S_WAIT_RELEASE;
        } else if ((now - release_ms) >= DOUBLE_GAP_MS) {
            pending = BUTTON_EVENT_SINGLE;      /* no 2nd press = single */
            state = S_IDLE;
        }
        break;

    case S_WAIT_RELEASE:
        if (edge_release) state = S_IDLE;
        break;
    }
}

ButtonEvent Button_GetEvent(void)
{
    ButtonEvent e = pending;
    pending = BUTTON_EVENT_NONE;
    return e;
}

uint8_t Button_IsActive(void)
{
    /* active while a gesture is running OR a pin change is still settling.
       (the settling part is what makes interrupt mode work correctly)   */
    return (state != S_IDLE || last_raw != stable_pressed) ? 1u : 0u;
}
