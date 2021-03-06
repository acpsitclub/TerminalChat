/**
 * File: lib-chatnet-client.h
 * Contains client-side code to communicate with server.
 *
 * Copyright (C) 2021 Muhammad Bin Zafar <midnightquantumprogrammer@gmail.com>
 * Licensed under the GPLv2.
 **/

#pragma once
#include "curl/curl.h"
#include "lib-cpython-builtins.h"

//const char* err = "\033[0;31m[Error]\033[0m";
//const char* info = "\033[0;32m[Info]\033[0m";
#define err "\033[0;31m[Error]\033[0m"
#define info "\033[0;32m[Info]\033[0m"
#define init "\033[0;32m[Init]\033[0m"
#define log  _GRY "[Log]" _R0
#define warn _YLW "[Warning]" _R0

//const char* cdir = "./chatnet.cache";
#define cdir "./chatnet.cache"
#define uSendDir cdir "/__uSend__"
#define shkeyDir cdir ""
#define uRecvAllFn cdir "/__uRecvAll__"
#define readingAlreadyFn cdir "/read_AllMsg"
#define activeFn cdir "/__active__"
//#define uSendDir str_addva(cdir, "/_uSend_")
//#define shkeyDir str_addva(cdir, "")
//#define uRecvAllFn str_addva(cdir, "/_uRecvAll_")  //TODO Store chatlogs in cdir/uRecvFn
//#define readingAlreadyFn str_addva(cdir, "/read_AllMsg")
//#define activeFn str_addva(cdir, "/_active_")

const char* NETADDR = "https://midnqp.000webhostapp.com/chatnet/server.php";
// NETADDR could be any address where `net.php` is stored.
//const char* NETADDR = "http://localhost/network.php";

#define PERFORMCURL_FAILED "--SYMBOL_PERFORMCURL_FAILED--"
#define WRITEMSG_FAILED "--SYMBOL_WRITEMSG_FAILED--"


char* serverComm(const char* postData);


char* read_uSend() {
	char* uSend = file_read(uSendDir);
	//uSend[strlen(uSend) - 1] = '\0';
	return uSend;
}


char* read_shkey(const char* uRecv) {
	char* add = str_addva(shkeyDir, "/", uRecv);
	char* tmpRead =  file_read(add);
	free(add);
	return tmpRead;
}


size_t curl_writefunc_callback(void* p, size_t size, size_t count, struct string* cResp) {
	size_t newLen = cResp->len + size * count;
	cResp->str = (char*)realloc(cResp->str, newLen + 1);

	if (cResp->str == NULL) { 
		printf("curl_writefunc() failed\n"); 
		exit(1); 
	}
	
	memcpy(cResp->str + cResp->len, p, size * count);
	cResp->str[newLen] = '\0';
	cResp->len = newLen;

	//210720: you just need the size*count, nothing else matters right?
	return size * count;
}


char* __performCurl__(const char* PostData) {
	curl_global_init(CURL_GLOBAL_DEFAULT);
	CURL* curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, NETADDR);
		//struct curl_slist* headers = NULL;
		//headers = curl_slist_append(headers, "Content-Type: application/json");
		//curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, PostData);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		

		/* 
		 * CRITICAL :: EXPLANATION :: curl_writefunc_callback()
		 *
		 *
		 * A function (with signatures according to curl's needs) 
		 * is provided to the curl option, for being used for 
		 * writing responses.
		 *
		 * And the `struct string* cResp` (the last argument 
		 * of the callback function) - a variable is provided
		 * the arg `cResp` using `CURLOPT_WRITEDATA`!
		 *
		 * Moreover do notice, the variable is provided as an address.
		 * Meaning, a new var won't be instatiated every time 
		 * data comes in like Nodejs. Data stream will be concat'ed
		 * to `cwfResp`!
		 *
		 * cwfResp = Curl Write Function's Response 
		 * (provided through CURL_WRITEDATA as address).
		 * cResp = Curl's Response 
		 * (this name is used at function declaration signature).
		 *
		 */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_writefunc_callback);
		struct string cwfResp; //curl write func : response
		str_init(&cwfResp);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &cwfResp); //Address of cwfResp (&cwfResp), not just (cwfResp). Else: Errors!!!!



		CURLcode rescode = curl_easy_perform(curl);
		if (rescode != CURLE_OK) {
			// unexpected failure
			return (char*)PERFORMCURL_FAILED;
		}
		curl_easy_cleanup(curl);
		curl_global_cleanup();
	
		char* rawResp = new(char*, strlen(cwfResp.str));
		strcpy(rawResp, cwfResp.str);
		free(cwfResp.str);
		return rawResp;
	}
	else {
		printf("%s Couldn't init performCurl.\n", err);
		exit(1);
	}
}


