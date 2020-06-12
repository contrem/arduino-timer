/*
 * timer_full
 *
 * Full example using the arduino-timer library.
 * Shows:
 *  - Setting a different number of tasks with microsecond resolution
 *  - disabling a repeated function
 *  - running a function after a delay
 *  - cancelling a task
 *
 */

#include <arduino-timer.h>

auto timer = timer_create_default(); // create a timer with default settings
Timer<> default_timer; // save as above

// create a timer that can hold 1 concurrent task, with microsecond resolution
// and a custom handler type of 'const char *
Timer<1, micros, const char *> u_timer;


// create a timer that holds 16 tasks, with millisecond resolution,
// and a custom handler type of 'const char *
Timer<16, millis, const char *> t_timer;

bool toggle_led(void *) {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // toggle the LED
  return true; // repeat? true
}

bool print_message(const char *m) {
  Serial.print("print_message: ");
  Serial.println(m);
  return true; // repeat? true
}

size_t repeat_count = 1;
bool repeat_x_times(void *opaque) {
  size_t limit = (size_t)opaque;

  Serial.print("repeat_x_times: ");
  Serial.print(repeat_count);
  Serial.print("/");
  Serial.println(limit);

  return ++repeat_count <= limit; // remove this task after limit reached
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT); // set LED pin to OUTPUT

  // call the toggle_led function every 500 millis (half second)
  timer.every(500, toggle_led);

  // call the repeat_x_times function every 1000 millis (1 second)
  timer.every(1000, repeat_x_times, (void *)10);

  // call the print_message function every 1000 millis (1 second),
  // passing it an argument string
  t_timer.every(1000, print_message, "called every second");

  // call the print_message function in five seconds
  t_timer.in(5000, print_message, "delayed five seconds");

  // call the print_message function at time + 10 seconds
  t_timer.at(millis() + 10000, print_message, "call at millis() + 10 seconds");

  // call the toggle_led function every 500 millis (half second)
  auto task = timer.every(500, toggle_led);
  timer.cancel(task); // this task is now cancelled, and will not run

  // call print_message in 2 seconds, but with microsecond resolution
  u_timer.in(2000000, print_message, "delayed two seconds using microseconds");

  if (!u_timer.in(5000, print_message, "never printed")) {
  /* this fails because we created u_timer with only 1 concurrent task slot */
    Serial.println("Failed to add microsecond event - timer full");
  }
}

void loop() {
  timer.tick(); // tick the timer
  t_timer.tick();
  u_timer.tick();
}
