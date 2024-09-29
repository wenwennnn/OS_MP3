// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

int compareL1(Thread *a, Thread *b) //MP3
{
    double aRemain = a->getBurstTime() - a->getRunningBrustTime();
    double bRemain = b->getBurstTime() - b->getRunningBrustTime();

    if(aRemain > bRemain) return -1;
    else if(aRemain < bRemain) return 1;
    else return 0;
}

int compareL2(Thread *a, Thread *b) //MP3
{
    if(a->getPriority() > b->getPriority()) return -1;
    else if(a->getPriority() < b->getPriority()) return 1;
    else return 0;
}


Scheduler::Scheduler() //MP3
{ 
    // readyList = new List<Thread *>; 
    readyL1 = new SortedList<Thread *>(compareL1);
    readyL2 = new SortedList<Thread *>(compareL2);
    readyL3 = new List<Thread *>;

    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    // delete readyList; //MP3
    delete readyL1;
    delete readyL2;
    delete readyL3;
} 

void
Scheduler::aging() //MP3
{
    if(!readyL1->IsEmpty()){
        ListIterator<Thread *> *iter = new ListIterator<Thread*>(readyL1);

        for(; !iter->IsDone(); iter->Next()){
            Thread* now = iter->Item();

            if((kernel->stats->totalTicks - now->getAgingTime()) >= 1500 && now->getPriority() < 149){
                now->setAgingTime(kernel->stats->totalTicks);
                int oldP = now->getPriority(), 
                newP = (oldP + 10 > 149) ? 149 : oldP + 10;

                DEBUG('z', "[C] Tick [" << kernel->stats->totalTicks << "]: Thread [" << now->getID() << "] changes its priority from [" << oldP << "] to [" << newP << "]");
                now->setPriority(newP);
                

                //readyL1->Remove(now); //???
                //readyL1->Insert(now);
            }
        }
    }
    if(!readyL2->IsEmpty()){
        ListIterator<Thread *> *iter = new ListIterator<Thread*>(readyL2);

        for(; !iter->IsDone(); iter->Next()){
            Thread* now = iter->Item();

            if((kernel->stats->totalTicks - now->getAgingTime()) >= 1500){
                now->setAgingTime(kernel->stats->totalTicks);
                int oldP = now->getPriority(), 
                newP = (oldP + 10 > 149) ? 149 : oldP + 10;

                DEBUG('z', "[C] Tick [" << kernel->stats->totalTicks << "]: Thread [" << now->getID() << "] changes its priority from [" << oldP << "] to [" << newP << "]");
                now->setPriority(newP);
            
                DEBUG('z', "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << now->getID() << "] is removed from queue L[2]");
                readyL2->Remove(now);

                if(now->getPriority() >= 100) {
                    DEBUG('z', "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << now->getID() << "] is inserted into queue L[1]");
                    readyL1->Insert(now);
                }
                else {
                    DEBUG('z', "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << now->getID() << "] is inserted into queue L[2]");
                    readyL2->Insert(now);
                }
            }
        }
    }
    if(!readyL3->IsEmpty()){
        ListIterator<Thread *> *iter = new ListIterator<Thread*>(readyL2);

        for(; !iter->IsDone(); iter->Next()){
            Thread* now = iter->Item();

            if((kernel->stats->totalTicks - now->getAgingTime()) >= 1500){
                now->setAgingTime(kernel->stats->totalTicks);
                int oldP = now->getPriority(), 
                newP = (oldP + 10 > 149) ? 149 : oldP + 10;

                DEBUG('z', "[C] Tick [" << kernel->stats->totalTicks << "]: Thread [" << now->getID() << "] changes its priority from [" << oldP << "] to [" << newP << "]");
                now->setPriority(newP);
            
                DEBUG('z', "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << now->getID() << "] is removed from queue L[3]");
                readyL3->Remove(now);

                if(now->getPriority() >= 50) {
                    DEBUG('z', "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << now->getID() << "] is inserted into queue L[2]");
                    readyL2->Insert(now);
                }
                else {
                    DEBUG('z', "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << now->getID() << "] is inserted into queue L[3]");
                    readyL3->Append(now);
                }
            }
        }
    }
}

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);
    //readyList->Append(thread);

    //MP3
    thread->setInReadyState(kernel->stats->totalTicks);
    if(thread->getPriority() < 50){
        DEBUG('z', "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getName() << "] is inserted into queue L[3]");
        readyL3->Append(thread);
    }
    else if(thread->getPriority() >= 100){
        DEBUG('z', "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getName() << "] is inserted into queue L[1]");
        readyL1->Insert(thread);
    }
    else{
        DEBUG('z', "[A] Tick [" << kernel->stats->totalTicks << "]: Thread [" << thread->getName() << "] is inserted into queue L[2]");
        readyL2->Insert(thread);
    }
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
/*
    if (readyList->IsEmpty()) {
		return NULL;
    } else {
    	return readyList->RemoveFront();
    }
*/
    //
    if(!readyL1->IsEmpty()){
        Thread *ret = readyL1->RemoveFront();
        DEBUG('z', "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << ret->getID() << "] is remove from queue L[1]");
        return ret;
    }
    else if(!readyL2->IsEmpty()){
        Thread *ret = readyL2->RemoveFront();
        DEBUG('z', "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << ret->getID() << "] is remove from queue L[2]");
        return ret;
    }
    else if(!readyL3->IsEmpty()){
        Thread *ret = readyL3->RemoveFront();
        DEBUG('z', "[B] Tick [" << kernel->stats->totalTicks << "]: Thread [" << ret->getID() << "] is remove from queue L[3]");
        return ret;
    }
    else return NULL;
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());

    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    //readyList->Apply(ThreadPrint);
    readyL1->Apply(ThreadPrint);
    readyL2->Apply(ThreadPrint);
    readyL3->Apply(ThreadPrint);
}
