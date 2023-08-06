// arduino-timer unit tests
// timerMultiCancelTest.ino unit test
// find regressions when:
//     timer.cancel(...) is called twice and cancels an extra task
//     timer.in/at/every(...) return task id 0 (should mean "not created")
//     timer.in/at/every(...) return an in-use task id (1 id with 2 tasks)

// Arduino "AUnit" library required

// Required for UnixHostDuino emulation
#include <Arduino.h>

#if defined(UNIX_HOST_DUINO)
#ifndef ARDUINO
#define ARDUINO 100
#endif
#endif

#include <AUnit.h>
#include <arduino-timer.h>

//#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

auto timer = timer_create_default();  // create a timer with default settings

// a generic task
bool dummyTask(void*) {
  //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));  // toggle the LED
  return true;  // repeat? true
}

struct TaskInfo {
  Timer<>::Task id;  // id to use to cancel
  unsigned long instanceNumber;
};

static const int numTaskIds = 20;

TaskInfo tasksToCancel[numTaskIds] = { { 0, 0 } };

unsigned long creationAttempts = 0;

bool createTask(int index) {
  bool successful = true;  // OK so far
  TaskInfo& taskInfo = tasksToCancel[index];

  if (taskInfo.id != (Timer<>::Task)NULL) {
    // cancel task in slot
    auto staleId = taskInfo.id;
    int beforeSize = (int)timer.size();
    successful &= timer.cancel(taskInfo.id);
    int afterSize = (int)timer.size();
    successful &= (afterSize == beforeSize - 1);
    if (!successful) {
      Serial.println(F("could not cancel a task"));
    } else {

      successful &= !timer.cancel(staleId);  // double cancel should not hit another task
      int afterSize2 = (int)timer.size();
      successful &= (afterSize2 == beforeSize - 1);
      if (!successful) {
        Serial.println(F("second cancel removed another task"));
      }
    }
  }

  auto newId = timer.every(250, dummyTask);
  ++creationAttempts;

  static size_t timerSize = 0;
  auto newSize = timer.size();
  if (timerSize != newSize) {
    timerSize = newSize;
    DEBUG_PRINT(F("Timer now has "));
    DEBUG_PRINT(timerSize);
    DEBUG_PRINTLN(F(" tasks"));
  }

  if (newId == 0) {
    Serial.print(F("timer task creation failure on creation number "));
    Serial.println(creationAttempts);
    successful = false;

  } else {

    // check for collisions before saving taskInfo
    for (int i = 0; i < numTaskIds; i++) {
      const TaskInfo& ti = tasksToCancel[i];
      if (ti.id == newId) {
        successful = false;
        Serial.print(F("COLLISION FOUND! instance number: "));
        Serial.print(creationAttempts);
        Serial.print(F(" hash "));
        Serial.print(F("0x"));
        Serial.print((size_t)newId, HEX);
        Serial.print(F(" "));
        Serial.print((size_t)newId, BIN);

        Serial.print(F(" matches hash for instance number: "));
        Serial.println(ti.instanceNumber);
      }
    }
    taskInfo.id = newId;
    taskInfo.instanceNumber = creationAttempts;

    static const unsigned long reportCountTime = 10000;
    if (creationAttempts % reportCountTime == 0) {
      DEBUG_PRINT(creationAttempts / 1000);
      DEBUG_PRINTLN(F("k tasks created."));
    }
  }
  return successful;
}

test(timerMultiCancel) {
  timer.cancel();  // ensure timer starts empty
  assertEqual((int)timer.size(), 0);
  creationAttempts = 0;

  // timer capacity is 0x10 -- stay below
  // load up some static tasks
  for (int i = 0; i < 6; i++) {
    assertTrue(createTask(i));
  }

  assertEqual((int)timer.size(), 6);

  // cancel/recreate tasks
  //for (unsigned long groups = 0; groups < 30000UL; groups++) {
  unsigned long groups = 0;
  do {
    //for (unsigned long groups = 0; creationAttempts < 0x10010; groups++) {  // trouble over 64k tasks?
    for (int i = 9; i < 0x10; i++) {
      assertTrue(createTask(i));
      if (groups > 0) {
        // should be steady-state task size now
        assertEqual((int)timer.size(), 13);
      }
    }
    groups++;
    //}
  } while (creationAttempts < 0x10010);  // no trouble over 64K tasks?

  Serial.print(creationAttempts);
  Serial.println(F(" tasks created."));
}

void sketch(void) {
  Serial.println();
  Serial.println(F("Running " __FILE__ ", Built " __DATE__));
}

void setup() {
  ::delay(1000UL);         // wait for stability on some boards to prevent garbage Serial
  Serial.begin(115200UL);  // ESP8266 default of 74880 not supported on Linux
  while (!Serial)
    ;  // for the Arduino Leonardo/Micro only
  sketch();
}

void loop() {
  // Should get:
  // TestRunner summary:
  //    <n> passed, <n> failed, <n> skipped, <n> timed out, out of <n> test(s).
  aunit::TestRunner::run();
}
