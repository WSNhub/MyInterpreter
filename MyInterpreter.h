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

struct Function1 {
  char *name;
  int   len;
  int (*func)(int);
};

struct Function2 {
  char *name;
  int   len;
  int (*func)(int, int);
};

struct Function3 {
  char *name;
  int   len;
  int (*func)(int, int, int);
};

class MyInterpreter
{
  public:
    MyInterpreter();
    ~MyInterpreter() {};

    void registerFunc1(char *name, int (*func)(int));
    void registerFunc2(char *name, int (*func)(int, int));
    void registerFunc3(char *name, int (*func)(int, int, int));
    void setVariable(char variable, int value);
    int run(char *prg, int len);

  protected:
    void printError(int err);
    int eval2(char *s, int len, int *val);
    char *skipSpace(char *s, char *e);
    void writeS(const char *s, int len);
    int stepRun();

  private:
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
