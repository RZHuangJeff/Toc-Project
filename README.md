# Toc-Project

## Description
This is the project for the TOC class. This project is a http server made for interfacing with Line Message API. The main purpose of this project is to make a access control system that can be triggered via chatting with the Linebot.

Following is the finite state machine diagram of the system. The code that defines such finite state machine is at [here](https://github.com/RZHuangJeff/Toc-Project/blob/main/ac_sys/ac_sys.c#L49).

![](https://github.com/RZHuangJeff/Toc-Project/blob/main/ac_sys_fsm_diag.png)

There are three main options valiable for this system.

1. Join: To join a family via providing a family id.
2. Unlock: To unlock a door if there is any.
3. Share: For a family keeper, he can share the permission to his friends easily.

The description of different parts goes here:

* [Access Control System](#Access-Control-System)
* [Http Server](#Http-Server)
* [Json Parser](#Json-Parser)

## Compilation & Execution
* Compile: ```make```
* Execute: ```./main```

If you are running this project on the server that doesn't have a public IP, maybe you will need something like [ngrok](https://ngrok.com/).

## Access Control System
This part defines the main finite state machine of the access control system. It uses the structure defined in [fsm.h](https://github.com/RZHuangJeff/Toc-Project/blob/main/fsm/fsm.h). This system has 19 states, and for different uesrs, the system maintains different finite state machine, which is able to support multiple users access.

For the first use, the system askes for the user to set a username which will be used while sharing.
After setting a valid username, there are three commands avaliable.

For the Join command, the system will ask user to enter a valid family-id that he wants to join in.

For the Unlock command, the system will fetch the doors that are accessable from the user and list them to the user. If there is no doors avaliable, the user will be noticed that he has no permission to do that.

For the Share command, the system will first check if the user is in a family, then ask for which door to share. after the user pick a valid door, the user to share to, how long to share, the proper permission will be set to that user.

## Http Server
This part is the main part that handles for the http requests and responses. After setting the routes and starting it up, the handler function that was binded with the specific route will be called to handle the request while a connection is recieved.

For every request, the server will parsing it into http_request_t type that defined in [http.h](https://github.com/RZHuangJeff/Toc-Project/blob/main/http/http.h), then a request will be passed to the handler function. And for the response, the handler function may set the response that passed to it directly, the server will send the response automatically.

## Json Parser
This is the part that handles for json parsing, including parsing string into json_ele_t type defined in [json.h](https://github.com/RZHuangJeff/Toc-Project/blob/main/json/json.h), and parsing given json_ele_t into a json formated string.