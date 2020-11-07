//
// timerTest.ino
//
// Confirm arduino-timer behaves as expected.

// UnixHostDuino emulation needs this include
// (it's not picked up "for free" by Arduino IDE)
//
#include <Arduino.h>

// fake it for UnixHostDuino emulation
#if defined(UNIX_HOST_DUINO)
#  ifndef ARDUINO
#  define ARDUINO 100
#  endif
#endif

//
// also, you need to provide your own forward references

// These tests depend on the Arduino "AUnit" library
#include <AUnit.h>
#include <arduino-timer.h>

////////////////////////////////////////////////////////
// You really want to be simulating time, rather than
// forcing tests to slow down.
// This simulation works across different processor families.
//
// BE CAREFUL to use delay() and millis() ONLY WHEN YOU MEAN IT!
//
// this namespace collision should help you make it more clear what you get
//
namespace simulateTime {

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

// TRAP ambiguous calls to delay() and millis() that are NOT simulated
void delay(unsigned long delayTime) {
    simDelay(delayTime);
}
unsigned long millis(void) {
    return simMillis();
}

}; // namespace simulateTime

using namespace simulateTime; // and detect ambiguous function calls


////////////////////////////////////////////////////////
// instrumented dummy tasks
class DummyTask {
  public:
    DummyTask(unsigned long runTime, bool repeats = true) :
        busyTime(runTime), repeat(repeats)
    {
        reset();
    };

