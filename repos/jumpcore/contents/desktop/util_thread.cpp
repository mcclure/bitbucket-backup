/*
 *  util_thread.cpp
 *  Jumpcore
 *
 *  Created by Andi McClure on 1/31/14.
 *  Copyright 2014 Run Hello. All rights reserved.
 *
 */

#include "util_thread.h"

thread_lock::thread_lock() {
	pthread_mutex_init (&raw, NULL);
}
thread_lock::~thread_lock() {
	pthread_mutex_destroy(&raw);
}
void thread_lock::lock() {
	pthread_mutex_lock(&raw);
}
void thread_lock::unlock() {
	pthread_mutex_unlock(&raw);
}
bool thread_lock::tryLock() {
	return !pthread_mutex_trylock(&raw);
}

locked::locked(thread_lock &_target) : target(_target) {
	target.lock();
}
locked::~locked() {
	target.unlock();
}

try_locked::try_locked(thread_lock &_target) : target(_target) {
	success = target.tryLock();
}
try_locked::~try_locked() {
	if (success)
		target.unlock();
}
