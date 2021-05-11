/*
 * File: thread.h
 * --------------
 * This file exports a simple, platform-independent thread abstraction,
 * along with simple tools for concurrency control.
 */

#ifndef _thread_h
#define _thread_h

#include <string>

/* Forward definition */

class Lock;

/*
 * Class: Thread
 * -------------
 * This class encapsulates a lightweight process running in the same
 * address space as the creator.  The class itself is opaque and is
 * manipulated by top-level functions as illustrated in the following
 * paradigm:
 *
 *    Thread child = fork(fn);
 *    ... code for the parent thread ...
 *    join(child);
 *
 * This code calls fn so that it runs in parallel with the parent code.
 */

class Thread {

public:

/*
 * Constructor: Thread
 * Usage: Thread thread;
 * ---------------------
 * Creates an inactive thread variable that will typically be overwritten
 * by the result of a fork call.
 */

   Thread();

/*
 * Destructor: ~Thread
 * -------------------
 * Frees any dynamic storage associated with the thread.
 */

   virtual ~Thread();

/*
 * Method: toString
 * Usage: string str = thread.toString();
 * --------------------------------------
 * Converts the thread to a string.
 */

   std::string toString();

/* Private section */

/**********************************************************************/
/* Note: Everything below this point in this class is logically part  */
/* of the implementation and should not be of interest to clients.    */
/**********************************************************************/

   long id;   /* id linking this thread to the platform-specific data */

};

/*
 * Function: fork
 * Usage: Thread child = fork(fn);
 *        Thread child = fork(fn, data);
 * -------------------------------------
 * Creates a child thread that calls fn in an address space shared with the
 * current thread.  The second form makes it possible to pass an argument
 * to fn, which may be of any type.
 */

Thread fork(void (*fn)());

template <typename ClientType>
Thread fork(void (*fn)(ClientType & data), ClientType & data);

/*
 * Function: join
 * Usage: join(thread);
 * --------------------
 * Waits for the specified thread to finish before proceeding.
 */

void join(Thread & thread);

/*
 * Function: yield
 * Usage: yield();
 * ---------------
 * Yields the processor to allow another thread to run.
 */

void yield();

/*
 * Function: getCurrentThread
 * Usage: Thread self = getCurrentThread();
 * ----------------------------------------
 * Returns the currently executing thread.
 */

Thread getCurrentThread();

/*
 * Class: Lock
 * -----------
 * This class represents a simple lock used to control concurrency.  The
 * usual strategy for using locks is to use the synchronized macro
 * described later in this interface.
 */

class Lock {

public:

/*
 * Constructor: Lock
 * Usage: Lock lock;
 * -----------------
 * Initializes a lock, which is initially in the unlocked state.
 */

   Lock();

/*
 * Destructor: ~Lock
 * -----------------
 * Frees any heap storage associated with the lock.
 */

   ~Lock();

/*
 * Method: wait
 * Usage: lock.wait();
 * -------------------
 * Waits for some other thread to call signal on this lock.  This call
 * requires that the lock be held by the calling thread.  The effect of the
 * wait method is to release the lock and then wait until the desired
 * signal operation occurs, at which point the lock is reacquired and
 * control returns from the wait call.  The wait method is typically used
 * inside a critical section containing a while loop to check for a
 * specific condition.  The standard paradigm for using the waitThread
 * function looks like this:
 *
 *    synchronized (lock) {
 *       while (conditional test) {
 *          lock.wait();
 *       }
 *       ... code to manipulate the locked resource ...
 *    }
 */

   void wait();

/*
 * Method: signal
 * Usage: lock.signal();
 * ---------------------
 * Signals all threads waiting on the lock so that they wake up and recheck
 * the corresponding condition.
 */

   void signal();

/**********************************************************************/
/* Note: Everything below this point in this class is logically part  */
/* of the implementation and should not be of interest to clients.    */
/**********************************************************************/

private:

   long id;     /* id linking this lock to the platform-specific data */

   friend class Lock_State;

};

/*
 * Statement: synchronized
 * Usage: synchronized (lock) ...
 * ------------------------------
 * Defines a critical section protected by the specified lock.  The general
 * strategy for using this facility is shown in the following paradigmatic
 * pattern:
 *
 *    synchronized (lock) {
 *       ... statements in the critical section ...
 *    }
 */

void lockForPlatform(int id);
void unlockForPlatform(int id);

class Lock_State {
public:

   Lock_State(Lock & lock) {
      lp = &lock;
      finished = false;
   }

   bool advance() {
      if (finished) {
         unlockForPlatform(lp->id);
         return false;
      } else {
         finished = true;
         lockForPlatform(lp->id);
         return true;
      }
   }

private:
   Lock *lp;
   bool finished;

};

#define synchronized(lock) for (Lock_State ls(lock) ; ls.advance(); )

int forkForPlatform(void (*fn)(void *), void *dp);

struct StartWithVoid {
   void (*fn)();
};

template <typename ClientType>
struct StartWithClientData {
   void (*fn)(ClientType & data);
   ClientType *dp;
};

template <typename ClientType>
static void forkWithClientData(void *arg) {
   StartWithClientData<ClientType> *startup =
      (StartWithClientData<ClientType> *) arg;
   startup->fn(*startup->dp);
}

template <typename ClientType>
Thread fork(void (*fn)(ClientType & data), ClientType & data) {
   StartWithClientData<ClientType> startup = { fn, &data };
   Thread thread;
   thread.id = forkForPlatform(forkWithClientData<ClientType>, &startup);
   return thread;
}

#endif