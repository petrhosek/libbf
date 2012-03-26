#include "bf_copy.h"

bfd * bf_create_writable_bfd(char * input_path, bfd * abfd, char * output_path)
{
	/* Copying with objcopy first. This is just to check our test works. */

	char * cmd  = "objcopy ";
	char * cmd2 = " ";
	char copy_cmd[strlen(cmd) + strlen(input_path) +
		strlen(cmd2) + strlen(output_path) + 1];

	strcpy(copy_cmd, cmd);
	strcat(copy_cmd, input_path);
	strcat(copy_cmd, cmd2);
	strcat(copy_cmd, output_path);

	if(system(copy_cmd)) {
		perror("Problem copying");
		xexit(-1);
	}

	return bfd_openr(input_path, NULL);
}
