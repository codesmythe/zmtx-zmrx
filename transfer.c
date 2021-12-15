/*
 * 	(Quick & Dirty) Transfer Shell
 *
 *	Jwahar Bammi
 * 	bang:   {any internet host}!dsrgsun.ces.CWRU.edu!bammi
 * 	domain: bammi@dsrgsun.ces.CWRU.edu
 *	GEnie:	J.Bammi
 */

#include "config.h"

#ifdef DLIBS
#include <string.h>
#endif
#ifdef __GNUC__
#include <string.h>
#endif

#include "zmdm.h"
#include "common.h"

#define ISWILD(X)	((X == '*')||(X == '?'))
#define PROMPT		fprintf(STDERR,"zmdm> ")

extern int dorz(), dosz(), ls(), rm(), cp(), cd(), md(), rd(), pwd(), df(),
	   hhelp();

#ifdef RECURSE
extern int doszf();
#endif

static struct comnds {
	char	*command;	/* command string	*/
	int     (*routine)();	/* routine to invoke	*/
	char	*synopsis;	/* synopsis		*/
	int	expand;		/* expand wildcards before calling routine? */
} comtab [] = {
	{ "rz",	dorz, "receive files using Z/X modem protocol",       TRUE },
	{ "rb", dorz, "receive files using Y modem protocol",         TRUE },
#ifdef RECURSE
	{ "sz", doszf, "send files using Z/Y/X modem protocol",        TRUE },
#else
	{ "sz", dosz, "send files using Z/Y/X modem protocol",        TRUE },
#endif
	{ "sb", dosz, "send files using Y modem protocol",            TRUE },
	{ "rm", rm,   "remove files",                                 TRUE },
	{ "cp", cp,   "copy files",                                   TRUE },
	{ "ls", ls,   "list directory",                               FALSE},
	{ "cd", cd,   "change working directory",		      FALSE},
	{ "md", md,   "make a directory",			      FALSE},
	{ "rd", rd,   "remove a directory",			      FALSE},
	{ "pwd",pwd,  "print  working directory",		      FALSE},
	{ "df", df,   "check free space",			      FALSE},
	{ "?",  hhelp, "this message",				      FALSE},
	{ (char *)NULL, (int (*)())NULL, (char *)NULL,                FALSE}
};

#define MAXARGS	1024
static char *targv[MAXARGS];
static int targc;
char *alltolower();

void transfer()
{
	char linebuf[132];
	char *line;
	int command;
	int status;
	extern int find_command();
	extern int expnd_args();
#ifdef DEBUG
	int i;
#endif

	fprintf(STDERR,"hit <RETURN> to return to emulator,  <?> for help\n\n");
	targc = 0;
	while (TRUE)
	{
		if(targc > 1)
			free_args();

		PROMPT;
		linebuf[0] = 127;
#ifndef REMOTE
		Cconrs(linebuf);
#else
		Cconraux(linebuf);
#endif
		putc('\n', STDERR);
		if(linebuf[1] == 0)
		/* cancelled */
		return;
		
		linebuf[(linebuf[1]+2)] = '\0';
		line = &linebuf[2];
#ifdef DEBUG
printf("Line: |%s|\n", line);
#endif

		targv[0] = line;
		targc    = 1;
	
		/* pick up targv[0] */
		while((*line != '\0') && (!isspace(*line)))
			line++;

		if(*line != '\0')
		{
			*line++ = '\0';
		}

		if((command = find_command(targv[0])) < 0)
		{
			fprintf(STDERR,"Invalid Command\n");
			continue;
		}

		if(expnd_args(line, comtab[command].expand))
			/* too many args */
			continue;
#ifdef DEBUG
printf("targc %d\n", targc);
for(i = 0; i < targc; i++)
	printf("%s ", targv[i]);
printf("\n\n");
#endif

		if((status = (*(comtab[command].routine))(targc, targv)) != 0)
			fprintf(STDERR,"Exit Status %d\n", status);

#ifdef DEBUG
printf("Exit Status %d\n", status);
#endif

	} /* While */
}

/*
 * Straight sequential search thru comtab
 */		
int find_command(s)
register char *s;
{
	register int i;

	for(i = 0; comtab[i].command != (char *)NULL; i++)
	{
		if(strcmp(s, comtab[i].command) == 0)
			return i;
	}

	return -1;
}

