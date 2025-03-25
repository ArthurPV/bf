#include <stdio.h>
#include <string.h>

#include "interpreter.h"

/// @param file const char* (&)
static inline int run_interpreter__Bf(const char *file);

/// @param FILE* (&)
/// @param cmd const char*? (&)
static void print_help__Bf(FILE *stream, const char *cmd);

int run_interpreter__Bf(const char *file)
{
	return run__BfInterpreter(file);
}

void print_help__Bf(FILE *stream, const char *cmd)
{
	fprintf(stream, "Usage: %s <file>\n", cmd ? cmd : "(undefined)");
	fprintf(stream, "\n");
	fprintf(stream, "  -h, --help		Print help\n");
}

int main(int argc, char **argv) {
	const char *cmd = argc > 0 ? argv[0] : NULL; // const char*? (&)

	if (argc == 2) {
		const char *op = argv[1]; // const char* (&)

		if (!strcmp(op, "-h") || !strcmp(op, "--help")) {
			print_help__Bf(stdout, cmd);

			return 0;
		}

		return run_interpreter__Bf(op);
	}

	print_help__Bf(stderr, cmd);

	return 1;
}
