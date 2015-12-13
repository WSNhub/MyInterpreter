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

#include "MyInterpreter.h"

//////////////////////////////////////////////////////////////////////////////
// Global variables
//////////////////////////////////////////////////////////////////////////////

const struct ConstValue constants[] = {
  {(char *)"LOW", 3, 0},
  {(char *)"HIGH", 4, 1},
  {(char *)"false", 5, 0},
  {(char *)"true", 4, 1},
};


#define CONST_NUM (sizeof(constants)/sizeof(constants[0]))
#define FUNC2_NUM (sizeof(func2)/sizeof(func2[0]))
#define FUNC3_NUM (sizeof(func3)/sizeof(func3[0]))

MyInterpreter::MyInterpreter()
{
  runAnimate = 0;
  runDelay = 0;
  runStep = 0;
  reportProgPos = 0;

  scriptBuf[0] = 0;
  scriptLen = 0;
};

#ifdef USE_DELEGATES
void MyInterpreter::registerFunc1(char *name, func1Delegate func)
#else
void MyInterpreter::registerFunc1(char *name, int (*func)(int))
#endif
{
  struct Function1 f;
  f.len = strlen(name) + 1; // +1 for '('
  f.name = (char *)malloc(f.len+1); // +1 for null
  if (!f.name)
  {
    Serial.println("registerFunc1 alloc failed");
    return;
  }
  memcpy(f.name, name, strlen(name));
  f.name[f.len-1] = '(';
  f.name[f.len] = 0;
  f.func = func;
  func1Handlers.add(f);
}

#ifdef USE_DELEGATES
void MyInterpreter::registerFunc2(char *name, func2Delegate func)
#else
void MyInterpreter::registerFunc2(char *name, int (*func)(int, int))
#endif
{
  struct Function2 f;
  f.len = strlen(name) + 1; // +1 for '('
  f.name = (char *)malloc(f.len+1); // +1 for null
  if (!f.name)
  {
    Serial.println("registerFunc1 alloc failed");
    return;
  }
  memcpy(f.name, name, strlen(name));
  f.name[f.len-1] = '(';
  f.name[f.len] = 0;
  f.func = func;
  func2Handlers.add(f);
}

#ifdef USE_DELEGATES
void MyInterpreter::registerFunc3(char *name, func3Delegate func)
#else
void MyInterpreter::registerFunc3(char *name, int (*func)(int, int, int))
#endif
{
  struct Function3 f;
  f.len = strlen(name) + 1; // +1 for '('
  f.name = (char *)malloc(f.len+1); // +1 for null
  if (!f.name)
  {
    Serial.println("registerFunc1 alloc failed");
    return;
  }
  memcpy(f.name, name, strlen(name));
  f.name[f.len-1] = '(';
  f.name[f.len] = 0;
  f.func = func;
  func3Handlers.add(f);
}

void MyInterpreter::setVariable(char variable, int value)
{
  int idx;

  if (variable >= 'a' && variable <= 'z')
    idx = variable - 'a';
  else if (variable >= 'A' && variable <= 'Z')
    idx = variable - 'A';
  else
    return;

  variables[idx] = value;
}

int MyInterpreter::stepRun()
{
  return 0;
}

void MyInterpreter::printError(int err)
{
  switch (err)
    {
    case ERROR_DIV0:
      Serial.println("Divided by 0");
      break;
    case ERROR_SYNTAX:
      Serial.println("Syntax error");
      break;
    case STOPPED:
      Serial.println("Stopped");
      break;
    case ERROR_INTERNAL:
    default:
      Serial.println("Syntax error");
      break;
    }
}

