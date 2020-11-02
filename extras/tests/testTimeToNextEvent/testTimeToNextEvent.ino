#line 2 "testTimeToNextEvent.ino"

// depends on the Arduino "AUnit" library
#include <AUnit.h>

#include <arduino-timer.h>

////////////////////////////////////////////////////////
// instrumented dummy tasks
class DummyTask {
  public:
    DummyTask(int runTime, bool repeats = true):
      busyTime(runTime), repeat(repeats)
    {
      reset();
    };

    int busyTime;
    int numRuns;
    int timeOfLastRun;
    bool repeat;

    bool run(void) {
      timeOfLastRun = millis();
      delay(busyTime);
      numRuns++;
      return repeat;
    };

    // run a task as an object method
    static bool runATask(void * aDummy)
    {
      DummyTask * aTask = static_cast<DummyTask *>(aDummy);
      return aTask->run();
    };

    void reset(void) {
      timeOfLastRun = 0;
      numRuns = 0;
    };
};

// timer doesn't work as a local var; too big for the stack?
// Since it's not on the stack it's harder to guarantee it starts empty after the first test()
Timer<> timer;

//////////////////////////////////////////////////////////////////////////////
test(delayToNextEvent) {

  DummyTask dt_3millisec(3);
  DummyTask dt_5millisec(5);

  timer.every( 7, DummyTask::runATask, static_cast<void *>(&dt_3millisec));
  timer.every(11, DummyTask::runATask, static_cast<void *>(&dt_5millisec));

  int start = millis();

  int aWait = timer.ticks();  // time to next active task
  assertEqual(aWait, 7);  // earliest task

  aWait = timer.tick();   // no tasks ran?
  int firstRunTime = millis() - start;
  assertEqual(firstRunTime, 0);

  delay(aWait);
  int firstActiveRunStart = millis();
  aWait = timer.tick();
  int firstTaskRunTime = millis() - firstActiveRunStart;
  assertEqual(firstTaskRunTime, 3);
  assertEqual(aWait, (11 - 7 - 3)); // other pending task

  // run some tasks; count them.
  while (millis() < start + 1000) {
    aWait = timer.tick();
    delay(aWait);
  }

  // expected numbers? the other task may cause some missed deadlines
  assertNear(dt_3millisec.numRuns, 100, 2); // 7+ millisecs apart (ideally 142 runs)
  assertNear(dt_5millisec.numRuns, 90, 4); // 11+ millisecs apart (ideally 90 runs)

};

void showID(void)
{
  Serial.println();
  Serial.println(F( "Running " __FILE__ ", Built " __DATE__));
};

//////////////////////////////////////////////////////////////////////////////
void setup() {
  delay(1000); // wait for stability on some boards to prevent garbage Serial
  Serial.begin(115200); // ESP8266 default of 74880 not supported on Linux
  while (!Serial); // for the Arduino Leonardo/Micro only
  showID();
}

//////////////////////////////////////////////////////////////////////////////
void loop() {
  // Should get:
  // TestRunner summary:
  //    <n> passed, <n> failed, <n> skipped, <n> timed out, out of <n> test(s).
  aunit::TestRunner::run();
}