    unsigned long busyTime;
    unsigned long numRuns;
    unsigned long timeOfLastRun;
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
// Since it's not on the stack it's harder to guarantee it starts empty
// after the first test()
//
#define MAXTASKS 5
Timer<MAXTASKS, simMillis> timer;

void prepForTests(void) {
    timer.cancel();
    simTime = 0ul;
}

//////////////////////////////////////////////////////////////////////////////
// confirm tasks can be cancelled.
test(timer_cancelTasks) {
    prepForTests();

    unsigned long aWait = timer.ticks();  // time to next active task
    assertEqual(aWait, 0ul);  // no tasks!

    DummyTask dt_3millisec(3ul);
    DummyTask dt_5millisec(5ul);
    DummyTask dt_7millisec(7ul);

    auto inTask = timer.in(13ul, DummyTask::runATask, &dt_3millisec);
    auto atTask = timer.at(17ul, DummyTask::runATask, &dt_5millisec);
    auto everyTask = timer.every(19ul, DummyTask::runATask, &dt_7millisec);

    aWait = timer.ticks();
    assertEqual(aWait, 13ul); // inTask delay

    timer.cancel(inTask);
    aWait = timer.ticks();
    assertEqual(aWait, 17ul); // atTask delay

    timer.cancel(atTask);
    aWait = timer.ticks();
    assertEqual(aWait, 19ul); // everyTask delay

    timer.cancel(everyTask);
    aWait = timer.ticks();
    assertEqual(aWait, 0ul); // no tasks! all canceled
};

//////////////////////////////////////////////////////////////////////////////
// confirm timer.at() behaviors
//
test(timer_at) {
    prepForTests();

    unsigned long aWait = timer.ticks();  // time to next active task
    assertEqual(aWait, 0ul);  // no tasks!
    assertEqual(simMillis(), 0ul);

    DummyTask waste_3ms(3ul);

    const unsigned long atTime = 17ul;
    const unsigned long lateStart = 4ul;
    simDelay(lateStart);

    // Note timer.at() returns the task ID.
    // Keep it to modify the task later.
    //
    timer.at(atTime, DummyTask::runATask, &waste_3ms);

    aWait = timer.tick();
    assertEqual(aWait, atTime - lateStart);
    assertEqual(waste_3ms.numRuns, 0ul);

    for (unsigned long i = lateStart + 1ul; i < atTime; i++ ) {
        simDelay(1ul);
        aWait = timer.tick();
        assertEqual(waste_3ms.numRuns, 0ul);  // still waiting
    }

    simDelay(1ul);
    aWait = timer.tick();
    assertEqual(waste_3ms.numRuns, 1ul);  // triggered
    assertEqual(aWait, 0ul);

    simDelay(1ul);
    aWait = timer.tick();
    assertEqual(waste_3ms.numRuns, 1ul);  // not repeating

    aWait = timer.tick();
    assertEqual(aWait, 0ul); // no tasks! all canceled
};

//////////////////////////////////////////////////////////////////////////////
// confirm timer.in() behaviors
//
test(timer_in) {
    prepForTests();

    unsigned long aWait = timer.ticks();  // time to next active task
    assertEqual(aWait, 0ul);  // no tasks!
    assertEqual(simMillis(), 0ul);

    DummyTask waste_3ms(3);

    const unsigned long lateStart = 7;
    simDelay(lateStart);

    const unsigned long delayTime = 17;
    timer.in(delayTime, DummyTask::runATask, &waste_3ms);

    aWait = timer.tick();
    assertEqual(aWait, delayTime);
    assertEqual(waste_3ms.numRuns, 0ul);

    for (unsigned long i = 1ul; i < delayTime; i++ ) {
        simDelay(1ul);
        aWait = timer.tick();
        assertEqual(waste_3ms.numRuns, 0ul);  // still waiting
    }

    simDelay(1ul);
    aWait = timer.tick();
    assertEqual(waste_3ms.numRuns, 1ul);  // triggered
    assertEqual(aWait, 0ul);

    simDelay(1ul);
    aWait = timer.tick();
    assertEqual(waste_3ms.numRuns, 1ul);  // not repeating

    aWait = timer.tick();
    assertEqual(aWait, 0ul); // no tasks! all canceled
};

//////////////////////////////////////////////////////////////////////////////
// confirm timer.every() behaviors
//
test(timer_every) {
    prepForTests();

    unsigned long aWait = timer.ticks();  // time to next active task
    assertEqual(aWait, 0ul);  // no tasks!
    assertEqual(simMillis(), 0ul);

    DummyTask waste_3ms(3ul);
    DummyTask waste_100ms_once(100ul, false);

    const unsigned long lateStart = 7ul;
    simDelay(lateStart);

    //const unsigned long delayTime = 17;
    timer.every(50ul, DummyTask::runATask, &waste_3ms);
    timer.every(200ul, DummyTask::runATask, &waste_100ms_once);

    aWait = timer.tick();
    assertEqual(aWait, 50ul);
    assertEqual(waste_3ms.numRuns, 0ul);

    for (unsigned long i = 1ul; i < 1000ul; i++ ) {
        simDelay(1ul);
        aWait = timer.tick();
    }

    assertEqual(waste_3ms.numRuns, 22ul);  // triggered
    assertEqual(waste_100ms_once.numRuns, 1ul);  // triggered

    aWait = timer.tick();
    assertEqual(aWait, 39ul); // still a repeating task
};

//////////////////////////////////////////////////////////////////////////////
// confirm calculated delays to next event in the timer are "reasonable"
// reported by timer.tick() and timer.ticks().
//
test(timer_delayToNextEvent) {
    prepForTests();

    unsigned long aWait = timer.ticks();  // time to next active task
    assertEqual(aWait, 0ul);  // no tasks!

    DummyTask dt_3millisec(3ul);
    DummyTask dt_5millisec(5ul);
    timer.every( 7ul, DummyTask::runATask, &dt_3millisec);
    timer.every(11ul, DummyTask::runATask, &dt_5millisec);

    assertEqual(dt_3millisec.numRuns, 0ul);

    unsigned long start = simMillis();
    assertEqual(start, 0ul);  // earliest task

    aWait = timer.ticks();  // time to next active task
    assertEqual(aWait, 7ul);  // earliest task

    aWait = timer.tick();   // no tasks ran?
    unsigned long firstRunTime = simMillis() - start;
    assertEqual(firstRunTime, 0ul);

    simDelay(aWait);
    unsigned long firstActiveRunStart = simMillis();
    aWait = timer.tick();
    unsigned long firstTaskRunTime = simMillis() - firstActiveRunStart;
    assertEqual(firstTaskRunTime, 3ul);
    assertEqual(aWait, (unsigned long) (11 - 7 - 3)); // other pending task

    // run some tasks; count them.
    while (simMillis() < start + 1000ul) {
        aWait = timer.tick();
        if(aWait == 0ul) {
            aWait = 1ul;  // at least one millisec
        }
        simDelay(aWait);
    }

    // expect the other task causes some missed deadlines
    assertNear(dt_3millisec.numRuns, 100ul, 9ul); // 7+ millisecs apart
    // (ideally 142 runs)

    assertNear(dt_5millisec.numRuns, 90ul, 4ul); // 11+ millisecs apart
    // (ideally 90 runs)
};


////////////// ... so which sketch is this?
void showID(void)
{
    Serial.println();
    Serial.println(F( "Running " __FILE__ ", Built " __DATE__));
};

//////////////////////////////////////////////////////////////////////////////
void setup() {
    ::delay(1000ul); // wait for stability on some boards to prevent garbage Serial
    Serial.begin(115200ul); // ESP8266 default of 74880 not supported on Linux
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
