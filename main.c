#if 0 //To compile, first compile game.c to generate the OBJ files, then type: "nmake main.c"
main.exe : simple.obj voxlap5.obj v5.obj kplib.obj winmain.obj
link main voxlap5 v5 kplib winmain ddraw.lib dinput.lib ole32.lib dxguid.lib user32.lib gdi32.lib /opt:nowin98
main.obj : main.c voxlap5.h sysmain.h
cl /c /J /TP main.c /Ox /Ob2 /G6Fy /Gs /ML /QIfist
!if 0
#endif

#include "sysmain.h"
#include "voxlap5.h"
#include <math.h>
#include <stdlib.h>
#include "myvars.h"

#define MAXSPRITES 1024


#ifdef __cplusplus
extern "C" {
#endif
	extern long mousx,mousy;
#ifdef __cplusplus
}
#endif

typedef struct {
	float x;
	float y;
	float z;
	long buttons;
} mouse;

typedef struct {
	vx5sprite spr;
	long tag;
	long tim;
	dpoint3d v;
	dpoint3d r;
} mysprite;



dpoint3d ipos, istr, ihei, ifor;
dpoint3d fallspd = {0.0,0.0,0.0};

mysprite spr[MAXSPRITES];
long numsprites = 0;

double odtotclk,dtotclk;
long totclk;
float zoom = 0.55;
mybool click_trex = false;
mybool key_trex[3];

mybool zooming = false;
mybool f3info = false;

//hex colors
//www.rgbtohex.net thanks to search hex code
myhex build_colors[9];
int colorid = 0;

//cube png
mypng cubepng;
//target cursor
mypng targetpng;

void camera_fix(){
	double return_lerp = 0.06;
	double str = istr.z;
	if (str != 0.0){

		double return_force = str * +0.05;
		dorthorotate(return_force,0.0,0.0,&istr,&ihei,&ifor);
	}

	double dist = sqrt(istr.x*istr.x + istr.y*istr.y + istr.z*istr.z);
	if (dist > 0.0){
		istr.x /= dist;
		istr.y /= dist;
	}


	double dist_h = sqrt(ihei.x*ihei.x + ihei.y*ihei.y + ihei.z*ihei.z);
	if (dist_h > 0.0){
		ihei.x /= dist_h;
		ihei.y /= dist_h;
		ihei.z /= dist_h;
	}

	ifor.x = istr.y*ihei.z - istr.z*ihei.y;
	ifor.y = istr.z*ihei.x - istr.x*ihei.z;
	ifor.z = istr.x*ihei.y - istr.y*ihei.x;

}

void construct(mouse *m){

	build_colors[0].hex = 0x003CDC;//blue
	build_colors[1].hex = 0xAA3CDC;//purple
	build_colors[2].hex = 0xFFFF00;//yellow
	build_colors[3].hex = 0xFF8200;//orange
	build_colors[4].hex = 0x008200;//green
	build_colors[5].hex = 0xD20000;//red
	build_colors[6].hex = 0x000000;//black
	build_colors[7].hex = 0xFFFFFF;//white
	build_colors[8].hex = 0x9B9B9B;//gray



	lpoint3d hitpos;
	long *voxel_ptr;
	long hitface;


	hitscan(&ipos,&ifor,&hitpos,&voxel_ptr,&hitface);

	if ((m->buttons & 3) == 0) {
		click_trex = false;

	}

	if (voxel_ptr != 0){
		
		double dist_sq = pow(hitpos.x - ipos.x,2) + pow(hitpos.y - ipos.y,2) + pow(hitpos.z - ipos.z,2);

		if (dist_sq < 30) {	
			if (m->buttons & 1 && click_trex == false){
				setcube(hitpos.x,hitpos.y,hitpos.z,-1);
				updatevxl();
				click_trex = true;

			}

			if (m->buttons & 2 && click_trex == false){
				long bx = hitpos.x;
				long by = hitpos.y;
				long bz = hitpos.z;

				switch (hitface){
					case 0:{ bx--;} break;
					case 1:{ bx++;} break;
					case 2:{ by--;} break;
					case 3:{ by++;} break;
					case 4:{ bz--;} break;
					case 5:{ bz++;} break;
				}


				setcube(bx,by,bz,build_colors[colorid].hex);
				updatevxl();
				click_trex = true;

			}
		}
		

	}

	if (keystatus[0x3f]){
		savevxl("voxdata/vxl/untitled.vxl",&ipos,&istr,&ihei,&ifor);
		keystatus[0x3f] = 0;
	}
}