char* serverComm(const char* postData) {
	//printf("%s %s\n", log, postData);

	for (int i = 0; i < 10; i++) {
		char* serverResponse = __performCurl__(postData);
		if (!str_eq(serverResponse, PERFORMCURL_FAILED)) {
			return serverResponse; //success. break.
		}
		else  {
			printf("%s Connection to server dropped. Retrying...\n", warn);
			//free(serverResponse); //segv error write access
#ifdef _WIN32
			Sleep(1);
#else
			sleep(1);
#endif
		}
	}
	printf("%s Connection dropped permanently. Check your internet connection.\n", err);
	exit(-1);
	//Lets free memory
	//return (char*)PERFORMCURL_FAILED;
}


char* read_AllMsg() {
	char* uSend = read_uSend();
	char* post = str_addva("--read_AllMsg ", uSend);
	char* respAfterPost = serverComm(post);
	free(uSend);
	free(post);
	return respAfterPost;
}


char* read_uRecvFromMsg(const char* msgText) {
	int firstSpace = str_index(msgText, " ", 0, strlen(msgText));
	return str_slice(msgText, 0, 1, firstSpace);
}


void read_uRecvAll() {
	char* uSend = read_uSend();
	char* add = str_addva("--read_uRecvAll ", uSend);
	char* uRecvAll = serverComm(add);
	file_write(uRecvAllFn, uRecvAll);
	free(uSend);
	free(add);
	free(uRecvAll);
}


char* read_active() {
	char* uSend = read_uSend();
	char* add = str_addva("--read_active ", uSend);
	char* activeAll = serverComm(add);
	file_write(activeFn, activeAll);
	free(uSend);
	free(add);
	return activeAll;
}


void write_chatroom(const char* uRecv) {
	//printf("----libchatnet.h: Writing chatroom----\n");
	char* uSend = read_uSend();
	char *_msg = str_addva("--write_chatroom ", uSend, " ", uRecv);
	char* uRecvFn = serverComm(_msg);
	
	char* uRecvKey = str_addva(shkeyDir, "/", uRecv);
	file_write(uRecvKey, uRecvFn);
	
	free(uSend);
	free(_msg);
	free(uRecvFn);
	free(uRecvKey);
}


char* write_ThisMsg(const char* msgText) {
	char* uRecv = read_uRecvFromMsg(msgText);

	char* chatroomFn = read_shkey(uRecv);
	//char* chatroomFn = str_addva(uRecv, "-", shkey);

	char* _msgText = str_replace(msgText, uRecv, "", 0, strlen(uRecv));

	char* uSend =read_uSend(); 
	char* post = str_addva("--write_ThisMsg ", uSend, " ", chatroomFn, _msgText, "\n");
	char* respAfterPost = serverComm(post);
	
	free(uRecv);
	free(chatroomFn);
	free(_msgText);
	free(uSend);
	free(post);

	return respAfterPost;
}


void write_exit() {
	file_remove(readingAlreadyFn);

	char* uSend = read_uSend();
	char* add = str_addva("--write_exit ", uSend);
	char* com = serverComm(add);
	printf("%s Exited chatnet network.\n", info);
	free(uSend);
	free(add);
	free(com);
}
