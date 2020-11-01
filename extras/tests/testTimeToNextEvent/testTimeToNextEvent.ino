#line 2 "testTimeToNextEvent.ino"

// depends on the Arduino "AUnit" library
#include <AUnit.h>

#include <arduino-timer.h>

////////////////////////////////////////////////////////
// instrumented dummy tasks
class DummyTask {
  public:
    DummyTask(int runTime, bool repeats = true):
      busyTime(runTime), numRuns(0), timeOfLastRun(0), repeat(repeats)
    {
      Serial.print("DummyTask at: ");
      Serial.println((int) this, HEX);
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
    static bool run(void * aDummy) {
      DummyTask * aTask = (DummyTask *) aDummy;
      Serial.print("DummyTask at: ");
      Serial.println((int) aDummy, HEX);
      
      return aTask->run();
    };

};


//////////////////////////////////////////////////////////////////////////////
test(delayToNextEvent) {
  Timer<> timer;
  DummyTask dt_3millisec(3);
  DummyTask dt_5millisec(5);

  timer.every( 7, DummyTask::run, &dt_3millisec);
  timer.every(11, DummyTask::run, &dt_5millisec);

  int start = millis();

  int aWait = timer.ticks();  // time to next active task
  assertEqual(aWait, 0);  // run tasks NOW!

  aWait = timer.tick();   // both tasks ran?
  int firstRunTime = millis() - start;
  assertEqual(firstRunTime, 8);
  //
  //  assertEqual(aWait, 0);  // short task needs to run again
  //
  //  aWait = timer.tick(); // run one pending task
  //  assertEqual(aWait, 11 - 7);

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
