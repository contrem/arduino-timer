// arduino-timer unit tests
// Arduino "AUnit" library required

// Required for UnixHostDuino emulation
#include <Arduino.h>

#if defined(UNIX_HOST_DUINO)
#  ifndef ARDUINO
#  define ARDUINO 100
#  endif
#endif

#include <AUnit.h>
#include <arduino-timer.h>

class Clock {
  public:
    Clock(unsigned long start = 0UL) : _time(start) {}

    unsigned long tick(unsigned long amount = 1) { return inc(amount); }
    unsigned long inc(unsigned long amount = 1) { return _time += amount; }
    unsigned long dec(unsigned long amount = 1) { return _time -= amount; }
    unsigned long set(unsigned long time) { return _time = time; }

    void reset() { _time = 0UL; }

    unsigned long millis() { return _time; }
    unsigned long micros() { return _time; }
    unsigned long time() { return _time; }

    void delay(unsigned long amount) { inc(amount); }
  private:
    unsigned long _time;
};

namespace CLOCK {
    Clock clock;

    unsigned long
    tick(unsigned long amount = 1)
    {
        return clock.tick(amount);
    }

    unsigned long
    set(unsigned long t)
    {
        return clock.set(t);
    }

    unsigned long
    millis()
    {
        return clock.millis();
    }

    void
    delay(unsigned long amount)
    {
        return clock.delay(amount);
    }

    void
    reset()
    {
        clock.reset();
    }
};

using namespace CLOCK;

struct Task {
    Clock *clock;

    unsigned long duration,
                  runs,
                  last_run;
    bool repeat;

    Task(
        Clock *clock,
        unsigned long duration,
        bool repeat = true
    ) : clock(clock),
        duration(duration),
        runs(0UL),
        last_run(0UL),
        repeat(repeat)
    {}

    bool
    run(Task *opaque)
    {
        last_run = this->clock->millis();
        this->clock->inc(duration);
        ++runs;

        if (this != opaque) {
             ;
        }

        return repeat;
    }
};

bool handler(Task *t)
{
    return t->run(t);
}

Task
make_task(unsigned long duration = 0UL, bool repeat = true)
{
    return Task(&CLOCK::clock, duration, repeat);
}

void
pre_test()
{
    CLOCK::reset();
}

test(timer) {
    auto timer = timer_create_default();
    assertEqual(timer.ticks(), 0UL);
}

test(timer_in) {
    pre_test();

    Timer<0x1, CLOCK::millis, Task *> timer;
    const unsigned long in = 3UL;

    auto task = make_task();
    const auto r = timer.in(in, handler, &task);

    // assert task added
    assertNotEqual((unsigned long)r, 0UL);
    // assert pending task time
    assertEqual(in, timer.ticks());

    // assert task has not run
    assertEqual(task.runs, 0UL);

    // tick forward in - 1 times
    for (unsigned long i = 1UL; i < in; ++i) {
        CLOCK::tick();

        assertEqual(in - i, timer.ticks());
        assertEqual(in - i, timer.tick());

        // assert task has not run
        assertEqual(task.runs, 0UL);
    }

    CLOCK::tick();

    // assert task is about to run
    assertEqual(0UL, timer.ticks());
    assertEqual(0UL, timer.tick());

    // assert task ran one time
    assertEqual(task.runs, 1UL);

    CLOCK::tick();
    assertEqual(0UL, timer.ticks());
    assertEqual(0UL, timer.tick());

    // assert task did not run again
    assertEqual(task.runs, 1UL);
}

