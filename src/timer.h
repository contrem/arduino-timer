/**
   arduino-timer - library for delaying function calls

   Copyright (c) 2018, Michael Contreras
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _CM_ARDUINO_TIMER_H__
#define _CM_ARDUINO_TIMER_H__

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#ifndef TIMER_MAX_TASKS
#define TIMER_MAX_TASKS 0x10
#endif

//#define TIMER_DEBUG

template <
    size_t max_tasks = TIMER_MAX_TASKS,   /* max allocated tasks */
    unsigned long (*time_func)() = millis /* time function for timer */
    >
class Timer
{
public:
    Timer()
    {
        // initialize the min heap references
        for (size_t i = 0; i < max_tasks; ++i)
        {
            heap[i] = i;
        }
    }

    typedef unsigned long Task;                                /* public task handle */
    typedef bool (*handler_t)(void *opaque, long overdue_by); /* task handler func signature */

    /* Calls handler with opaque as argument in delay units of time */
    Task
    in(unsigned long delay, handler_t h, void *opaque = NULL)
    {
        return add_task(h, opaque, time_func() + delay);
    }

    /* Calls handler with opaque as argument at time */
    Task
    at(unsigned long time, handler_t h, void *opaque = NULL)
    {
        return add_task(h, opaque, time);
    }

    /* Calls handler with opaque as argument every interval units of time */
    Task
    every(unsigned long interval, handler_t h, void *opaque = NULL)
    {
        return add_task(h, opaque, time_func() + interval, interval);
    }

    /* Cancel the timer task */
    bool
    cancel(Task task)
    {
        for (size_t i = 0; i < ctr; i++)
        {
            if (tasks[heap[i]].id == task)
            {
                del_task(i);
                return true;
            }
        }
        return false;
    }

    /* Ticks the timer forward - call this function in loop() */
    unsigned long
    tick()
    {
        while (ctr)
        {
#ifdef TIMER_DEBUG
            dump(true);
#endif
            const unsigned long now = time_func();
            // get the most pressing task
            struct task *task = &tasks[heap[0]];
            // calculate how much time is left
            const long remaining = task->expires - now;
            if (remaining > 0)
            {
#ifdef TIMER_DEBUG
                // slow the output a little bit
                delay(min(200, remaining));
#endif
                // not yet. This is the hot path
                return remaining;
            }

            // run the handler for who knows how long
            const bool again = task->handler(task->opaque, -remaining);

            if (task->repeat && again)
            {
                // reschedule
                while (task->expires <= time_func())
                {
                    // increament the expires date to avoid drift because
                    // of time spent in handler
                    task->expires = task->expires + task->repeat;
                }
                bubbleDown(0);
            }
            else
            {
                del_task(0);
            }
        }
        return 0;
    }

private:
    /* count of tasks in the heap */
    size_t ctr;
    /* each task is given a unique id */
    unsigned long max_id = 1;
    /*
     index in to tasks array in min heap order. tasks[heap[0]] is always the
     soonest task to expire
    */
    size_t heap[max_tasks];

    struct task
    {
        handler_t handler;     /* task handler callback func */
        void *opaque;          /* argument given to the callback handler */
        Task id;               /* unique id */
        unsigned long expires; /* when the task expires */
        unsigned long repeat;  /* repeat task */
    } tasks[max_tasks];

#ifdef TIMER_DEBUG
    void
    dump(bool check)
    {
        unsigned long now = time_func();
        Serial.print("\ttime:");
        Serial.print(now);
        Serial.print(" count:");
        Serial.print(ctr);
        Serial.print(" heap:[");
        bool valid = true;
        for (int i = 0; i < max_tasks; i++)
        {
            if (i == ctr)
                Serial.print("___ ");
            size_t idx = heap[i];
            struct task *task = &tasks[idx];
            Serial.print(idx);
            Serial.print(":");
            long remaining = i < ctr ? task->expires - now : task->expires;
            Serial.print(remaining);
            Serial.print(" ");

            int p = (i - 1) / 2;
            if (i < ctr && i && tasks[heap[p]].expires > tasks[idx].expires)
                valid = false;
        }
        Serial.println(valid ? "]" : check ? "] ----- ERROR" : "]*");
    }
#endif

    inline Task
    add_task(handler_t handler, void *opaque, unsigned long expires, unsigned long repeat = 0)
    {
        if (ctr >= max_tasks)
            return 0;

#ifdef TIMER_DEBUG
        Serial.print("add in ");
        Serial.print(expires - time_func());
        Serial.print(" every ");
        Serial.println(repeat);
#endif

        // tac the new task on to the end of the heap
        const size_t slot = ctr++;
        struct task *task = &tasks[heap[slot]];

        task->id = max_id++;
        task->handler = handler;
        task->opaque = opaque;
        task->expires = expires;
        task->repeat = repeat;

        // bubble up the min heap to the right place
        bubbleUp(slot);

        return task->id;
    }

    inline void
    del_task(size_t slot)
    {
        struct task *task = &tasks[heap[slot]];
        task->id = 0;
        task->handler = NULL;
        task->opaque = NULL;
        task->expires = 0;
        task->repeat = 0;
        // put task in free pool by exchanging it with far future task and
        // decrementing the count
        swap(slot, --ctr);
        // check invariants
        bubbleDown(slot);
    }

    inline void
    bubbleUp(size_t slot)
    {
#ifdef TIMER_DEBUG
        Serial.print("\tup ");
        Serial.println(slot);
        dump(false);
#endif
        size_t parent = (slot - 1) / 2;
        while (slot && tasks[heap[parent]].expires > tasks[heap[slot]].expires)
        {
            swap(slot, parent);
            slot = parent;
            parent = (slot - 1) / 2;
        }
    }

    inline void
    bubbleDown(size_t slot)
    {
#ifdef TIMER_DEBUG
        Serial.print("\tdown ");
        Serial.println(slot);
        dump(false);
#endif
        bool done = false;
        while (!done)
        {
            if (slot >= ctr)
            {
                return;
            }
            size_t smallest = slot;

            const size_t left = 2 * slot + 1;
            if (left < ctr && tasks[heap[left]].expires < tasks[heap[smallest]].expires)
            {
                // left side is expiring sooner
                smallest = left;
            }

            const size_t right = 2 * slot + 2;
            if (right < ctr && tasks[heap[right]].expires < tasks[heap[smallest]].expires)
            {
                // right side is expiring soonest
                smallest = right;
            }

            if (smallest != slot)
            {
                swap(slot, smallest);
                // loop again with slot set to where the smallest was
                slot = smallest;
            }
            else
            {
                done = true;
            }
        }
    }

    inline void
    swap(int a, int b)
    {
#ifdef TIMER_DEBUG
        Serial.print("\tswap ");
        Serial.print(a);
        Serial.print("<->");
        Serial.println(b);
#endif
        size_t tmp = heap[a];
        heap[a] = heap[b];
        heap[b] = tmp;
    }
};

/* create a timer with the default settings */
inline Timer<>
timer_create_default()
{
    return Timer<>();
}

#endif
