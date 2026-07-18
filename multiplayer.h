#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <io.h>
#include <stdio.h>
#include <windows.h>
#include <fcntl.h>
#include <iostream.h>
#include <enet/enet.h>
#include <string.h>
#include "myvars.h"

#ifdef __cplusplus
extern "C" {
#endif
	HWND WINAPI GetConsoleWindow(void);
#ifdef __cplusplus
}
#endif

#ifndef ENABLE_QUICK_EDIT_MODE
#define ENABLE_QUICK_EDIT_MODE 0x0040
#endif

#ifndef _INTPTR_T_DEFINED
#ifdef _WIN64
typedef __int64 intptr_t;
#else
typedef long intptr_t;
#endif
#define _INTPTR_T_DEFINED
#endif

#define MAX_PLAYERS 32
extern ENetHost *client;
extern ENetPeer *peer;
extern mybool enet_configured;
extern mybool cmd_show;
extern mybool nomousefocus;

static char chat_input_buffer[128] = "";
static int chat_input_pos = 0;
typedef struct {
	char nick[32];
	int id;
	char ip[64];
	int port;
} client_c;//client config
typedef struct {
	char nick[32];
	int id;
	float x,y,z;
	float dirx,diry,dirz;
	mybool connected;
} internet_c;
//packets
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

inline void multiplayer_run(client_c *c,internet_c *s){
	if (!client) return;

	HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
	DWORD numEvents = 0;

	if (GetNumberOfConsoleInputEvents(hInput, &numEvents) && numEvents > 0) {
		INPUT_RECORD eventBuffer[32];
		DWORD eventsRead = 0;

		if (ReadConsoleInput(hInput, eventBuffer,32,&eventsRead)) {
			for (DWORD i = 0; i < eventsRead; i++) {

				if (eventBuffer[i].EventType == KEY_EVENT && eventBuffer[i].Event.KeyEvent.bKeyDown) {
					char letter = eventBuffer[i].Event.KeyEvent.uChar.AsciiChar;
					WORD keyCode = eventBuffer[i].Event.KeyEvent.wVirtualKeyCode;

					if (keyCode == VK_RETURN) {
						if (chat_input_pos > 0) {
							chat_input_buffer[chat_input_pos] = '\0';
							printf("\n%s: %s\n",c->nick,chat_input_buffer);

							//send enet chat (add later)
							
							chat_input_pos = 0;
							chat_input_buffer[0] = '\0';
						}
					}
					else if (keyCode == VK_BACK) {
						if (chat_input_pos > 0) {
							chat_input_pos--;
							printf("\b \b");
						}
					}
					else if (letter >= 32 && letter <= 126) {
						if (chat_input_pos < 127) {
							chat_input_buffer[chat_input_pos++] = letter;
							putchar(letter);
						}
					}
				}
			}
		}
	}

	ENetEvent event;

        while (enet_host_service(client, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("\nConnected Sucessfull ! on IP:%s Port:%d\n",c->ip,c->port);
                    break;

                case ENET_EVENT_TYPE_RECEIVE:

                    // data on: event.packet->data
                    // size of packages: event.packet->dataLength

                    // --- process datas here ---
                    if (event.packet->dataLength >= sizeof(int)) {
                        GenericPacket* msg = (GenericPacket*)event.packet->data;

                        switch (msg->type) {
				case 1:
					{
						packet_id receive;
						memcpy(&receive, event.packet->data, sizeof(packet_id));

						if (c->id == -1){
						
							c->id = receive.id; 
							printf("Your ID: %i",c->id);
						} else {
							s->id = receive.id;
							s->connected = receive.connected;
							
						}
					}
				break;
				
				case 2:
				{
					packet_construct receive;
									memcpy(&receive, event.packet->data, sizeof(packet_construct));
									if (receive->action == 1) {
										setcube(receive->x,receive->y,receive->z,receive->color);
									} else {
										setcube(receive->x,receive->y,receive->z,-1);
									}
				}
				break;

				case 3:
				{
					packet_construct receive;
									memcpy(&receive, event.packet->data, sizeof(packet_pos));
									int id = receive.id;

									if (id != c->){
										s[id]->x = receive.x;
										s[id]->y = receive.y;
										s[id]->z = receive.z;

										s[id]->dirx = receive.dirx;
										s[id]->diry = receive.diry;
										s[id]->dirz = receive.dirz;
									}
										



				}
				break;

                            default:
                                break;
                        }
                    }

                    enet_packet_destroy(event.packet);
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
		    printf("Disconnected Server");
                    break;

                default:
                    break;
            }
        }
}

