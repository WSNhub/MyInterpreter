// A SMING-compatible C interpreter
//
// This is a modified version of:
// A C interpreter for Arduino
// Copyright(c) 2012 Noriaki Mitsunaga.  All rights reserved.
//
// This is a free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// See file LICENSE.txt for further informations on licensing terms.
//

#ifndef __MYINTERPRETER_H__
#define __MYINTERPRETER_H__
#include "Arduino.h"

#define USE_DELEGATES

enum ERRORS {
  STOPPED = 10,
  FOUND_CONTINUE = 1,
  FOUND_BREAK = 2,
  ERROR_DIV0 = -1, 
  ERROR_SYNTAX = -2,
  ERROR_INTERNAL = -3
};

struct ConstValue {
  char *name;
  int   len;
  int   val;
};

#ifdef USE_DELEGATES
typedef Delegate<int(int)> func1Delegate;
typedef Delegate<int(int, int)> func2Delegate;
typedef Delegate<int(int, int, int)> func3Delegate;
#endif

struct Function1 {
  char *name;
  int   len;
#ifdef USE_DELEGATES
  func1Delegate func;
#else
  int (*func)(int);
#endif
};

struct Function2 {
  char *name;
  int   len;
#ifdef USE_DELEGATES
  func2Delegate func;
#else
  int (*func)(int, int);
#endif
};

struct Function3 {
  char *name;
  int   len;
#ifdef USE_DELEGATES
  func3Delegate func;
#else
  int (*func)(int, int, int);
#endif
};

class MyInterpreter
{
  public:
    MyInterpreter();
    ~MyInterpreter() {};

#ifdef USE_DELEGATES
    void registerFunc1(char *name, func1Delegate func);
    void registerFunc2(char *name, func2Delegate func);
    void registerFunc3(char *name, func3Delegate func);
#else
    void registerFunc1(char *name, int (*func)(int));
    void registerFunc2(char *name, int (*func)(int, int));
    void registerFunc3(char *name, int (*func)(int, int, int));
#endif

    void setVariable(char variable, int value);

#ifndef DISABLE_SPIFFS
    bool loadFile(char *fileName);
#endif
    bool load(char *prg, int len);
    void run();

  protected:
    void printError(int err);
    int eval2(char *s, int len, int *val);
    char *skipSpace(char *s, char *e);
    void writeS(const char *s, int len);
    int stepRun();
    int run(char *prg, int len);

  private:
    char scriptBuf[1025];
    int scriptLen;
    int variables[26];
    Vector<struct Function1> func1Handlers;
    Vector<struct Function2> func2Handlers;
    Vector<struct Function3> func3Handlers;
    int runAnimate = 0;
    int runDelay = 0;
    int runStep = 0;
    bool reportProgPos = 0;
};

#endif
