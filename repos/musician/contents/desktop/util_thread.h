/*
 *  util_thread.h
 *  Jumpcore
 *
 *  Created by Andi McClure on 1/31/14.
 *  Copyright 2014 Run Hello. All rights reserved.
 *
 */
 
#ifndef _UTIL_THREAD_H
#define _UTIL_THREAD_H
 
#include "kludge.h"

struct thread_lock {
	pthread_mutex_t raw;
	thread_lock();
	~thread_lock();
	void lock();
	void unlock();
	bool tryLock();
};

struct locked {
	thread_lock &target;
	locked(thread_lock &_target);
	~locked();
};

struct try_locked {
	thread_lock &target;
	bool success;
	try_locked(thread_lock &_target);
	~try_locked();
	operator bool() const { return success; }
};

template<class T, int N>
struct array_swap {
	T* output;
	T* input;
	thread_lock inuse;
	
	array_swap() {
		output = new T[N];
		input = new T[N];
	}
	~array_swap() {
		delete[] input;
		delete[] output;
	}
	void swap() {
		T *temp = output;
		output = input;
		input = temp;
	}
};

template<class T>
struct scalar_swap {
	T* output;
	T* input;
	thread_lock inuse;
	
	scalar_swap() {
		output = new T;
		input = new T;
	}
	~scalar_swap() {
		delete input;
		delete output;
	}
	void swap() {
		T *temp = output;
		output = input;
		input = temp;
	}
};

#endif _UTIL_THREAD_H