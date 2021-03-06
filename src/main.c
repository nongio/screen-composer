#define _POSIX_C_SOURCE 200809L
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEBUG 1
#include "log.h"
#include "sc_compositor.h"
#include "sc_config.h"

extern struct sc_configuration configuration;

int
main(int argc, char **argv, char **environ)
{
	//	wlr_log_init(WLR_DEBUG, NULL);

	char *startup_cmd = NULL;
	char *config_file = "./config.ini";

	int c;
	while ((c = getopt(argc, argv, ":s:c:h")) != -1) {
		switch (c) {
		case 's':
			startup_cmd = optarg;
			break;
		case 'c':
			config_file = optarg;
			break;
		default:
			break;
		}
	}
	if (optind < argc) {
		LOG("Usage: %s [-s startup command -c config file path]\n", argv[0]);
		return 0;
	}

    if (sc_load_config(config_file)) {
        ELOG("can't load %s\n", config_file);
    }
	LOG("config loaded from '%s'\n", config_file);
	LOG("display:%dx%d:%d\n", configuration.display_width,
		configuration.display_height, configuration.display_refresh);
	LOG("shaders:%s\n", configuration.shaders_path);

	sc_compositor_create();
	sc_compositor_start_server();

	setenv("WAYLAND_DISPLAY", sc_compositor_get_socket(), true);

	if (startup_cmd) {
		if (fork() == 0) {
			execl("/bin/sh", "/bin/sh", "-c", startup_cmd, (void *) NULL);
		}
	}
	LOG("socket: %s\n", sc_compositor_get_socket());

	sc_compositor_start_eventloop();

	return 0;
}