long initapp (long argc, char **argv)
{
	prognam = " The Ninos";
	xres = 320; yres = 240; colbits = 32; fullscreen = 0;
	initvoxlap();
	loadvxl("voxdata/vxl/untitled.vxl",&ipos,&istr,&ihei,&ifor);
	ipos.x = 756 + 4;
	ipos.z = 126 + 4;
	ipos.y = 598 + 4;
	loadsky ("voxdata/png/TOONSKY.JPG");

	vx5.lightmode = 1;
	vx5.fallcheck = 1;
	vx5.maxscandist = 356;
	vx5.fogcol =  0x55CDFF;

	readklock(&dtotclk);
	totclk = (long)(dtotclk*1000.0);

	setnormflash(0.0f,0.0f,-1.0f,64,128);
	updatelighting(0,0,0,1024,1024,256);

	if (myloadpng("voxdata/png/cube.png",&cubepng) > 0){
	cubepng.sx = 2;
	cubepng.sy = 2;
	}

	if (myloadpng("voxdata/png/point.png",&targetpng) > 0){
		targetpng.sx = xres / 2;
		targetpng.sy = yres / 2;
	}

	for (int i = 0;i < 3;i++){
		key_trex[i] = false;
	}

	updatevxl();

	return(0);
}

const double TIME_STEP = 1.0 / 60.0;
double accumulator = 0.0;

