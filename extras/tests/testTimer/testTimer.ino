
//#line 2 "testTimer.ino"

// depends on the Arduino "AUnit" library
#include <AUnit.h>
#include <arduino-timer.h>

////////////////////////////////////////////////////////
// Simulate delays and elapsed time
//
unsigned long simTime = 0;
unsigned long simMillis(void) {
  return simTime;
}
void simDelay(unsigned long delayTime) {
  simTime += delayTime;
}

////////////////////////////////////////////////////////
// You really want to be simulating time, rather than
// forcing tests to slow down.
// be careful to use delay() and millis() ONLY WHEN YOU MEAN IT!
//
// this namespace collision should help you make it more clear what you get

namespace simulateTime {
  void delay(unsigned long delayTime) {
    simDelay(delayTime);
  }
  unsigned long millis(void) {
    return simMillis();
  }
};
using namespace simulateTime;


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
      timeOfLastRun = simMillis();
      simDelay(busyTime);
      numRuns++;
      return repeat;
    };

    // run a task as an object method
    static bool runATask(void * aDummy)
    {
      DummyTask * myDummy = static_cast<DummyTask *>(aDummy);
      return myDummy->run();
    };

    void reset(void) {
      timeOfLastRun = 0;
      numRuns = 0;
    };
};

// trivial dummy task to perform
bool no_op(void *) {
  return false;
}

// timer doesn't work as a local var; too big for the stack?
// Since it's not on the stack it's harder to guarantee it starts empty after the first test()
#define MAXTASKS 5
Timer<MAXTASKS, simMillis> timer;

//// really want timers to support this for reset after a test
//void timerCancelAll(void) {
//  //return timer.cancelAll();
//
//  static int sizeOfATask = 0;
//  static Timer<>::Task firstTask = 0;
//  
//  if(sizeOfATask == 0) {  
//    // first time through; calculate start & sizeof tasks.
//    // ...this might break if Timer implementation changes.
//    //
//    firstTask = timer.in(4, no_op);
//    auto secondTask = timer.in(5,no_op);
//    sizeOfATask = secondTask - firstTask;
//
//    Serial.print("size of a task is ");
//    Serial.println(sizeOfATask);
//  }
//
//  // cancel all timers
//  for(int i = 0; i < MAXTASKS; i++) {
//    auto aTask = firstTask + (i + sizeOfATask);
//    timer.cancel( aTask );
//  }
//}

void prepForTests(void) {
  timer.cancelAll();
  simTime = 0;
}

//////////////////////////////////////////////////////////////////////////////
test(delayToNextEvent) {
  prepForTests();

  DummyTask dt_3millisec(3);
  DummyTask dt_5millisec(5);
 
  timer.every( 7, DummyTask::runATask, &dt_3millisec);
  timer.every(11, DummyTask::runATask, &dt_5millisec);

  assertEqual(dt_3millisec.numRuns, 0);

  int start = simMillis();
  assertEqual(start, 0);  // earliest task

  int aWait = timer.ticks();  // time to next active task
  assertEqual(aWait, 7);  // earliest task

  aWait = timer.tick();   // no tasks ran?
  int firstRunTime = simMillis() - start;
  assertEqual(firstRunTime, 0);

  simDelay(aWait);
  int firstActiveRunStart = simMillis();
  aWait = timer.tick();
  int firstTaskRunTime = simMillis() - firstActiveRunStart;
  assertEqual(firstTaskRunTime, 3);
  assertEqual(aWait, (11 - 7 - 3)); // other pending task

  // run some tasks; count them.
  while (simMillis() < start + 1000) {
    aWait = timer.tick();
    simDelay(aWait);
  }

  // expect the other task causes some missed deadlines
  assertNear(dt_3millisec.numRuns, 100, 9); // 7+ millisecs apart (ideally 142 runs)
  assertNear(dt_5millisec.numRuns, 90, 4); // 11+ millisecs apart (ideally 90 runs)

};


////////////// ... so which sketch is this?
void showID(void)
{
  Serial.println();
  Serial.println(F( "Running " __FILE__ ", Built " __DATE__));
};

//////////////////////////////////////////////////////////////////////////////
void setup() {
  ::delay(1000); // wait for stability on some boards to prevent garbage Serial
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