test(timer_at) {
    pre_test();

    Timer<0x1, CLOCK::millis, Task *> timer;
    const unsigned long in = 3UL;

    auto task = make_task();
    const auto r = timer.at(in, handler, &task);

    // assert task added
    assertNotEqual((unsigned long)r, 0UL);
    // assert pending task time
    assertEqual(in, timer.ticks());

    // assert task has not run
    assertEqual(task.runs, 0UL);

    // tick forward in - 1 times
    for (unsigned long i = 1UL; i < in; ++i) {
        CLOCK::tick();

        assertEqual(in - i, timer.ticks());
        assertEqual(in - i, timer.tick());

        // assert task has not run
        assertEqual(task.runs, 0UL);
    }

    CLOCK::tick();

    // assert task is about to run
    assertEqual(0UL, timer.ticks());
    assertEqual(0UL, timer.tick());

    // assert task ran one time
    assertEqual(task.runs, 1UL);

    CLOCK::tick();
    assertEqual(0UL, timer.ticks());
    assertEqual(0UL, timer.tick());

    // assert task did not run again
    assertEqual(task.runs, 1UL);
}

test(timer_every) {
    pre_test();

    Timer<0x1, CLOCK::millis, Task *> timer;
    const unsigned long every = 2UL, until = 9UL;;
    auto task = make_task();
    const auto r = timer.every(every, handler, &task);

    // assert task added
    assertNotEqual((unsigned long)r, 0UL);
    // assert pending task time
    assertEqual(every, timer.ticks());

    // assert task has not run
    assertEqual(task.runs, 0UL);

    for (unsigned long i = 0UL; i < until; ++i) {
        unsigned long period = i ? i % every : every;

        assertEqual(period, timer.ticks());
        const auto ticks = timer.tick();
        assertEqual(ticks, timer.ticks());

        CLOCK::tick();
    }

    // assert task ran correct number of times
    assertEqual(task.runs, until / every);
}

test(timer_cancel) {
    pre_test();

    Timer<0x1, CLOCK::millis, Task *> timer;
    const unsigned long in = 3UL;

    auto task = make_task();
    auto r = timer.in(in, handler, &task);

    // assert task added
    assertNotEqual((unsigned long)r, 0UL);
    // assert pending task time
    assertEqual(in, timer.ticks());

    // assert task has not run
    assertEqual(task.runs, 0UL);

    timer.cancel(r);

    // assert task cleared
    assertEqual((unsigned long)r, 0UL);

    // assert task will not run
    assertEqual(timer.ticks(), 0UL);

    CLOCK::tick(in);

    // assert task did not run
    assertEqual(timer.tick(), 0UL);
    assertEqual(task.runs, 0UL);
}

test(timer_cancel_all) {
    pre_test();

    Timer<0x2, CLOCK::millis, Task *> timer;
    const unsigned long in = 3UL;

    auto task = make_task();
    auto r = timer.in(in, handler, &task);
    auto r2 = timer.in(in, handler, &task);

    // assert task added
    assertNotEqual((unsigned long)r, 0UL);
    assertNotEqual((unsigned long)r2, 0UL);
    assertNotEqual((unsigned long)r, (unsigned long)r2);

    // assert pending task time
    assertEqual(in, timer.ticks());

    // assert task has not run
    assertEqual(task.runs, 0UL);

    timer.cancel();

    // assert task will not run
    assertEqual(timer.ticks(), 0UL);

    CLOCK::tick(in);

    // assert task did not run
    assertEqual(timer.tick(), 0UL);
    assertEqual(task.runs, 0UL);
}

test(timer_ticks) {
    pre_test();

    Timer<0x2, CLOCK::millis, Task *> timer;
    const unsigned long small = 3UL, big = 13UL;
    unsigned long delta = 0;

    auto small_task = make_task();
    auto big_task = make_task();

    auto r = timer.in(small, handler, &small_task);
    auto r2 = timer.every(big, handler, &big_task);

    // assert task added
    assertNotEqual((unsigned long)r, 0UL);
    assertNotEqual((unsigned long)r2, 0UL);

    assertEqual(small, timer.ticks()); // small pending

    // assert tasks have not run
    assertEqual(small_task.runs, 0UL);
    assertEqual(big_task.runs, 0UL);

    CLOCK::tick(small);

    assertEqual(0UL, timer.ticks()); // small ready to run

    timer.tick(); // run small

    assertEqual(small_task.runs, 1UL); // small ran once
    assertEqual(big_task.runs, 0UL); // big has not run

    delta = big - small;
    assertEqual(delta, timer.ticks()); // big pending

    CLOCK::tick(); // tick one forward
    --delta;
    assertEqual(delta, timer.ticks()); // check the reduction

    CLOCK::tick(delta);
    assertEqual(0UL, timer.ticks()); // big ready to run

    timer.tick(); // run big

    assertEqual(small_task.runs, 1UL); // small ran once only
    assertEqual(big_task.runs, 1UL); // big ran once

    assertEqual(big, timer.ticks()); // big pending again
}

