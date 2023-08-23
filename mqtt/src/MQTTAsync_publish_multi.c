/*******************************************************************************
 * Copyright (c) 2012, 2022 IBM Corp., Ian Craggs
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *   https://www.eclipse.org/legal/epl-2.0/
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cjson/cJSON.h>
#include <stdint.h>
#include "MQTTAsync.h"

#if !defined(_WIN32)
#include <unistd.h>
#else
#include <windows.h>
#endif

#if defined(_WRS_KERNEL)
#include <OsWrapper.h>
#endif

#define ADDRESS     "mqtt://localhost:1883"
#define CLIENTID    "Clinet Pub"
#define TOPIC       "data"
#define QOS         2
#define TIMEOUT     10000L
// #define SAMPLE_PERIOD   1L    // publishing frequency (in ms)
#define DATANUM     505
// #define payloadSize 80

char* data;

typedef struct {	// Global struct to hold client and payload
    MQTTAsync client;
    char* payload;
} ClientContext;

typedef struct {	// Global struct for payload
    int64_t ts;
	int cnt;
	int payloadsize;
    char* data;
} ClientPayLoad;

int64_t getTime(void)
{
	#if defined(_WIN32)
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		return ((((int64_t) ft.dwHighDateTime) << 8) + ft.dwLowDateTime) / 10000;
	#else
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return ((int64_t) ts.tv_sec * 1000000) + ((int64_t) ts.tv_nsec / 1000);
	#endif
}

volatile int finished = 0;
volatile int connected = 0;

void connlost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	printf("\nConnection lost\n");
	printf("     cause: %s\n", cause);

	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 0;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
 		finished = 1;
	}
}

void onDisconnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Disconnect failed\n");
	finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful disconnection\n");
	finished = 1;
}

void onSendFailure(void* context, MQTTAsync_failureData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
	int rc;

	printf("Message send failed token %d error code %d\n", response->token, response->code);
	opts.onSuccess = onDisconnect;
	opts.onFailure = onDisconnectFailure;
	opts.context = client;
	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
}

void onSend(void* context, MQTTAsync_successData* response)
{
	printf("Message with token value %d delivery confirmed\n", response->token);
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Connect failed, rc %d\n", response ? response->code : 0);
	finished = 1;
}


void onConnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful connection\n");
	connected = 1;
}

int messageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* m)
{
	/* not expecting any messages */
	return 1;
}

int main(int argc, char* argv[])
{
	if (argc != 4) {
        printf("Usage: %s <sample_period> <payload_size> <pub_id>\n", argv[0]);
        return 1; // Exit with an error code
    }

    long double SAMPLE_PERIOD = (long double) atof(argv[1]);
    int payloadSize = atoi(argv[2]);

	char *pub_id = argv[3];
    char topic[1000];  // Buffer to hold the concatenated string

    // Clear the buffer to ensure it's null-terminated
    memset(topic, 0, sizeof(topic));

    // Concatenate the strings
	strcat(topic, "data");
    strcat(topic, pub_id);
    

	// ClientID
	char clientid[24];
	sprintf(clientid, "Pub_%s", argv[3]);

	printf("--------- Start %s, Topic: %s, Sample period: %Lf, payloadsize: %d ----------\n\n"
			, clientid, topic, SAMPLE_PERIOD, payloadSize);

	// Client setup
	MQTTAsync client;   // for connection to brokers
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;

    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;   // for message publishing
	MQTTAsync_responseOptions pub_opts = MQTTAsync_responseOptions_initializer;

	int rc;
	ClientContext clientContext;
	clientContext.client = client;

	// Create Client
	if ((rc = MQTTAsync_create(&clientContext.client, ADDRESS, clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to create client object, return code %d\n\n", rc);
		exit(EXIT_FAILURE);
	}
	else{printf("Successfully create client object!\n\n");}

	// Set Callback
	if ((rc = MQTTAsync_setCallbacks(clientContext.client, NULL, connlost, messageArrived, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to set callback, return code %d\n\n", rc);
		exit(EXIT_FAILURE);
	}
	else{printf("Successfully set callback!\n\n");}

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 0;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = clientContext.client;
	if ((rc = MQTTAsync_connect(clientContext.client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n\n", rc);
		exit(EXIT_FAILURE);
	}

	while (!connected) {
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(100000L);
		#endif
	}
    
	ClientPayLoad clientPayload;    // Set payload
	data = (char*)malloc(payloadSize+1);
	memset(data, 'A', payloadSize);
	data[payloadSize] = '\0';
	if (data == NULL) {
        printf("Cannot alloc byte buffer\n");
        return 1;
    }
	
	clientPayload.data = data;   // PAYLOAD
	clientPayload.payloadsize = payloadSize;

	int i;
	for (i = 0; i < DATANUM; i++) {
		clientPayload.ts = getTime();
		clientPayload.cnt = i;

        cJSON *root = cJSON_CreateObject();	// struct to json
        cJSON_AddNumberToObject(root, "ts", clientPayload.ts);
		cJSON_AddNumberToObject(root, "id", clientPayload.cnt);
		cJSON_AddNumberToObject(root, "payloadsize", clientPayload.payloadsize);
        cJSON_AddStringToObject(root, "data", clientPayload.data);
        char *jsonPayload = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
        clientContext.payload = jsonPayload;

		pub_opts.onSuccess = onSend;
		pub_opts.onFailure = onSendFailure;
		pub_opts.context = clientContext.client;

		pubmsg.payload = (char*)clientContext.payload;
		pubmsg.payloadlen = (int)strlen(clientContext.payload);
		// printf("string length: %d\n", pubmsg.payloadlen);
		pubmsg.qos = QOS;
		pubmsg.retained = 0;

		if ((rc = MQTTAsync_sendMessage(clientContext.client, topic, &pubmsg, &pub_opts)) != MQTTASYNC_SUCCESS)
		{
			printf("Failed to start sendMessage, return code %d\n", rc);
			exit(EXIT_FAILURE);
		}

		free(jsonPayload);

		#if defined(_WIN32)
			Sleep(SAMPLE_PERIOD);
		#else
			usleep(SAMPLE_PERIOD * 1000); // micro second
		#endif
	}
        
	MQTTAsync_destroy(&clientContext.client);
	free(data);
	printf("publish finished");
 	return rc;
}
  
