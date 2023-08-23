#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cjson/cJSON.h>
#include "MQTTAsync.h"

#if !defined(_WIN32)
#include <unistd.h>
#else
#include <windows.h>
#endif

#if defined(_WRS_KERNEL)
#include <OsWrapper.h>
#endif

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "ExampleClientSub"
#define TOPIC       "data"
#define QOS         2
#define TIMEOUT     10000L
// #define SAMPLE_PERIOD   1L    // publishing frequency (in ms)
// #define CLIENTS_NUM 1
#define DATANUM     505
// #define payloadSize 80

int64_t** DATA;  // record latency

typedef struct {	// Global struct for payload
    int64_t ts;
	int cnt;
	int payloadsize;
    const char* data;
} ClientPayLoad;

typedef struct {
    MQTTAsync client;
    char clientid[24];
    char topic[100];
    long double sample_time;
    int idx;
} ClientContext;


int disc_finished = 0;
int subscribed = 0;
int finished = 0;
int cnt = 0;

void onConnect(void* context, MQTTAsync_successData* response);
void onConnectFailure(void* context, MQTTAsync_failureData* response);

void connlost(void *context, char *cause)
{
	ClientContext* clientContext = (ClientContext*)context; // Cast the context back to the struct
    MQTTAsync client = clientContext->client;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	printf("\nConnection lost\n");
	if (cause)
		printf("     cause: %s\n", cause);

	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 0;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		finished = 1;
	}
}

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
        // return ((int64_t) ts.tv_nsec / 1000);
	#endif
}

void latency(MQTTAsync_message *message, int idx, long double sample_time)
{
	int64_t t_sub = getTime();	// subscription time

	cJSON *receivedRoot = cJSON_Parse(message->payload);
	ClientPayLoad receivedPayload;

	if (receivedRoot) {
		receivedPayload.ts = (int64_t)cJSON_GetObjectItem(receivedRoot, "ts")->valuedouble;
		receivedPayload.payloadsize = (int)cJSON_GetObjectItem(receivedRoot, "payloadsize")->valueint;
		receivedPayload.cnt = (int)cJSON_GetObjectItem(receivedRoot, "id")->valueint;
		cJSON_Delete(receivedRoot);
	}
	printf("	data# %d, payloadsize: %d, sample time: %Lf, pub time: %d, sub time: %d, latency: %d us\n\n",
			receivedPayload.cnt, receivedPayload.payloadsize, sample_time, receivedPayload.ts, t_sub, t_sub-receivedPayload.ts);

    // Add latency to DATA
    DATA[receivedPayload.cnt][idx] = t_sub-receivedPayload.ts;
    cnt = cnt + 1;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    ClientContext* clientContext = (ClientContext*)context;

    printf("--> %s: Message arrived\n", clientContext->clientid);
    printf("     topic: %s\n", topicName);
    // printf("   message: %.*s\n", message->payloadlen, (char*)message->payload);
    
    // get latency
	latency(message, clientContext->idx, clientContext->sample_time);

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void onDisconnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Disconnect failed, rc %d\n", response->code);
	disc_finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful disconnection\n");
	disc_finished = 1;
}

void onSubscribe(void* context, MQTTAsync_successData* response)
{
	printf("Subscribe succeeded\n");
	subscribed = 1;
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Subscribe failed, rc %d\n", response->code);
	finished = 1;
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Connect failed, rc %d\n", response->code);
	finished = 1;
}