/*
 * Expand command line args, return TRUE if too many args, or Not matching Quotes
 */
int expnd_args(s, expand_wild)
register char *s;
int expand_wild;
{
	char next_arg[128];
	register char *p;
	register int contains_wild;
	extern int add_argv();
	extern int handl_wild();

	while(*s != '\0')
	{
		p = next_arg;
		while(isspace(*s)) s++; /* skip leading space */
		if(*s != '\0')
		{
			contains_wild = FALSE;
			if(*s == '\047')
			{
				/* Quoted arg */
				s++;
				while((*s != '\0') && (*s != '\047'))
					*p++ = *s++;
				*p = '\0';
				if(*s == '\0')
				{
					fprintf(STDERR,"No Matching Quote\n");
					return TRUE;
				}
				else
					s++;
				if(add_argv(next_arg))
					return TRUE;
			}
			else
			{
				while(!isspace(*s) && (*s != '\0'))
				{
					contains_wild |= ISWILD(*s);
					*p++ = *s++;
				}
				*p = '\0';

				if(contains_wild && expand_wild)
				{
					if(handl_wild(next_arg))
						return TRUE;
				}
				else
				{
					if(add_argv(next_arg))
						return TRUE;
				}
			} /* if-else */
		} /* If */
	} /* while */

	return FALSE;
}

/*
 * add an arg to argv. Return TRUE if error
 */
int add_argv(s)
char *s;
{
	extern char *strcpy();
#ifdef __GNUC__
	extern size_t strlen();
	extern char *myalloc(unsigned int);
#else
	extern int strlen();
	extern char *myalloc();
#endif

	if(targc > (MAXARGS-1))
	{
		fprintf(STDERR,"Too many arguements (%d Max)\n", MAXARGS);
		return TRUE;
	}
	targv[targc++] = strcpy(myalloc((unsigned int)(strlen(s)+1)), s);
	
	return FALSE;
	
}

/*
 * expand wild card arguments. Return TRUE on error.
 */
int handl_wild(s)
char *s;
{
	extern struct stat statbuf;
	extern char *mkpathname();
	
	if(Fsfirst(s, 0x21) != 0)
	{
		/* No match */
		fprintf(STDERR,"No Match for %s\n", s);
		return TRUE;
	}

	alltolower(statbuf.st_name);
	if(add_argv(mkpathname(s, statbuf.st_name)))
		return TRUE;

	while(Fsnext() == 0)
	{
		alltolower(statbuf.st_name);
		if(add_argv(mkpathname(s, statbuf.st_name)))
			return TRUE;
	}

	return FALSE;
}

/*
 * Given a spec with a trailing wildcard and a base will name construct pathname
 *
 */
char *mkpathname(spec, file)
register char *spec, *file;
{
	extern char *rindex();
	register char *p;

	if((p = rindex(spec, '\\')) == (char *)NULL)
		/* no path name */
		return file;

	while(*file != '\0')
		*++p = *file++;
	*++p = '\0';
	
	return spec;
}

void free_args()
{
	register int i;
	
	for(i = 1; i < targc; i++)
		free(targv[i]);
}

/*
 * remove files
 *	Usage: rm [-i] files ... 
 */	
int rm(argc, argv)
register int argc;
register char **argv;
{
	register int interactive;
	register int status;
	extern int yesno();
	extern int errno;

	interactive = FALSE;
	status = 0;
	while(--argc)
	{
		++argv;
		if( ((*argv)[0] == '-') && ((*argv)[1] == 'i') )
			interactive = TRUE;
		else
		{
			if(interactive)
				if(!yesno("rm: remove", *argv))
					continue;
			if(unlink(*argv))
			{
				status++;
				fprintf(STDERR, "%s: no such file\n", *argv);
			}
		}
	}
	return status;
}

/*
 * Prompt and return Yes/No truth value
 *
 */
int yesno(p1, p2)
register char *p1, *p2;
{
	char reply[16];

	fprintf(STDERR,"%s %s (y/n): ", p1, p2);
	reply[0] = 16;
#ifndef REMOTE
	Cconrs(reply);
#else
	Cconraux(reply);
#endif

	putc('\n', STDERR);
	return ( (reply[2] == 'y') || (reply[2] == 'Y') );

}

/*
 * copy files
 *	Usage:
 *		cp src dest
 *	    or
 *		cp files.. directory
 */
