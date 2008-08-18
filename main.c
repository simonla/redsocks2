#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <event.h>
#include "log.h"
#include "main.h"
#include "utils.h"

extern app_subsys redsocks_subsys;
extern app_subsys base_subsys;
// extern app_subsys reddns_subsys;

app_subsys *subsystems[] = {
	&redsocks_subsys,
	&base_subsys,
//	&reddns_subsys,
};

static const char *confname = "redsocks.conf";

int main(int argc, char **argv)
{
	int error;
	app_subsys **ss;
	bool conftest = false;
	int opt;

	while ((opt = getopt(argc, argv, "tc:")) != -1) {
		switch (opt) {
		case 't':
			conftest = true;
			break;
		case 'c':
			confname = optarg;
			break;
		default:
			printf(
				"Usage: %s [-t] [-c config]\n"
				"  -t           test config syntax\n",
				argv[0]);
			return EXIT_FAILURE;
		}
	}


	FILE *f = fopen(confname, "r");
	if (!f) {
		perror("Unable to open config file");
		return EXIT_FAILURE;
	}

	parser_context* parser = parser_start(f, NULL);
	if (!parser) {
		perror("Not enough memory for parser");
		return EXIT_FAILURE;
	}

	FOREACH(ss, subsystems)
		if ((*ss)->conf_section)
			parser_add_section(parser, (*ss)->conf_section);
	error = parser_run(parser);
	parser_stop(parser);
	fclose(f);
	if (error)
		return EXIT_FAILURE;

	if (conftest)
		return EXIT_SUCCESS;

	event_init();

	FOREACH(ss, subsystems) {
		if ((*ss)->init) {
			error = (*ss)->init();
			if (error)
				goto shutdown;
		}
	}

	log_error(LOG_NOTICE, "redsocks started");

	event_dispatch();

shutdown:
	for (--ss; ss >= subsystems; ss--)
		if ((*ss)->fini)
			(*ss)->fini();

	event_base_free(NULL);

	return !error ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* vim:set tabstop=4 softtabstop=4 shiftwidth=4: */
/* vim:set foldmethod=marker foldlevel=32 foldmarker={,}: */
