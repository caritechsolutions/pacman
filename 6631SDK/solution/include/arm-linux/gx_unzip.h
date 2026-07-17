#ifndef GX_UNZIP_H
#define GX_UNZIP_H 1

//usage:#define unzip_trivial_usage
//usage:       "[-lnopq] FILE[.zip] [FILE]... [-x FILE...] [-d DIR]"
//usage:#define unzip_full_usage "\n\n"
//usage:       "Extract FILEs from ZIP archive\n"
//usage:     "\n	-l	List contents (with -q for short form)"
//usage:     "\n	-n	Never overwrite files (default: ask)"
//usage:     "\n	-o	Overwrite"
//usage:     "\n	-p	Print to stdout"
//usage:     "\n	-q	Quiet"
//usage:     "\n	-x FILE	Exclude FILEs"
//usage:     "\n	-d DIR	Extract into DIR"
int gx_unzip_main(int argc, char **argv);

#endif