int cp(argc, argv)
int argc;
char **argv;
{
	char dest[128];
	register int status;
#ifdef __GNUC__
	extern size_t strlen();
#else
	extern int strlen();
#endif
	extern int cpy();
	extern char *basename();
	
	status = 0;
	if(argc > 3)
	{
		register int i;

		if(!existd(argv[argc-1]))
		{
			fprintf(STDERR,"%s does not exists or is not a directory\n",
				argv[argc-1]);
			return 1;
		}

		for(i = 1; i < argc - 1; i++)
		{
			strcpy(dest, argv[argc-1]);
			if( (argv[argc-1])[((int)strlen(argv[argc-1])-1)] != '\\')
				strcat(dest, "\\");
			strcat(dest, basename(argv[i]));

			fprintf(STDERR,"copying %s to %s\n", argv[i], dest);
			status |= cpy(argv[i], dest);
		}
	}
	else
	{
		if(argc > 2)
		{
			if(existd(argv[2]))
			{
				/* dest is a directory */
				strcpy(dest, argv[2]);
				if( (argv[2])[((int)strlen(argv[2])-1)] != '\\')
					strcat(dest, "\\");

				strcat(dest, basename(argv[1]));

				fprintf(STDERR,"copying %s to %s\n", argv[1], dest);
				return (cpy(argv[1], dest));
			}

			if(strcmp(argv[1], argv[2]) == 0)
			{
				fprintf(STDERR,"Cannot copy a file onto itself\n");
				return 3;
			}
			status = cpy(argv[1], argv[2]);
		}
		else
		{
			fprintf(STDERR,"Usage: cp source dest or cp files .. directory\n");
			return 2;
		}
	}

	return status;
}


/*
 * Cpy src -> dest
 *
 */
int cpy(src, dest)
char *src, *dest;
{
	register int fps,fpd;
	register long count;
	register int status;

	status = 0;

	if((fps = Fopen(src, 0)) < (-3))
	{
		status = fps;
		fprintf(STDERR,"%s: no such file\n", src);
		return status;
	}

	if((fpd = Fcreate(dest, 0)) < (-3))
	{
		if((fpd = Fopen(dest, 1)) < (-3))
		{
			Fclose(fps);
			status = fpd;
			fprintf(STDERR,"%s: cannot create\n",dest);
			return status;
		}
	}
	
	while( (count = Fread(fps, (long)BBUFSIZ, bufr)) > 0L)
	{
		if(Fwrite(fpd, count, bufr) != count)
		{
			status = 1;
			fprintf(STDERR,"Error Writing %s\n",dest);
			break;
		}
	}
	if(count < 0L)
	{
		status = 2;
		fprintf(STDERR,"Error Reading %s\n", src);
	}

	Fclose(fpd);
	Fclose(fps);

	return status;
}

#define haswild(X) \
( (rindex(X,'*') != (char *)NULL) || (rindex(X,'?') != (char *)NULL) )

/*
 * list directories
 */
int ls(argc, argv)
int argc;
char **argv;
{
	char path[128];
	register char *p;
	extern char *rindex();
	extern int existd();

	if(argc < 2)
		lis("*.*");
	else
	{
		while(--argc)
		{
			++argv;
			if((p = rindex(*argv,'\\')) == (char *)NULL)
				p = *argv;
			else
				p++;
			if(*p == '\0')
			{
				strcpy(path, *argv);
				strcat(path,"*.*");
			}
			else
			{
				if(haswild(p))
					strcpy(path, *argv);
				else
				{
					if(existd(p))
					{
						strcpy(path, *argv);
						strcat(path, "\\*.*");
					}
					else
						strcpy(path, *argv);
				}
			}
			lis(path);
		} /* while */
	}
	
	return 0;
}

/*
 * given a possibly wild carded string put out list of subtrees
 *
 */
void lis(wild)
char *wild;
{
	extern struct stat statbuf;
	register int count;

#ifdef DEBUG
printf("ls %s\n", wild);
#endif

	if(Fsfirst(wild, 0x0020 | 0x0010) != 0)
	{
		fprintf(STDERR,"%s - no match.\n", wild);
		return;
	}

	count = 0;
	alltolower(statbuf.st_name);
	if(!((strcmp(statbuf.st_name,".") == 0) || 
	   (strcmp(statbuf.st_name, "..") == 0)))
		putls(&statbuf, ++count);

	while(Fsnext() == 0)
	{
		alltolower(statbuf.st_name);
		if(!((strcmp(statbuf.st_name,".") == 0) || 
		   (strcmp(statbuf.st_name, "..") == 0)))
			putls(&statbuf, ++count);
	}

	if((count % 4))
		putc('\n', STDERR);

}