inline void multiplayer_deinit(){
	if (peer) { enet_peer_disconnect_now(peer,0) ; peer = NULL;}
	if (client) {enet_host_destroy(client); client = NULL;}
	if (enet_configured) { enet_deinitialize(); enet_configured = false; }
	nomousefocus = false;

	if (cmd_show) {
		fclose(stdout);
		fclose(stdin);
		FreeConsole();
		cmd_show = false;
	}

}

void multiplayer_cmd_init(client_c *c) {

	AllocConsole();
	cmd_show = true;
	
	//stdout
	HANDLE stdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	int conDescriptor = _open_osfhandle((intptr_t)stdHandle,_O_TEXT);
	FILE* fp = _fdopen(conDescriptor, "w");
	*stdout = *fp;
	setvbuf(stdout,NULL,_IONBF,0);
	
	//stdin
	HANDLE stdInputHandle = GetStdHandle(STD_INPUT_HANDLE);
	int conInputDescriptor = _open_osfhandle((intptr_t)stdInputHandle,_O_TEXT);
	FILE* fpIn = _fdopen(conInputDescriptor, "r");
	*stdin = *fpIn;
	setvbuf(stdin, NULL, _IONBF, 0);

	//quickedit set off
	DWORD prev_mode;
	GetConsoleMode(stdInputHandle, &prev_mode);
	SetConsoleMode(stdInputHandle, prev_mode & ~ENABLE_QUICK_EDIT_MODE);

	//focus
	HWND hConsole = GetConsoleWindow();
	SetForegroundWindow(hConsole);
	
	nomousefocus = true;
	
	printf("Multiplayer The Ninos\n Welcome Here Will be Chat,and Multiplayer Settings\n");

	char op_exit;
	printf("\nCancel Multiplayer ? Y/N\n");
	scanf(" %c",&op_exit);

	if (op_exit == 'N' || op_exit == 'n') {
		goto b_iniclient; //initialize client config
	}

	else {
		goto b_exit; //box exit
	}
	


b_iniclient:
	{
		printf("\nYou Nick: ");
		scanf("%32s",c->nick);
		printf("\nServer IP: ");
		scanf("%63s",c->ip);
		printf("\nPort: ");
		scanf("%d",&(c->port));
		goto b_inienet;
	}
b_inienet:
	{
		if (enet_initialize() != 0) {
			MessageBox(NULL," Enet Error: cannot initialize enet","Error !!",MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
			goto b_exit;
		} else {printf("\nENET Initialized\n");}

		atexit(enet_deinitialize);


		client = enet_host_create(NULL, 1, 2, 0, 0);
		if (client == NULL) {
			MessageBox(NULL," Enet Error: cannot create client","Error !!",MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
			goto b_exit;
		} else {printf("\nENET Host Initialized\n");}

		ENetAddress address;
		enet_address_set_host(&address, c->ip);
		address.port = c->port;

		peer = enet_host_connect(client, &address, 2, 0);
		if (peer == NULL) {
			MessageBox(NULL," Enet Error: cannot create peer","Error !!",MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);	
			goto b_exit;
		} else {printf("\nENET Peer Initialized\n");}
		return;

	}

	b_exit:
	{
		printf("Exiting");
		Sleep(1000);
		multiplayer_deinit();
	}
}





#endif
//¨¨