int MyInterpreter::eval2(char *s, int len, int *val)
{
  int i, l;
  char *e;
  int v, varn = -1, err;

  //fwrite(s, 1, len, stderr);
  //fprintf(stderr, "\n");

  e = s + len;
  // Skip space
  while (*s != 0 && s<e && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n'))
    s ++;

  if (*s == 0 || s == e) {
    *val = 1;
    return 0;
  }

  if (*s == '(') {
    // ( で始まる場合
    char *p;
    int in = 1;

    s ++;
    while (*s != 0 && s<e && (*s == ' ' || *s == '\t'))
      s ++;
    if (*s == 0 || *s == ')')
      return ERROR_SYNTAX;

    p = s;
    while (*s != 0 && s<e) {
      if (*s == '(') {
	in ++;
      } else if (*s == ')') {
	in --;
	if (in == 0) {
	  s ++;
	  break;
	}
      }
      s ++;
    }
    if (in > 0)
      return ERROR_SYNTAX;
    if ((err = eval2(p, s-p-1, &v)) != 0)
      return err;
  } else if ((*s == '!' && *(s+1) != '=') || *s == '~' || *s == '-' || *s == '+') {
    // トークンが1項演算子の場合
    char *p, op;
    int in = 0;
    op = *s;
    s ++;
    while (*s != 0 && s<e && (*s == ' ' || *s == '\t'))
      s ++;
    if (*s == 0 || s == e)
      return ERROR_SYNTAX;
    p = s;
    while (*s != 0 && s<e && (in>0 || (*s != '+' && *s != '-' && *s != '>' && *s != '<' && *s != '='))) {
      if (*s == '(') {
	in ++;
      } else if (*s == ')') {
	in --;
      }
      s ++;
    }

    if (eval2(p, s-p, &v) != 0)
      return ERROR_SYNTAX;
    if (op == '!')
      v = !v;
    else if (op == '~')
      v = ~v;
    else if (op == '-')
      v = -v;
  } else if (*s>='0' && *s<='9') {
    // トークンが数値の場合
    if (*s == '0' && *(s+1) == 'x') {
      v = 0;
      s += 2;
      while (*s != 0 && s<e && ((*s>='0' && *s<='9') || 
				(*s>='a' && *s<='f') || (*s>='A' && *s<='F'))) {
	if (*s>='0' && *s<='9')
	  v = (v<<4) + (*s - '0');
	else if (*s>='a' && *s<='f')
	  v = (v<<4) + (*s - 'a' + 0xa);
	else if (*s>='A' && *s<='F')
	  v = (v<<4) + (*s - 'A' + 0xa);
	s ++;
      }
    } else if (*s == '0' && *(s+1) == 'b') {
      v = 0;
      s += 2;
      while (*s != 0 && s<e && (*s>='0' && *s<='1')) {
	v = (v<<1) + (*s - '0');
	s ++;
      }
    } else {
      v = 0;
      while (*s != 0 && s<e && *s>='0' && *s<='9') {
	v = v*10 + (*s - '0');
	s ++;
      }
    }
  } else if (((*s>='a' && *s<='z') || (*s>='A' && *s<='Z')) &&
	     (!(*(s+1)>='a' && *(s+1)<='z') && !(*(s+1)>='A' && *(s+1)<='Z')))  {
      // The token is a varibale
    if (*s>='a' && *s<='z')
      varn = *s - 'a';
    else 
      varn = *s - 'A';
    v = variables[varn];
    s ++;
  } else {
    // 定数かどうかをチェックする
    for (i=0; i<CONST_NUM; i++) {
      const struct ConstValue *c = constants+i;
      if (e-s>=c->len && strncmp(c->name, s, 3) == 0) {
	v = c->val;
	s += c->len;
	break;
      }
    }
    // Check if the token is a supported function
    // 引数が1つの関数
    for (i=0; i<func1Handlers.count(); i++) {
      const struct Function1 *f = &func1Handlers[i];
      if (e-s>=f->len && strncmp(f->name, s, f->len) == 0) {
	char *p;
	int in = 1, v1;
	s += f->len;
	p = s;
	while (*s != 0 && s<e && in>0) {
	  if (*s == '(') {
	    in ++;
	  } else if (*s == ')') {
	    in --;
	  }
	  s ++;
	}
	if (in>0)
	  return ERROR_SYNTAX;
	if (eval2(p, s-p-1, &v1) != 0)
	  return ERROR_SYNTAX;
	
#ifdef USE_DELEGATES
	v = f->func(v1);
#else
        v = (*f->func)(v1);
#endif
      }
    }

    // 引数が2つの関数
    for (i=0; i<func2Handlers.count(); i++) {
      const struct Function2 *f = &func2Handlers[i];
      if (e-s>=f->len && strncmp(f->name, s, f->len) == 0) {
	char *p;
	int in = 0, v1, v2;
	s += f->len;
	p = s;
	while (*s != 0 && s<e && !(in == 0 && *s == ',')) {
	  if (*s == '(')
	    in ++;
	  else if (*s == ')')
	    in --;
	  s ++;
	}
	if (*s != ',')
	  return ERROR_SYNTAX;
	if (eval2(p, s-p, &v1) != 0)
	  return ERROR_SYNTAX;

	s ++;
	p = s;
	in = 1;
	while (*s != 0 && s<e && in>0) {
	  if (*s == '(')
	    in ++;
	  else if (*s == ')')
	    in --;
	  s ++;
	}
	if (in>0)
	  return ERROR_SYNTAX;
	if (eval2(p, s-p-1, &v2) != 0)
	  return ERROR_SYNTAX;

#ifdef USE_DELEGATES
	v = f->func(v1, v2);
#else
        v = (*f->func)(v1, v2);
#endif
        break;
      }
    }

    // 引数が3つの関数
    for (i=0; i<func3Handlers.count(); i++) {
      const struct Function3 *f = &func3Handlers[i];
      if (e-s>=f->len && strncmp(f->name, s, f->len) == 0) {
	char *p;
	int in = 0, v1, v2, v3;
	s += f->len;
	p = s;
	while (*s != 0 && s<e && !(in == 0 && *s == ',')) {
	  if (*s == '(')
	    in ++;
	  else if (*s == ')')
	    in --;
	  s ++;
	}
	if (*s != ',')
	  return ERROR_SYNTAX;
	if (eval2(p, s-p, &v1) != 0)
	  return ERROR_SYNTAX;

	s ++;
	p = s;
	in = 0;
	while (*s != 0 && s<e && !(in == 0 && *s == ',')) {
	  if (*s == '(')
	    in ++;
	  else if (*s == ')')
	    in --;
	  s ++;
	}
	if (*s != ',')
	  return ERROR_SYNTAX;
	if (eval2(p, s-p-1, &v2) != 0)
	  return ERROR_SYNTAX;

	s ++;
	p = s;
	in = 1;
	while (*s != 0 && s<e && in>0) {
	  if (*s == '(')
	    in ++;
	  else if (*s == ')')
	    in --;
	  s ++;
	}
	if (in>0)
	  return ERROR_SYNTAX;
	if (eval2(p, s-p-1, &v3) != 0)
	  return ERROR_SYNTAX;

#ifdef USE_DELEGATES
	v = f->func(v1, v2, v3);
#else
        v = (*f->func)(v1, v2, v3);
#endif
        break;
      }
    }
  }

  // 次のトークンを確かめる
    {
      int v2;

      // Skip space
      while (*s != 0 && s<e && (*s == ' ' || *s == '\t'))
	s ++;

      if (*s == 0 || s == e) { // 後ろに演算子はなかった
	*val = v;
	return 0;
      }

      if (*s == '=' && *(s+1) != '=') {
	char *p;
	int op = *s, in = 0;

	s ++;
	while (*s != 0 && s<e && (*s == ' ' || *s == '\t'))
	  s ++;
	if (*s == 0 || s == e)
	  return ERROR_SYNTAX;

	p = s;
	while (*s != 0 && s<e && (in>0 || 
				  !(*s == '=' && *(s+1) == '='))) {
	  if (*s == '(') {
	    in ++;
	  } else if (*s == ')') {
	    in --;
	  }
	  s ++;
	}
	
	if ((err = eval2(p, s-p, &v2)) != 0)
	  return err;

	if (varn < 0 || varn>=sizeof(variables))
	  return ERROR_INTERNAL;
	v = variables[varn] = v2;

	if (*s == 0 || s == e) {
	  *val = v;
	  return 0;
	}
      }


      while (*s == '*' || *s == '/' || *s == '%') {
	char *p;
	int op = *s, in = 0;

	s ++;
	while (*s != 0 && s<e && (*s == ' ' || *s == '\t'))
	  s ++;
	if (*s == 0 || s == e)
	  return ERROR_SYNTAX;

	p = s;
	while (*s != 0 && s<e && (in>0 || 
			   (*s != '*' && *s != '/' && *s != '%' &&
			    *s != '+' && *s != '-' && *s != '>' && *s != '<' &&
			    !(*s == '=' && *(s+1) == '=')  && *s != '!'))) {
	  if (*s == '(') {
	    in ++;
	  } else if (*s == ')') {
	    in --;
	  }
	  s ++;
	}
	
	if ((err = eval2(p, s-p, &v2)) != 0)
	  return err;
	switch (op) {
	case '*':
	  v = v*v2;
	  break;
	case '/':
	  v = v/v2;
	  break;
	case '%':
	  v = v%v2;
	  break;
	}
	if (*s == 0 || s == e) {
	  *val = v;
	  return 0;
	}
      }
	  
      if (*s == '+' || *s == '-') {
	char *p;
	int op = *s, in = 0;

	p = s;
	while (*s != 0 && s<e && (in>0 || (*s != '>' && *s != '<' && *s != '=' && *s != '!'))) {
	  if (*s == '(') {
	    in ++;
	  } else if (*s == ')') {
	    in --;
	  }
	  s ++;
	}
	if (in != 0)
	  return ERROR_SYNTAX;

	// 演算子の右側を計算する
	if ((err = eval2(p, s-p, &v2)) != 0)
	  return err;
	// 左と右を足す
	v = v + v2;
	if (*s == 0 || s==e) {
	  *val = v;
	  return 0;
	}
      }

      if (*s == '>' || *s == '<' || (*s == '=' && *(s+1) == '=')
	  || (*s == '!' && *(s+1) == '=')) {
	char *p;
	int op = *s, op2 = *(s+1), in = 0;

	s ++;
	if (op2 == '=' || op2 == '<' || op2 == '>')
	  s ++;

	p = s;
	while (*s != 0 && s<e && (in>0 || (*s != '&' && *s != '^' && *s != '|'))) {
	  if (*s == '(') {
	    in ++;
	  } else if (*s == ')') {
	    in --;
	  }
	  s ++;
	}
	if (in != 0)
	  return ERROR_SYNTAX;

	// 演算子の右側を計算する
	if ((err = eval2(p, s-p, &v2)) != 0)
	  return err;

	if (op == '>') {
	    if (op2 == '=') {
	      v = v>=v2;
	    } else if (op2 == '>') {
	      v = v>>v2;
	    } else {
	      v = v>v2;
	    }
	} else if (op == '<') {
	  if (op2 == '=') {
	    v = v<=v2;
	  } else if (op2 == '<') {
	    v = v<<v2;
	  } else {
	    v = v<v2;
	  }
	} else if (op == '=' && op2 == '=') {
	  v = (v == v2);
	} else if (op == '!' && op2 == '=') {
	  v = (v != v2);
	}
	if (*s == 0 || s==e) {
	  *val = v;
	  return 0;
	}
      }

      if ((*s == '&' && *(s+1) != '&') || (*s == '|' && *(s+1) != '|') ||
	   *s == '^')  {
	char *p;
	int op = *s, in = 0;

	s ++;
	p = s;
	while (*s != 0 && s<e && (in>0 || !((*s == '&' && *(s+1) == '&') ||
					    (*s == '|' && *(s+1) == '|')))) {
	  if (*s == '(') {
	    in ++;
	  } else if (*s == ')') {
	    in --;
	  }
	  s ++;
	}
	if (in != 0)
	  return ERROR_SYNTAX;

	// 演算子の右側を計算する
	if ((err = eval2(p, s-p, &v2)) != 0)
	  return err;
	if (op == '&')
	  v = v & v2;
	else if (op == '|')
	  v = v | v2;
	else if (op == '^')
	  v = v ^ v2;
	if (*s == 0 || s==e) {
	  *val = v;
	  return 0;
	}
      }

      if ((*s == '&' && *(s+1) == '&') || (*s == '|' && *(s+1) == '|')) {
	int op = *s;

	s += 2;
	// 演算子の右側を計算する
	if ((err = eval2(s, e-s, &v2)) != 0)
	  return err;
	if (op == '&')
	  v = v && v2;
	else if (op == '|')
	  v = v || v2;

	*val = v;
	return 0;
      }

    }
error:
  return ERROR_INTERNAL;
}

char *MyInterpreter::skipSpace(char *s, char *e)
{
  while (*s != 0 && s<e && (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n'))
    s ++;
  return s;
}

int MyInterpreter::run(char *prg, int len)
{
  char *cur, *s, *e;
  int err, in, v, skip;

  s = prg;
  e = prg + len;

  for (;;) {
    char *p, *start;
    start = s = skipSpace(s, e);
    if (*s == 0 || s>=e)
      break;

    WDT.alive();

    skip = 0;
    // fprintf(stderr, s);

    if ((strncmp(s, "continue", 8) == 0
	 && (*(s+8) == ';' ||
	     *(s+8) == ' ' || *(s+8) == '\t' || 
	     *(s+8) == '\r' || *(s+8) == '\n')) ||
	(strncmp(s, "break", 5) == 0
	 && (*(s+5) == ';' ||
	     *(s+5) == ' ' || *(s+5) == '\t' || 
	     *(s+5) == '\r' || *(s+5) == '\n'))) {
      char *cond, *cond_e, *p1, *statement, *statement_e;
      char *statement2 = NULL, *statement2_e;
      int continue_break = 0;

      p1 = s;
      if (strncmp(s, "continue",8) == 0) {
	continue_break = 1;
	s += 8;
      } else {
	s += 5;
      }
      s = skipSpace(s, e);
      if (*s != ';') {
	writeS(p1, 20);
	return ERROR_SYNTAX;
      }
      s ++;

      if (continue_break == 0)
	return FOUND_BREAK;
      return FOUND_CONTINUE;
    } else if ((strncmp(s, "while", 5) == 0
	 && (*(s+5) == '(' ||
	     *(s+5) == ' ' || *(s+5) == '\t' || 
	     *(s+5) == '\r' || *(s+5) == '\n')) ||
	(strncmp(s, "if", 2) == 0
	 && (*(s+2) == '(' ||
	     *(s+2) == ' ' || *(s+2) == '\t' || 
	     *(s+2) == '\r' || *(s+2) == '\n'))) {
      char *cond, *cond_e, *p1, *statement, *statement_e;
      char *statement2 = NULL, *statement2_e;
      int while_if = 0;
      if (strncmp(s, "while", 5) == 0) {
	while_if = 1;
	s += 5;
      } else
	s += 2;
      p1 = s;
      s = skipSpace(s, e);
      if (*s != '(') {
	writeS(p1, 20);
	return ERROR_SYNTAX;
      }
      s ++;
      cond = s;
      in = 1;
      while (*s !=0 && s<e && (in>0 || *s != ')')) {
	if (*s == '(')
	  in ++;
	if (*s == ')') {
	  in --;
	  if (in == 0)
	    break;
	}
	s ++;
      }
      if (*s != ')') {
	writeS(p1, s-p1);
	return ERROR_SYNTAX;
      }
      cond_e = s;
      s ++;
      s = skipSpace(s, e);
      if (*s == 0 || s>=e) {
	writeS(p1, s-p1);
	return ERROR_SYNTAX;
      }

      statement = s;
      in = 0;
      while (*s !=0 && s<e && (in>0 || (*s != '}' && *s != ';'))) {
	if (*s == '{')
	  in ++;
	else if (*s == '}') {
	  in --;
	  if (in == 0)
	    break;
	}
	s ++;
      }
      if (*s == 0 || s>=e || (*s != ';' && *s != '}') || in > 0) {
	writeS(p1, s-p1);
	return ERROR_SYNTAX;
      }
      s ++;
      statement_e = s;
      // 必要なら else 部分を取り出す
      s = skipSpace(s, e);
      // fprintf(stderr, "*1 %s\n", s);
      if (while_if == 0 && strncmp(s, "else", 4) == 0
	  && (*(s+4) == '{' ||
	      *(s+4) == ' ' || *(s+4) == '\t' || 
	      *(s+4) == '\r' || *(s+4) == '\n')) {
	s += 4;
	s = skipSpace(s, e);
	if (*s == 0 || s>=e) {
	  writeS(p1, s-p1);
	  return ERROR_SYNTAX;
	}
	//fprintf(stderr, "*2 %s\n", s);
	statement2 = s;
	in = 0;
	while (*s !=0 && s<e && (in>0 || (*s != '}' && *s != ';'))) {
	  if (*s == '{')
	    in ++;
	  else if (*s == '}') {
	    in --;
	    if (in == 0)
	      break;
	  }
	  s ++;
	}
	if (*s == 0 || s>=e || (*s != ';' && *s != '}') || in > 0) {
	  writeS(p1, s-p1);
	  return ERROR_SYNTAX;
	}
	statement2_e = s;
      }

      if (while_if>0) {
	// while 文の処理
	if (!reportProgPos && (runAnimate || runStep)) {
	  Serial.print("while (");
	  writeS(cond, cond_e-cond);
	  Serial.print(")\n");
	}
	if (!reportProgPos && (err = stepRun()) != 0)
	  return err;
	for (;;) {
	  err = eval2(cond, cond_e-cond, &v);
	  if (err)
	    return err;
	  if (!reportProgPos && (runAnimate || runStep)) {
	    writeS(cond, cond_e-cond);
	    Serial.print(": ");
	    Serial.print(v ? "true" : "false");
	    Serial.println("");
	  }
	  //fwrite(cond, 1, cond_e-cond, stderr);
	  //fprintf(stderr, ": %d\n", v);
	  if (v == 0)
	    break;
	  if (!reportProgPos && (runAnimate || runStep)) {
	    if (err = stepRun() != 0)
	      return err;
	  }
	  err = run(statement, statement_e-statement);
	  if (err == FOUND_CONTINUE)
	    continue;
	  else if (err == FOUND_BREAK)
	    break;
	  else if (err)
	    return err;
	}
      } else {
	// if 文の処理
	if (!reportProgPos && (runAnimate || runStep)) {
	  Serial.print("if (");
	  writeS(cond, cond_e-cond);
	  Serial.print(")");
	}
	err = eval2(cond, cond_e-cond, &v);
	if (err) {
	  if (!reportProgPos && (runAnimate || runStep))
	    Serial.println("");
	  return err;
	}
	if (!reportProgPos && (runAnimate || runStep)) {
	  Serial.print(": ");
	  Serial.print(v ? "true" : "false");
	  Serial.println("");
	}
	//fwrite(cond, 1, cond_e-cond, stderr);
	//fprintf(stderr, ": %d\n", v);
	if (v != 0) {
	  err = run(statement, statement_e-statement);
	  if (err)
	    return err;
	} else {
	  if (statement2 != NULL) {
	    err = run(statement2, statement2_e-statement2);
	    if (err)
	      return err;
	  }
	}
      }
    } else if ((strncmp(s, "for", 3) == 0
	 && (*(s+3) == '(' ||
	     *(s+3) == ' ' || *(s+3) == '\t' || 
	     *(s+3) == '\r' || *(s+3) == '\n'))) {
      char *init, *init_e;
      char *cond, *cond_e, *inc, *inc_e;
      char *statement, *statement_e, *p1;
      p1 = s;
      s += 3;
      s = skipSpace(s, e);
      if (*s != '(') {
	writeS(p1, 20);
	return ERROR_SYNTAX;
      }
      s ++;
      s = skipSpace(s, e);
      init = s;
      while (*s !=0 && s<e && *s != ';')
	s ++;
      if (*s == 0 || s>=e) {
	writeS(p1, 20);
	return ERROR_SYNTAX;
      }
      init_e = s;
      s ++;
      s = skipSpace(s, e);
      cond = s;
      while (*s !=0 && s<e && *s != ';')
	s ++;
      if (*s == 0 || s>=e) {
	writeS(p1, s-p1);
	return ERROR_SYNTAX;
      }
      cond_e = s;
      s ++;
      inc = s = skipSpace(s, e);
      in = 1;
      while (*s !=0 && s<e && (in>0 || *s != ')')) {
	if (*s == '(')
	  in ++;
	else if (*s == ')') {
	  in --;
	  if (in == 0)
	    break;
	}
	s ++;
      }
      if (*s == 0 || s>=e) {
	writeS(p1, s-p1);
	return ERROR_SYNTAX;
      }
      inc_e = s;
      s ++;
      s = skipSpace(s, e);
      statement = s;
      in = 0;
      while (*s !=0 && s<e && (in>0 || (*s != '}' && *s != ';'))) {
	if (*s == '{')
	  in ++;
	else if (*s == '}') {
	  in --;
	  if (in == 0)
	    break;
	}
	s ++;
      }
      if (*s == 0 || s>=e || (*s != ';' && *s != '}') || in > 0) {
	writeS(p1, s-p1);
	return ERROR_SYNTAX;
      }
      s ++;
      statement_e = s;

      // for 文の処理
      if (runAnimate || runStep) {
	Serial.print("for (");
	writeS(init, init_e-init);
	Serial.print("; ");
	writeS(cond, cond_e-cond);
	Serial.print("; ");
	writeS(inc, inc_e-inc);
	Serial.print(")\n");
      }
      if (!reportProgPos && (runAnimate || runStep)) {
	if ((err = stepRun()) != 0)
	  return err;
	writeS(init, init_e-init);
	Serial.println("");
      }
      if ((err = eval2(init, init_e-init, &v)) != 0) {
	return err;
      }
      if (!reportProgPos && (runAnimate || runStep))
	if(err = stepRun() != 0)
	  return err;
      for (;;) {
	if ((err = eval2(cond, cond_e-cond, &v)) != 0) 
	  return err;
	if (!reportProgPos && (runAnimate || runStep)) {
	  writeS(cond, cond_e-cond);
	  Serial.print(": ");
	  Serial.print(v ? "true" : "false");
	  Serial.println("");
	}
	if (v == 0)
	  break;
	//fwrite(statement, 1, statement_e-statement, stderr);
	if ((err = run(statement, statement_e-statement)) != 0) {
	  if (err == FOUND_CONTINUE)
	    ; /* DO_NOTHING */
	  else if (err == FOUND_BREAK)
	    break;
	  else return err;
	}
	if (!reportProgPos && (runAnimate || runStep)) {
	  writeS(inc, inc_e-inc);
	  Serial.println("");
	}
	if ((err = eval2(inc, inc_e-inc, &v)) != 0)
	  return err;
	if (runAnimate || runStep) {
	  if ((err = stepRun()) != 0)
	    return err;
	}
      }
    } else if (*s == '{' || *s == '}') {
      s ++;
      skip = 1;
    } else { // 式文の処理
      p = s;
      in = 0;
      while (*s != 0 && s<e && (in>0 || *s != ';'))
	s ++;
      if (*s == 0 && in>0) {
	writeS(p, s-p);
	return ERROR_SYNTAX;
      }
      if (s != p) {
	if (!reportProgPos && (runAnimate != 0 || runStep)) {
	  writeS(p, s-p);
	  Serial.println("");
	}
	if ((err = eval2(p, s-p, &v)) != 0) {
	  writeS(p, s-p);
	  Serial.println("");
	  return err;
	}
      } else
	skip = 1;
      s ++;
#if 0
      printf("%d\n", v);
#endif
      if (skip == 0) {
	if ((err = stepRun()) != 0)
	  return err;
      }
    }
  }
  return 0;
}

void MyInterpreter::writeS(const char *s, int len)
{
  Serial.print(s);
}

bool MyInterpreter::load(char *prg, int len)
{
    if (len > sizeof(scriptBuf) - 1)
    {
        debugf("Scripts exceeds max length of %d bytes", sizeof(scriptBuf) - 1);
        return false;
    }

    scriptLen = len;
    memcpy(scriptBuf, prg, len);
    scriptBuf[scriptLen] = 0;

    return true;
}

#ifndef DISABLE_SPIFFS
bool MyInterpreter::loadFile(char *fileName)
{
    if (!fileExist(fileName))
    {
        debugf("Script file %s does not exist", fileName);
        return false;
    }

    if (fileGetSize(fileName) > sizeof(scriptBuf) - 1)
    {
        debugf("Scripts exceeds max length of %d bytes", sizeof(scriptBuf) - 1);
        return false;
    }

    scriptLen = fileGetSize(fileName);
    fileGetContent(fileName, scriptBuf, sizeof(scriptBuf));
    scriptBuf[scriptLen] = 0;

    return true;
}
#endif

void MyInterpreter::run()
{
    if (!scriptLen)
        return;

    run(scriptBuf, scriptLen);
}
