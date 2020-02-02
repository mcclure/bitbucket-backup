/*
 *  condition.mm
 *  iJumpman
 *
 *  Created by Andi McClure on 11/28/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <Foundation/NSLock.h>
#include "condition.h"

// TODO: Reimplement using pthreads
// See: http://developer.apple.com/iphone/library/documentation/Cocoa/Conceptual/Multithreading/ThreadSafety/ThreadSafety.html#//apple_ref/doc/uid/10000057i-CH8-SW4

condition::condition() {
    underly = [[NSCondition alloc] init];
}
condition::~condition() {
    NSCondition *u = (NSCondition *)underly;
    [u release];
}
void condition::lock() {
    NSCondition *u = (NSCondition *)underly;
    [u lock];
}
void condition::unlock() {
    NSCondition *u = (NSCondition *)underly;
    [u unlock];
}
void condition::wait() {
    NSCondition *u = (NSCondition *)underly;
    [u wait];
}
void condition::broadcast() {
    NSCondition *u = (NSCondition *)underly;
    [u broadcast];
}
void condition::signal() {
    NSCondition *u = (NSCondition *)underly;
    [u signal];
}
