#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include "interpreter.h"

struct BfInterpreterContext {
	size_t content_len;
	char *content; // char* (&)
	size_t jmp_frames_len;
#define MAX_JMP_FRAMES_LEN 1024
	size_t jmp_frames[MAX_JMP_FRAMES_LEN];
	size_t stack_size;
	char *stack; // char* (&)
	char *stack_pointer; // char* (&)
};

#define EMIT_BF_ERROR(msg) fprintf(stderr, "error: "msg"\n");
#define EMIT_BF_ERROR_FMT(msg, ...) fprintf(stderr, "error: "msg"\n", __VA_ARGS__);

/// @param struct BfInterpreterContext* (&)
static int
increment_stack_pointer__BfInterpreter(struct BfInterpreterContext *ctx);

/// @param struct BfInterpreterContext* (&)
static int
decrement_stack_pointer__BfInterpreter(struct BfInterpreterContext *ctx);

/// @param struct BfInterpreterContext* (&)
/// @param current_ref char* (&)* (&)
static int
handle_start_loop__BfInterpreter(struct BfInterpreterContext *ctx, char **current_ref);

/// @param struct BfInterpreterContext* (&)
/// @param current_ref char* (&)* (&)
static int
handle_end_loop__BfInterpreter(struct BfInterpreterContext *ctx, char **current_ref);

/// @param struct BfInterpreterContext* (&)
static int
interpret_file__BfInterpreter(struct BfInterpreterContext *ctx);

/// @param file const char* (&)
/// @param stream FILE* (&)
static int
read_file__BfInterpreter(const char *file, FILE *stream);

int
increment_stack_pointer__BfInterpreter(struct BfInterpreterContext *ctx)
{
	int next_stack_pos = ctx->stack_pointer - ctx->stack + 1;

	if (next_stack_pos < ctx->stack_size) {
		++ctx->stack_pointer;

		return 0;
	}

	EMIT_BF_ERROR("stack overflow detected")

	return 1;
}

int
decrement_stack_pointer__BfInterpreter(struct BfInterpreterContext *ctx)
{
	int next_stack_pos = ctx->stack_pointer - ctx->stack - 1;

	if (next_stack_pos >= 0) {
		--ctx->stack_pointer;

		return 0;
	}

	EMIT_BF_ERROR("stack underflow detected")

	return 1;
}

int
handle_start_loop__BfInterpreter(struct BfInterpreterContext *ctx, char **current_ref)
{
	char *current = *current_ref;

	if (*ctx->stack_pointer) {
		if (ctx->jmp_frames_len + 1 < MAX_JMP_FRAMES_LEN) {
			ctx->jmp_frames[ctx->jmp_frames_len++] = current - ctx->content;
			*current_ref = ++current;

			return 0;
		}

		EMIT_BF_ERROR("jmp_frames overflow");

		return 1;
	}

	size_t count = 1;

	while (*current && count > 0) {
		switch (*current) {
			case '[':
				++count;

				break;
			case ']':
				--count;

				break;
			default:
				break;
		}

		++current;
	}

	*current_ref = current;

	return 0;
}

int
handle_end_loop__BfInterpreter(struct BfInterpreterContext *ctx, char **current_ref)
{
	char *current = *current_ref;

	if (*ctx->stack_pointer) {
		if (ctx->jmp_frames_len > 0) {
			*current_ref = (ctx->content + ctx->jmp_frames[ctx->jmp_frames_len - 1]) + 1;
		} else {
			EMIT_BF_ERROR("jmp_frames overflow");

			return 1;
		}

		return 0;
	}

	--ctx->jmp_frames_len;
	*current_ref = ++current;

	return 0;
}

int
interpret_file__BfInterpreter(struct BfInterpreterContext *ctx)
{
	int res = 0;

	for (char *current = ctx->content; *current && res == 0;) {
		switch (*current) {
			case '>':
				res = increment_stack_pointer__BfInterpreter(ctx);

				break;
			case '<':
				res = decrement_stack_pointer__BfInterpreter(ctx);

				break;
			case '+':
				++(*ctx->stack_pointer);

				break;
			case '-':
				--(*ctx->stack_pointer);

				break;
			case '.':
				putchar(*ctx->stack_pointer);

				break;
			case ',':
				(*ctx->stack_pointer) = getchar();

				break;
			case '[':
				res = handle_start_loop__BfInterpreter(ctx, &current);

				continue;
			case ']':
				res = handle_end_loop__BfInterpreter(ctx, &current);

				continue;
			default:
				break;
		}

		++current;
	}

	return res;
}

int
read_file__BfInterpreter(const char *file, FILE *stream)
{
	char *content = NULL; // char*?
	int res = 0;
	struct stat st;

	if (stat(file, &st) == -1) {
		goto failed_to_stat;
	}
	
	size_t content_len = st.st_size;
	size_t content_n = content_len + 1;

	content = malloc(content_n);

	if (!content) {
		goto failed_to_allocate_memory;
	}

	memset(content, 0, content_n);

	if (fread(content, sizeof(char), content_n, stream) != content_len) {
		goto failed_to_read_file;
	}

#define Kb 1024
	size_t stack_size = Kb * 32;
	char *stack = malloc(stack_size);
#undef Kb

	if (!stack) {
		goto failed_to_allocate_memory;
	}

	memset(stack, 0, stack_size);

	struct BfInterpreterContext ctx = {
		.content_len = content_len,
		.content = content,
		.jmp_frames_len = 0,
		.jmp_frames = {0},
		.stack_size = stack_size,
		.stack = stack,
		.stack_pointer = stack
	};

	res = interpret_file__BfInterpreter(&ctx);

	free(stack);

	goto end;

failed_to_stat:
	EMIT_BF_ERROR_FMT("failed to stat: `%s`", file);

	goto set_error;

failed_to_allocate_memory:
	EMIT_BF_ERROR("failed to allocate memory");

	goto set_error;

failed_to_read_file:
	EMIT_BF_ERROR_FMT("failed to read file: `%s`", file);

	goto set_error;

set_error:
	res = 1;

end:
	if (content) {
		free(content);
	}

	return res;
}

int
run__BfInterpreter(const char *file)
{
	FILE *file_stream = fopen(file, "r");
	int res = 0;

	if (!file_stream) {
		goto failed_to_open_file;
	}

	res = read_file__BfInterpreter(file, file_stream);

	if (fclose(file_stream) != 0) {
		goto failed_to_close_file;
	}

	goto end;

failed_to_open_file:
	EMIT_BF_ERROR_FMT("could not open file: `%s`", file);

	goto set_error;

failed_to_close_file:
	EMIT_BF_ERROR_FMT("failed to close file: `%s`", file);

	goto set_error;

set_error:
	res = 1;

end:
	return res;
}