test(timer_size) {
    pre_test();

    Timer<0x2, CLOCK::millis, Task *> timer;

    assertEqual(timer.size(), 0UL);

    auto t = make_task();

    auto r = timer.in(0UL, handler, &t);

    assertNotEqual((unsigned long)r, 0UL);
    assertEqual(timer.size(), 1UL);

    r = timer.in(0UL, handler, &t);

    assertNotEqual((unsigned long)r, 0UL);
    assertEqual(timer.size(), 2UL);

    timer.cancel();

    assertEqual(timer.size(), 0UL);
}

test(timer_empty) {
    pre_test();

    Timer<0x1, CLOCK::millis, Task *> timer;

    assertEqual(timer.empty(), true);

    auto t = make_task();

    auto r = timer.in(0UL, handler, &t);

    assertNotEqual((unsigned long)r, 0UL);
    assertEqual(timer.empty(), false);

    timer.cancel();

    assertEqual(timer.empty(), true);
}

test(timer_rollover_every) {
    pre_test();

    Timer<0x1, CLOCK::millis, Task *> timer;
    const unsigned long every = 2UL, until = 257UL, rollover = 32UL;
    auto task = make_task();
    const auto r = timer.every(every, handler, &task);

    // assert task added
    assertNotEqual((unsigned long)r, 0UL);
    // assert pending task time
    assertEqual(every, timer.ticks());

    // assert task has not run
    assertEqual(task.runs, 0UL);

    for (unsigned long i = 0UL; i < until; ++i) {
        unsigned long period = i ? i % every : every;

        assertEqual(period, timer.ticks());
        const auto ticks = timer.tick();
        assertEqual(ticks, timer.ticks());

        CLOCK::tick();

        // rollover / overflow clock back to 0
        CLOCK::set(CLOCK::millis() % rollover);
    }

    // assert task ran correct number of times
    assertEqual(task.runs, until / every);
}

test(timer_rollover) {
    pre_test();

    Timer<0x1, CLOCK::millis, Task *> timer;
    const unsigned long in = 2UL;

    CLOCK::set((unsigned long)-1); // start clock at max value

    auto task = make_task();
    const auto r = timer.in(in, handler, &task);

    // assert task added
    assertNotEqual((unsigned long)r, 0UL);
    // assert pending task time
    assertEqual(in, timer.ticks());

    // assert task has not run
    assertEqual(task.runs, 0UL);

    // roll clock over - one tick
    CLOCK::tick();
    assertEqual(in - 1UL, timer.ticks());

    timer.tick();

    // assert task has not run
    assertEqual(task.runs, 0UL);

    CLOCK::tick();
    assertEqual(0UL, timer.ticks()); // ready to run
    timer.tick();

    // assert task ran
    assertEqual(task.runs, 1UL);
}

void
sketch(void)
{
    Serial.println();
    Serial.println(F("Running " __FILE__ ", Built " __DATE__));
}

void
setup()
{
    ::delay(1000UL); // wait for stability on some boards to prevent garbage Serial
    Serial.begin(115200UL); // ESP8266 default of 74880 not supported on Linux
    while (!Serial); // for the Arduino Leonardo/Micro only
    sketch();
}

void
loop()
{
    // Should get:
    // TestRunner summary:
    //    <n> passed, <n> failed, <n> skipped, <n> timed out, out of <n> test(s).
    aunit::TestRunner::run();
}
