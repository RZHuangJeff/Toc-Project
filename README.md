# Toc-Project

## Description
This is the project for the TOC class. This project is a http server made for interfacing with Line Message API. The main purpose of this project is to make a access control system that can be triggered via chatting with the Linebot.

Following is the finite state machine diagram of the system. The code that defines such finite state machine is at [here](https://github.com/RZHuangJeff/Toc-Project/blob/main/ac_sys/ac_sys.c#L49).
```graphviz
digraph{

}
```

There are three main options valiable for this system.

1. Join: To join a family via providing a family id.
2. Unlock: To unlock a door if there is any.
3. Share: For a family keeper, he can share the permission to his friends easily.


For the description of different parts goes here:

* [Access Control System](#Access-Control-System)
* [Http Server](#Http-Server)
* [Json Parser](#Json-Parser)

## Compilation & Execution
* Compile: ```make```
* Execute: ```./main```

If you are running this project on the server that doesn't have a public IP, maybe you will need something like [ngrok](https://ngrok.com/).

## Access Control System

## Http Server

## Json Parser