/*
 *  condition.h
 *  iJumpman
 *
 *  Created by Andi McClure on 11/28/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

class condition {
    void *underly;
public:
    condition();
    ~condition();
    void lock();
    void unlock();
    void wait();
    void broadcast();
    void signal();
    
    void set(bool &val, bool to) {
        lock();
        val = to;
        broadcast();
        unlock();
    }
    void wait(bool &val, bool become) {
        lock();
        while (val != become)
            wait();
        unlock();
    }
};