/*
 * Put out a directory entry
 */
void putls(statbuf, count)
register struct stat *statbuf;
register int count;
{
	char dbuf[16];

	if((statbuf->st_mode) & 0x0010)
	{
		/* subtree */
		strcpy(dbuf, statbuf->st_name);
		strcat(dbuf, "/");
		fprintf(STDERR,"%-13s      ", dbuf);
	}
	else
		/* file */
		fprintf(STDERR,"%-12s %5ld ", statbuf->st_name, statbuf->st_size);
	
	if(!(count %4))
		putc('\n', STDERR);
}

/*
 * Change working directory
 */
int cd(argc, argv)
int argc;
char **argv;
{
	register char *d, *path;
	register int drive;
	extern char *index();

	if((argc < 2) || (argc > 2))
	{
		fprintf(STDERR,"Usage: cd directory\n");
		return 1;
	}


	
	if((d = index(argv[1],':')) == (char *)NULL)
	{
		/* Drive was not specified, must mean the current drive */
		path = argv[1];
	}
	else
	{
		d--;
		if(isupper(*d))
		{
			*d = tolower(*d);
		}
		drive = *d - 'a';
		if(d[2] != '\\')	/* just gave D: */
		{
			/* we will shove in the '\' */
			d[1] = '\\';
			path = &d[1];
		}
		else
			path = &d[2];

		/* Set the Drive */
		if(Dsetdrv(drive) < 0)
		{
			fprintf(STDERR,"Could not set drive %c:\n", *d);
			return 2;
		}

	}


	/* Set the Path */
	if(Dsetpath(path) != 0)
	{
		fprintf(STDERR,"Could not set directory %s\n", path);
		return 3;
	}
	
	return 0;
	
}

/*
 * pwd - figure out the current working directory
 *
 */
int pwd()
{
	register int drive;
	char _cwd[128];
	
	drive = Dgetdrv();
	Dgetpath(&_cwd[3],0);
	fprintf(STDERR,"%c:%s%s\n", (drive + 'a'),
		((_cwd[3] == '\\')? "" : "\\"), &_cwd[3]);

	return 0;
}

/*
 * Print free space
 */
int df(argc, argv)
int argc;
char **argv;
{
	struct {
	    long b_free;
	    long b_total;
	    long b_secsiz;
	    long b_clsiz;
	} fbuf;

	register int drive;
	register long ffree, total;
	extern long drv_map;

	if(argc > 1)
	{
		++argv;
		drive = (*argv)[0];
		if(isupper(drive))
			drive = tolower(drive);

		drive = drive - 'a';
		if((drv_map & (1L << drive)) == 0)
		{
			fprintf(STDERR,"Invalid Drive %c:\n", (drive + 'a'));
			return 1;
		}
		drive +=1;
	}
	else
		drive = Dgetdrv()+1;	/* default current drive */

	fprintf(STDERR,"Please Wait .....");
	Dfree(&fbuf,drive);
	ffree = fbuf.b_free * fbuf.b_clsiz * fbuf.b_secsiz;
	total =	fbuf.b_total * fbuf.b_clsiz * fbuf.b_secsiz;
	fprintf(STDERR,"\r%ld Bytes (", ffree);
	prratio(STDERR, ffree, total);
	fprintf(STDERR, ") Free on drive %c:\n", ((drive-1) + 'a'));

	return 0;
}


/*
 * print a ratio
 * avoid floats like the plague
 */
void prratio(stream, num, den)
FILE *stream;
long  num, den;
{
        register int q;                 /* Doesn't need to be long */

        if(num > 214748L) {             /* 2147483647/10000 */
                q = num / (den / 10000L);
        } else {
                q = 10000L * num / den;         /* Long calculations, though */
        }
        if (q < 0) {
                putc('-', stream);
                q = -q;
        }
        fprintf(stream, "%d.%02d%%", q / 100, q % 100);
}

/*
 * Make directories
 */
