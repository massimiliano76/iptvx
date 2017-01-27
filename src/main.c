/*

   Copyright 2017   Jan Kammerath

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <libconfig.h>
#include <glib.h>
#include "args.h"
#include "config.h"
#include "window.h"
#include "video.h"
#include "webkit.h"
#include "js.h"
#include "epg.h"

/* context of the window */
void* main_window_context;

/* window dimensions */
int main_window_width;
int main_window_height;

/* handles any key down event */
void keydown(int keyCode){
	if(keyCode == 27){
		// termination of application
		iptvx_video_free();
	}else{
		// forward key input to webkit
		iptvx_js_sendkey(keyCode);		
	}
}

/*
	Handles retrieval of API control messages
	@param		message 	the control message as string
*/
void control_message_received(void* message){
	printf("CONTROL MESSAGE RECEIVED: %s\n",message);
}

/* handles browser load finish */
void load_finished(void* webview){
	/* initialise JS API with webview and callback func */
	iptvx_js_init(webview,control_message_received);
}

/* plays a channel's url with the video player */
void channel_video_play(char* url){
	/* initialise the video playback */
	iptvx_video_init(url,main_window_width,main_window_height);

	/* start the playback on screen */
	iptvx_video_play(iptvx_window_lock,iptvx_window_unlock,
					iptvx_window_display,main_window_context);	
}

/* starts playback when SDL window is ready */
void window_ready(void* context){
	/* keep window context as it might be required later on */
	main_window_context = context;

	/* get the default channel and play it */
	channel* defaultChannel = iptvx_epg_get_default_channel();
	channel_video_play((char*)defaultChannel->url);
}

/* main application code */
int main (int argc, char *argv[]){
	/* parse input arguments first */
	struct arguments arguments = iptvx_parse_args(argc,argv);

	/* ensure that there is a config file */
	if(iptvx_config_init() == true){
		main_window_width = iptvx_config_get_setting_int("width",1280);
		main_window_height = iptvx_config_get_setting_int("height",720);

		/* initialise the epg */
		config_t* cfg = iptvx_get_config();
		iptvx_epg_init(cfg);

		/* get the pointers to the webkit png data and status */
		void* overlay_data = iptvx_get_overlay_ptr();
		void* overlay_ready = iptvx_get_overlay_ready_ptr();
		iptvx_window_set_overlay(overlay_data,overlay_ready);

		/* start the webkit thread */
		char* overlayApp = iptvx_config_get_overlay_app();
		iptvx_webkit_start_thread(overlayApp,load_finished);

		/* create the thread for the main window */
		iptvx_create_window(main_window_width,
							main_window_height,
							keydown,window_ready);
	}


	return 0;
}