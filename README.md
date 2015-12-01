# MyInterpreter

Original code can be found on http://n.mtng.org/ele/arduino/iarduino.html

Here's an example:

```
//Create the object
MyInterpreter interpreter;

//Provide some handlers
int print(int a)
{
  Serial.println(a);
  return a;
}
int updateSensorState(int node, int sensor, int value)
{
  MyMessage myMsg;
  myMsg.set(value);
  gw.sendRoute(build(myMsg, node, sensor, C_SET, 2 /*message.type*/, 0));
}

//Hook the handlers into the interpreter
interpreter.registerFunc1((char *)"print", print);
interpreter.registerFunc3((char *)"updateSensorState", updateSensorState);

//Prepare local variables for the script
interpreter.setVariable('n', message.sender);
interpreter.setVariable('s', message.sensor);
interpreter.setVariable('v', atoi(message.getString(convBuf)));

//Generate the script itself
char *progBuf = (char *)"if(n==40){if(s==0){print(n);print(s);if(v%2==0){print(v);updateSensorState(n,1,0);}else{updateSensorState(n,1,1);}}}";

//And finally execute
interpreter.run(progBuf, strlen(progBuf));
```