void onConnect(void* context, MQTTAsync_successData* response)
{
	ClientContext* clientContext = (ClientContext*)context; // Cast the context back to the struct
	MQTTAsync client = clientContext->client;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	int rc;

	printf("Successful connection\n");

	printf("Subscribing to topic %s\nfor %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", clientContext->topic, clientContext->clientid, QOS);
	opts.onSuccess = onSubscribe;
	opts.onFailure = onSubscribeFailure;
	opts.context = client;
	if ((rc = MQTTAsync_subscribe(client, clientContext->topic, QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc);
		finished = 1;
	}
}

int main(int argc, char* argv[])
{
    if (argc != 5) {
        printf("Usage: %s <sample_period> <payload_size> <sub_num> <pub_id>\n", argv[0]);
        return 1; // Exit with an error code
    }

    long double SAMPLE_PERIOD = (long double) atof(argv[1]);
    int payloadSize = atoi(argv[2]);
    int num_clients = atoi(argv[3]);

    char *pub_id = argv[4];
    char topic[1000];  // Buffer to hold the concatenated string

    // Clear the buffer to ensure it's null-terminated
    memset(topic, 0, sizeof(topic));

    // Concatenate the strings
    strcat(topic, "data");
    strcat(topic, pub_id);

    int rc = 0;
    int i;
    // define DATA[][]
    DATA = (int64_t **)malloc(DATANUM * sizeof(int64_t *));
    for (i = 0; i < DATANUM; i++) {
        DATA[i] = (int64_t *)malloc(num_clients * sizeof(int64_t));
    }

    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;

	// int num_clients = CLIENTS_NUM;  // number of clients
    ClientContext clientContext[num_clients];    // define multiple clients

    for (i=0; i<num_clients; i++)
    {
        // printf("------------ Start Client_%d -----------\n", i);
        sprintf(clientContext[i].clientid, "Sub%d_Pub%s", i, argv[4]);
        sprintf(clientContext[i].topic, topic);
        clientContext[i].idx = i;
        clientContext[i].sample_time = SAMPLE_PERIOD;

        // create client
        if ((rc = MQTTAsync_create(&(clientContext[i].client), ADDRESS, clientContext[i].clientid,
             MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
        {
            printf("Failed to create client, return code %d\n", rc);
            rc = EXIT_FAILURE;
            goto exit;
        }
        else{printf("Successfully create client object!\n\n");}

        // set callbacks
        if ((rc = MQTTAsync_setCallbacks(clientContext[i].client, &clientContext[i], connlost, msgarrvd, NULL)) != MQTTASYNC_SUCCESS)
        {
            printf("Failed to set callbacks, return code %d\n", rc);
            rc = EXIT_FAILURE;
            goto destroy_exit;
        }
        else{printf("Successfully set callback!\n\n");}

        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 0;
        conn_opts.onSuccess = onConnect;
        conn_opts.onFailure = onConnectFailure;
        conn_opts.context = &clientContext[i];
        if ((rc = MQTTAsync_connect(clientContext[i].client, &conn_opts)) != MQTTASYNC_SUCCESS)
        {
            printf("Failed to start connect, return code %d\n", rc);
            rc = EXIT_FAILURE;
            goto destroy_exit;
        }

        while (!subscribed && !finished)
            #if defined(_WIN32)
                Sleep(100);
            #else
                usleep(10000L);
            #endif

        if (finished)
            goto exit;
    }
    
    long double sleep_t = SAMPLE_PERIOD * DATANUM / 1000 + 5;
    printf("sleep time: %Lf", sleep_t);
    sleep(sleep_t);
    // int ch;
    // do
    // {
    //     ch = getchar();
    // } while (ch!='Q' && ch != 'q');
    // while(cnt!=DATANUM*num_clients){}
    // printf("cnt: %d\n", cnt);

    disc_opts.onSuccess = onDisconnect;
	disc_opts.onFailure = onDisconnectFailure;
    for (i=0; i<num_clients; ++i)
    {
        if ((rc = MQTTAsync_disconnect(clientContext[i].client, &disc_opts)) != MQTTASYNC_SUCCESS)
        {
            printf("Failed to start disconnect, return code %d\n", rc);
            rc = EXIT_FAILURE;
            goto destroy_exit;
        }
        while (!disc_finished)
        {
            #if defined(_WIN32)
                Sleep(100);
            #else
                usleep(10000L);
            #endif
        }
    }

    // Write DATA to csv file
    char filename[50];
    // sprintf(filename, "../output/client-%d_payload-%d_st-%d.csv", num_clients, payloadSize, (int)SAMPLE_PERIOD);
    sprintf(filename, "../tmp/pub_id=%s.csv", pub_id);
    printf("------ Start writing %s ------\n", filename);

    FILE *csvFile = fopen(filename, "w");
    
    if (csvFile == NULL) {
        printf("Failed to open file.\n");
        return 1; // Exit with an error code
    }

    // Add header
    for (int col = 0; col < num_clients; col++) {
        char header[50];
        sprintf(header, "Client_%d", col);
        fprintf(csvFile, "%s", header);
        
        if (col < num_clients - 1) {
            fprintf(csvFile, ",");
        }
    }
    fprintf(csvFile, "\n");

    // Write data rows
    for (int row = 5; row < DATANUM; row++) {   // remove the first 5 latency data
        for (int col = 0; col < num_clients; col++) {
            // Assuming 'data' is a matrix containing your data
            fprintf(csvFile, "%d", DATA[row][col]);
            
            // Don't add a comma at the end of the last column
            if (col < num_clients - 1) {
                fprintf(csvFile, ",");
            }
        }
        fprintf(csvFile, "\n");
    }
    
    // Close the file
    fclose(csvFile);

    for (i = 0; i < num_clients; i++) {
        free(DATA[i]);
    }
    free(DATA);

    for (i = 0; i < num_clients; ++i)
    {
        MQTTAsync_destroy(&(clientContext[i].client));
    }

    destroy_exit:
        for (i = 0; i < num_clients; ++i)
        {
            MQTTAsync_destroy(&(clientContext[i].client));
        }
    exit:
        return rc;

}