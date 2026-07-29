# STM32 Button Driver

A small, reusable push-button driver for the STM32F103 ("Blue Pill"). One physical
button, wired to a single GPIO pin, is turned into clean high-level events:

- **Single press**
- **Double press**
- **Long press**

with **software debouncing** so electrical noise never produces a false event.
The same driver runs in two modes: **polling** and **interrupt (EXTI)**.

The hardware is deliberately dumb — it only connects a pin to ground. All of the
intelligence (debounce + gesture detection) lives in `button.c`, so `main.c` never
touches the GPIO directly. It just asks: *what did the user do?*

```c
Button_Update();

switch (Button_GetEvent())
{
    case BUTTON_EVENT_SINGLE:  /* toggle LED        */  break;
    case BUTTON_EVENT_DOUBLE:  /* blink twice       */  break;
    case BUTTON_EVENT_LONG:    /* LED on 2 seconds  */  break;
    default:                                            break;
}
```

## Hardware

| Signal | Pin  | Notes                                   |
|--------|------|-----------------------------------------|
| Button | PA0  | Other side to GND, **internal pull-up** |
| LED    | PC13 | On-board LED (active-low)               |

Because of the pull-up to GND, the pin reads **LOW when pressed**.

## Files

| File                | Purpose                                             |
|---------------------|-----------------------------------------------------|
| `Core/Inc/button.h` | Public API — events and functions                   |
| `Core/Src/button.c` | Debounce + gesture state machine                    |

Drop these into a CubeMX/STM32CubeIDE project generated for the STM32F103C8Tx.

## Public API

```c
void        Button_Init(void);      // call once at startup
void        Button_Update(void);    // call often (loop, or after an interrupt)
ButtonEvent Button_GetEvent(void);  // returns one event, then clears it
uint8_t     Button_IsActive(void);  // 1 while a gesture is being judged
```

## How it works

The driver is two layers stacked on top of each other.

**1. Debounce.** A raw pin read is noisy — one physical press produces a burst of
`010101...` for a few milliseconds. The driver only accepts a new state once the
raw reading has held steady for `DEBOUNCE_MS` (20 ms). This turns the noise into
two clean events: *just pressed* and *just released*.

**2. Gesture state machine.** Those clean edges, plus timing from `HAL_GetTick()`,
drive a small finite state machine:

```
IDLE ──press──► PRESSED ──held 1s──► LONG_HELD ──release──► IDLE
                   │
                release
                   ▼
              WAIT_SECOND ──2nd press──► WAIT_RELEASE ──release──► IDLE   (DOUBLE)
                   │
              400ms gap
                   ▼
                 IDLE                                                     (SINGLE)
```

Long press fires exactly once. Single press only fires *after* the double-press
window closes with no second tap.

## Polling vs. interrupt mode

**Polling** — call `Button_Update()` every loop. Simple, always correct.

**Interrupt (EXTI)** — configure PA0 as `GPIO_EXTI0`, both-edge trigger, and set a
flag in the callback:

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) button_irq_flag = 1;
}
```

Then in the loop:

```c
if (button_irq_flag || Button_IsActive())
{
    button_irq_flag = 0;
    Button_Update();
}
```

An interrupt only fires when the pin *changes*, but long press and the
single-vs-double decision depend on *time passing with no change*. The
`|| Button_IsActive()` guard is what keeps `Button_Update()` running through those
timeouts once a gesture has started, then goes quiet until the next press. That is
the point of the design — interrupt efficiency without breaking the timing logic.

## Tuning

Timing constants live at the top of `button.c`:

```c
#define DEBOUNCE_MS     20      // debounce settle time
#define LONG_PRESS_MS   1000    // hold this long = long press
#define DOUBLE_GAP_MS   400     // max gap between taps for a double press
```

## Build

Generate an STM32CubeIDE project for the **STM32F103C8Tx**, add `button.c` /
`button.h` to `Core/Src` / `Core/Inc`, wire the calls into the `USER CODE`
sections of `main.c`, and build. Output is a `.elf` / `.hex` you can flash or run
in a simulator.

## Skills demonstrated

GPIO input · internal pull-ups · software debouncing · edge detection ·
system timers (`HAL_GetTick()`) · finite state machines · modular driver design ·
polling firmware · external interrupts (EXTI).

## License

MIT — see [LICENSE](LICENSE).
