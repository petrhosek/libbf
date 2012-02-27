#include <stdlib.h>
#include <bfd.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <libiberty.h>

/*
 * Gets the current directory.
 */
bool get_root_folder(char * path, size_t size)
{
	return getcwd(path, size) != NULL;
}

/*
 * Get target.
 */
bool get_target(char * path, size_t size)
{
	if(!get_root_folder(path, size)) {
		return FALSE;
	} else {
		int target_desc;

		strncat(path, "/Hello", size - strlen(path) - 1);
		target_desc = open(path, O_RDONLY);

		if(target_desc == -1) {	
			return FALSE;
		} else {
			close(target_desc);
			return TRUE;
		}
	}
}

/*
 * Get embed object.
 */
bool get_embed_object(char * path, size_t size)
{
	if(!get_root_folder(path, size)) {
		return FALSE;
	} else {
		int target_desc;

		strncat(path, "/Embed.o", size - strlen(path) - 1);
		target_desc = open(path, O_RDONLY);

		if(target_desc == -1) {	
			return FALSE;
		} else {
			close(target_desc);
			return TRUE;
		}
	}
}

bool for_each_symbol(bfd * abfd,
		void (*handler)(bfd * abfd, asymbol *, void *),
		void * param)
{
	long  storage_needed = bfd_get_symtab_upper_bound(abfd);

	if(storage_needed < 0) {
		return FALSE;
	} else if(storage_needed == 0) {
		return TRUE;
	} else {
		asymbol **symbol_table    = xmalloc(storage_needed);
		long    number_of_symbols = bfd_canonicalize_symtab(abfd, symbol_table);

		if(number_of_symbols < 0) {
			free(symbol_table);
			return FALSE;
		} else {
			for(long i = 0; i < number_of_symbols; i++) {
				handler(abfd, symbol_table[i], param);
			}
		}

		free(symbol_table);
		return TRUE;
	}
}

int main(void)
{
	bfd * abfd_target;
	bfd * abfd_embed;

	char target[FILENAME_MAX]    = {0};
	char embed_obj[FILENAME_MAX] = {0};

	get_target(target, ARRAY_SIZE(target));
	get_embed_object(embed_obj, ARRAY_SIZE(embed_obj));

	abfd_target = bfd_openw(target, NULL);
	bfd_set_format(abfd_target, bfd_object);

	if(abfd_target == NULL) {
		perror("Failed to load target.");
		xexit(-1);
	}

	abfd_embed = bfd_openr(embed_obj, NULL);

	if(abfd_embed == NULL) {
		perror("Failed to load object to be embedded.");
		xexit(-1);
	}


	asection * sec = bfd_make_section(abfd_target, "lalasection");
	if(sec == NULL) {
		puts("sec was NULL");
	}

	if(!bfd_set_section_size(abfd_target, sec, 1024)) {
		perror("bfd_set_section_size returned false.");
	}

	bfd_close(abfd_target);
	bfd_close(abfd_embed);
	return EXIT_SUCCESS;
}