void doframe ()
{

	odtotclk = dtotclk;
	readklock(&dtotclk);
	double frame_time = dtotclk - odtotclk;

	if (frame_time > 0.25) {
		frame_time = 0.25;
	}

	accumulator += frame_time;
	mouse m = {0.0f,0.0f,0.0f,0};	
	readmouse(&m.x,&m.y,&m.z,&m.buttons);

	dorthorotate(0.0,m.y * 0.002,m.x * 0.002,&istr,&ihei,&ifor);
	camera_fix();
	construct(&m);

	
	while (accumulator >= TIME_STEP) {
	double speed = (keystatus[0x2a]) ? 0.05 : 0.02;
	
	startfalls();
	for (int i = vx5.flstnum-1;i >= 0;i--){

		if (vx5.flstcnt[i].userval == -1) {
			vx5.flstcnt[i].userval2 = totclk + 100;
			vx5.flstcnt[i].userval = 1;

			if (numsprites < MAXSPRITES) {

				if (meltfall(&spr[numsprites].spr,i,1)){
					long k = numsprites++;

					spr[k].tag = -17;
					spr[k].tim = totclk;

					spr[k].spr.s.x = 1.0f; spr[k].spr.s.y = 0.0f; spr[k].spr.s.z = 0.0f;
					spr[k].spr.h.x = 0.0f; spr[k].spr.h.y = 1.0f; spr[k].spr.h.z = 0.0f;
					spr[k].spr.f.x = 0.0f; spr[k].spr.f.y = 0.0f; spr[k].spr.f.z = 1.0f;

					spr[k].v.x = 0.0f;
					spr[k].v.y = 0.0;
					spr[k].v.z = 0.1f;

					spr[k].r.x = ((float)rand()/32767.0f) * 0.1f;
					spr[k].r.y = ((float)rand()/32767.0f) * 0.1f;
					spr[k].r.z = ((float)rand()/32767.0f) * 0.1f;
				}
			}
		}
	}
	finishfalls();

	for (int i = 0;i < numsprites;i++){
		if (spr[i].tag == -17){

			spr[i].v.z += 0.05f;

			spr[i].spr.p.x += spr[i].v.x;
			spr[i].spr.p.y += spr[i].v.y;
			spr[i].spr.p.z += spr[i].v.z;

			dpoint3d ds = { spr[i].spr.s.x, spr[i].spr.s.y, spr[i].spr.s.z };
			dpoint3d dh = { spr[i].spr.h.x, spr[i].spr.h.y, spr[i].spr.h.z };
			dpoint3d df = { spr[i].spr.f.x, spr[i].spr.f.y, spr[i].spr.f.z };

			dorthorotate(spr[i].r.x,spr[i].r.y,spr[i].r.z,&ds,&dh,&df);
			if (spr[i].spr.p.z >= 250.0f){
				spr[i].tag = 0;
			}
		}
	}


	fallspd.x = 0.0;
	fallspd.y = 0.0;
	if (fallspd.z < 4.0){
		fallspd.z += 0.003;
	}

	readkeyboard(); 

	//W
	if (keystatus[0x11]){
		fallspd.x += ifor.x * speed;
		fallspd.y += ifor.y * speed;
	}
	//S
	if (keystatus[0x1f]){
		fallspd.x -= ifor.x * speed;
		fallspd.y -= ifor.y * speed;
	}
	//A
	if (keystatus[0x1e]){
		fallspd.x -= istr.x * speed;
		fallspd.y -= istr.y * speed;
	}
	//D
	if (keystatus[0x20]){
		fallspd.x += istr.x * speed;
		fallspd.y += istr.y * speed;
	}

	//C
	if (keystatus[0x2e]){
		if (zoom < 6.3) zoom += 0.01;
	} else {
		if (zoom > 0.55){
		zooming = true;
		zoom -= 0.02;
		} else {zooming = false;}
	} 

	if (zooming == false && zoom > 0.55 || zoom < 0.55) zoom = 0.55;

	//f2
	if (keystatus[0x3c]){
		if (key_trex[0] == false){
			key_trex[0] = true;
			f3info = !f3info;
		}

	} else {key_trex[0] = false;}

	

	//up
	if (keystatus[0xc8]){
		if (key_trex[1] == false){
			key_trex[1] = true;
			colorid++;
			if (colorid >= 9){
				colorid = 0;
			}
		}
	} else {key_trex[1] = false;}
	//down
	if (keystatus[0xd0]){
		if (key_trex[2] == false){
			key_trex[2] = true;
			colorid--;
			if (colorid < 0){
				colorid = 8;
			}
		}
	} else {key_trex[2] = false;}


	double posz = ipos.z;
	clipmove(&ipos,&fallspd,0.25);
	//

	if (fallspd.z > 0.0 && ipos.z <= posz){
		fallspd.z = 0.0;
	} else if (fallspd.z < 0.0 && ipos.z >= posz){
		fallspd.z = 0.0;
		if (keystatus[0x39]){
			fallspd.z = -0.10;
			keystatus[0x39] = 0;
		}
	}
accumulator -= TIME_STEP;	

	}

	long frameptr, pitch, xdim, ydim;
	startdirectdraw(&frameptr,&pitch,&xdim,&ydim);
	voxsetframebuffer(frameptr,pitch,xdim,ydim);
	setcamera(&ipos,&istr,&ihei,&ifor,xdim*.5,ydim*.5,xdim*zoom);
	opticast();

		if (f3info == true){
		print6x8(10,10,0xffffff,-1,"X:%.2f Z:%.2f Y:%.2f",ipos.x,ipos.z,ipos.y);
		print6x8(10,18,0xffffff,-1,"Zoom:%f",zoom);
		for (int i = 0;i < 255;i++){
			if (keystatus[i]) {
				print6x8(10,27,0xffffff,-1,"Current Key Code: 0x%02x",i);
			}
		}
	}
	if (f3info == false){
		//print6x8(10,10,build_colors[colorid].hex,-1,"Block Color");
		if (cubepng.ptr){
		drawtile(cubepng.ptr,cubepng.x * 4,(long)cubepng.x,(long)cubepng.y,0,0,cubepng.sx * 65536,cubepng.sy * 65536,65536,65536,build_colors[colorid].hex,-1);
		}
	}
	drawtile(targetpng.ptr,targetpng.x * 4,(long)targetpng.x,(long)targetpng.y,0,0,targetpng.sx * 65536,targetpng.sy * 65536,65536,65536,0,-1);		

	for (int i = 0;i < numsprites;i++) {
		if (spr[i].tag == -17) {
			drawsprite(&spr[i].spr);
		}
	}

	//f3 screenshot
	if (keystatus[0x3d]){
		screencapture32bit("screenshot.png");
	}

	stopdirectdraw();
	nextpage();

	Sleep(1);
	if (keystatus[1]) quitloop();
}

void uninitapp () {  /*uninitvoxlap();*/ }



