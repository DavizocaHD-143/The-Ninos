#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <enet/enet.h>
#include "myvars.h"
#include "vxl.h"

#define PORT 2019
#define MAX_PLAYERS 32
#define CHANNELS 2

#pragma pack(push, 1)
typedef struct {
	int type;
} GenericPacket;

typedef struct {
	int type;
	int id;
	mybool connected;
} packet_id;

typedef struct {
	int type;
	int action;
	int x;
	int y;
	int z;
	int color;
} packet_construct;

typedef struct {
	int type;
	int id;
	float x,y,z;
	float dirx,diry,dirz;
} packet_pos;
#pragma pack(pop)

typedef struct {
	char nick[32];
	int id;
	float x,y,z;
	mybool connected;
} internet_c;

void log_network(const char *text) {
	FILE *file = fopen("server_log.txt", "a");
	if (file) {
		fprintf(file, "%s\n", text);
		fclose(file);
	}
	printf(text);

}


int main(int argc, char** argv) {
	if (enet_initialize() != 0) {
		log_network("Error initializing ENet!\n");
		return 1;
	}
	atexit(enet_deinitialize);

	ENetAddress address;
	ENetHost* server;

	address.host = ENET_HOST_ANY;
	address.port = PORT;

	server = enet_host_create(&address, MAX_PLAYERS, CHANNELS, 0, 0);
	if (server == NULL) {
		log_network("Error creating ENet server!\n");
		return 1;
	}

	printf("The Ninos server started on port %d...\n", PORT);
	printf("Loading Map...");
	int res = vxl_load("vxl/map.vxl");
	if (res == 0) {
		printf("Loaded Sucessfull");
	} else {
		printf("Error Loading");
	}


	internet_c p[MAX_PLAYERS];
	memset(p, 0, sizeof(p)); //clear memory
	for (int i = 0;i < MAX_PLAYERS;i++){
		p[i].connected = false;
	}

	ENetEvent event;
	bool running = true;

	while (running) {
		while (enet_host_service(server, &event, 10) > 0) {
			switch (event.type) {
				case ENET_EVENT_TYPE_CONNECT:
						{

					int open_id = -1;

					for (int i = 0;i < MAX_PLAYERS;i++){
						if (p[i].connected == false){
							open_id = i;
							break;
						}
					}

					if (open_id != -1) {

						packet_id p_id;
						p_id.type = 1;
						p_id.id = open_id;
						p_id.connected = true;
						ENetPacket* send_id = enet_packet_create(&p_id, sizeof(packet_id), ENET_PACKET_FLAG_RELIABLE);
						enet_host_broadcast(server, 0, send_id);

						p[open_id].connected = true;
						event.peer->data = (void*)(uintptr_t)open_id;
						printf("New client connected from\nID:%i HOST:%u PORT:%u \n",
								open_id,
								event.peer->address.host,
								event.peer->address.port);
					}
					}
					break;

				case ENET_EVENT_TYPE_RECEIVE:
					// READ
					if (event.packet->dataLength >= sizeof(int)) {
						GenericPacket* msg = (GenericPacket*)event.packet->data;

						switch (msg->type) {
							case 2:
								{
									packet_construct receive;
									memcpy(&receive, event.packet->data, sizeof(packet_construct));
									if (receive.action == 1) {
										vxl_set_geom(receive.x,receive.y,receive.z,1);
										vxl_set_color(receive.x,receive.y,receive.z,receive.color);
										else {
											vxl_set_geom(receive.x,receive.y,receive.z,0);
										}

										ENetPacket* send = enet_packet_create(&receive, sizeof(packet_construct), ENET_PACKET_FLAG_RELIABLE);
										enet_host_broadcast(server,0,send);

								}
							break;

							case 3:
							{
								packet_pos receive;
									memcpy(&receive, event.packet->data, sizeof(packet_pos));
									int id = receive.id;
									p[id].x = receive.x;
									p[id].y = receive.y;
									p[id].z = receive.z;

									ENetPacket* send = enet_packet_create(&receive, sizeof(packet_pos), ENET_PACKET_FLAG_RELIABLE);
										enet_host_broadcast(server,0,send);
							}
							break;
							default:
								break;
						}
					}

					enet_packet_destroy(event.packet);
					break;

				case ENET_EVENT_TYPE_DISCONNECT:
					disconnect_id = event.peer->data;
					p[disconnect_id].connected = false;
					packet_id p;
					p.type = 1;
					p.id = disconnect_id;
					p.connected = false;
					ENetPacket* send_id = enet_packet_create(&p_id, sizeof(packet_id), ENET_PACKET_FLAG_RELIABLE);
						enet_host_broadcast(server, 0, send_id);
					printf("Player disconnected.\n");
					event.peer->data = NULL;
					break;

				default:
					break;
			}
		}
	}

	printf("Shutting down server...\n");
	enet_host_destroy(server);

	return 0;
}