int md(argc, argv)
int argc;
char **argv;
{
	register int status, s;

	status = 0;
	while(--argc)
	{
		++argv;
		if(( s = Dcreate(*argv)))
		{
			status |= s;
			fprintf(STDERR,"Could not create %s\n", *argv);
		}
	}

	return status;
}

/*
 * Remove directories
 */
int rd(argc, argv)
int argc;
char **argv;
{
	register int status, s;

	status = 0;
	while(--argc)
	{
		++argv;
		if(( s = Ddelete(*argv)))
		{
			status |= s;
			fprintf(STDERR,"Could not delete %s\n", *argv);
		}
	}

	return status;
}


/*
 * Allocate memory with error check
 *
 */
char *myalloc(size)
unsigned int size;
{
	register char *m;
#ifdef __GNUC__
	extern void *malloc(size_t);
#else
	extern char *malloc();
#endif

#ifdef __GNUC__
	if((m = (char *)malloc((size_t)size)) == (char *)NULL)
#else
	if((m = (char *)malloc(size)) == (char *)NULL)
#endif
	{
		fprintf(STDERR,"Out of Memory\nSorry, cannot continue\n");
		exit(1);
	}

	return m;
}

/*
 * Put out help
 */
int hhelp()
{
	register int i;

#ifdef REMOTE
	fprintf(STDERR,"\n\t\t\t    Available Commands \n\n");
#else
	fprintf(STDERR,"\n\t\t\t   \033p Available Commands \033q\n\n");
#endif
	for(i = 0; comtab[i].command != (char *)NULL; i++)
	{
		fprintf(STDERR,"\t%s\t%s\n", comtab[i].command,
			comtab[i].synopsis);
	}
	fprintf(STDERR,"\n\tor hit <RETURN> to exit to terminal emulator\n\n");
	return 0;

}

/*
 * Lower case string
 */
char *alltolower(s)
register char *s;
{
	register char *p;

	for(p = s; *p != '\0'; p++)
	{
		if(isupper(*p))
			*p = tolower(*p);
	}
	return s;
}

/*
 * return the base filename, given (potentially) a pathname
 */
char *basename(s)
register char *s;
{
	register char *p;
	extern char *rindex();

	if((p = rindex(s, '\\')) == (char *)NULL)
	{
		if((p = rindex(s, ':')) == (char *)NULL)
			return s;
		else
			return ++p;
	}
	return ++p;
}

#ifdef RECURSE
int doszf(argc, argv)
int argc;
char **argv;
{
	extern int expandargs();
#ifdef DDEBUG
int i;
printf("doszf: argc %d\n", argc);
for(i = 0; i < argc; i++)
	printf("%s ", argv[i]);
printf("\n\n");
#endif

	return(expandargs(dosz, argc, argv));
}

#include "expandar.c"

#endif

#ifdef REMOTE
/*
 * Read a line from the Aux port
 *
 */

void Cconraux(l)
char *l;
{
	register int c;
	register int count;
	register char *p;

	p = &l[2];
	count = 0;

	while((c = get_a_c()) != '\n')
	{
		*p++ = c;
		count++;
	}
	l[1] = count;
}

static char _chr_buf[128];
static int _n_chr = 0;
static char *_next_chr = &_chr_buf[0];
static char *_next_avail = &_chr_buf[0];

int get_a_c()
{
        register int c;

        if(_n_chr > 0)
        {
                _n_chr--;
                return(*_next_avail++);
        }
        
        _n_chr = 0;
        _next_avail = &_chr_buf[0];
        _next_chr = &_chr_buf[0];

        while( ((c = Bconin(1) & 0x7fff) != '\r') && (c != '\n'))
        {
                if((c == '\003') || (c == '\025'))
                {
			while(_n_chr-- > 0)
				wr_modem("\b \b");
                        _n_chr = 0;
                        _next_avail = &_chr_buf[0];
                        _next_chr = &_chr_buf[0];

                }

                if((c == '\b') || (c == '\177'))
                {
                        if(_n_chr > 0)
                        {
                                _next_chr--;
                                _n_chr--;
                                wr_modem("\b \b");
                        }
                }
                else
                {
                        *_next_chr++ = c;
                        _n_chr++;
			Bconout(1, c);
                }
        }

        *_next_chr = '\n';
        return(*_next_avail++);
}
#endif /* REMOTE */

/* -eof- */
