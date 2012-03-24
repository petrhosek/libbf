#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>

#include <bf_func.h>

/*
 * Gets path to currently running executable.
 */
bool get_target_path(char * target_path, size_t size)
{
	char *   path;
	ssize_t len;

	pid_t pid = getpid();
	sprintf(target_path, "/proc/%d/exe", (int)pid );

	path = strdup(target_path);
	len  = readlink(path, target_path, size);

	target_path[len] = '\0';
	free(path);
	return TRUE;
}

/*
 * The aim of this program is to detour execution so func2 is called instead of
 * func1. It is an elementary test for the basic functionality of bf_detour.
 */
void patch_func1_func2(void)
{
	struct binary_file * bf			   = NULL;
	char		     target_path[PATH_MAX] = {0};

	/* Get path for the running instance of this program */
	get_target_path(target_path, ARRAY_SIZE(target_path));

	bf = load_binary_file(target_path);
	// bf_detour_func(bf, func1, func2);
	close_binary_file(bf);
}

/*
 * func1 is invoked by the regular execution of detour_test.
 */
void func1(void)
{
	puts("func1 was invoked");
}

/*
 * func2 is not invoked by the regular execution of detour_test. The aim is to
 * detour execution to this function.
 */
void func2(void)
{
	puts("func2 was invoked");
}

int main(void)
{
	patch_func1_func2();
	func1();
	return EXIT_SUCCESS;
}
