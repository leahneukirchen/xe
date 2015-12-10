/*
 * xe - simple xargs and apply replacement
 *
 * To the extent possible under law,
 * Christian Neukirchen <chneukirchen@gmail.com>
 * has waived all copyright and related or neighboring rights to this work.
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static char delim = '\n';
static char default_replace[] = "{}";
static char *replace = default_replace;
static char *argsep;
static char *sflag;

static int maxatonce = 1;
static int maxjobs = 1;
static int runjobs = 0;
static int Rflag, Aflag, aflag, kflag, nflag, vflag;
static long iterations = 0;
static FILE *traceout;

static size_t argmax;
static int push_overflowed;

static char *buf;
static size_t buflen;
static size_t bufcap;

static char **args;
static size_t argslen;
static size_t argscap;

static char *line = 0;
static size_t linelen = 0;

static char *
getarg()
{
	if (aflag || Aflag) {
		if (args && *args)
			return *args++;
		else
			return 0;
	}

	int read = getdelim(&line, &linelen, delim, stdin);
	if (read == -1) {
		if (feof(stdin))
			return 0;
		else
			exit(1);
	}
	if (line[read-1] == delim)  // strip delimiter
		line[read-1] = 0;

	return line;
}

static int
mywait()
{
	int status;
	pid_t pid;

	pid = wait(&status);
	if (pid < 0) {
		if (errno == ECHILD)
			return 0;
		// no other error possible?
	}	
	
	if (WIFEXITED(status) && !kflag) {
		if (WEXITSTATUS(status) >= 1 && WEXITSTATUS(status) <= 125) {
			exit(123);
		} else if (WEXITSTATUS(status) == 255) {
			fprintf(stderr, "xe: %d exited with status 255\n", pid);
			exit(124);
		} else if (WEXITSTATUS(status) > 125) {
			exit(WEXITSTATUS(status));
		}
	} else if (WIFSIGNALED(status)) {
		fprintf(stderr, "xe: %d terminated by signal %d\n",
		    pid, WTERMSIG(status));
		exit(125);
	}
	
	runjobs--;
	return 1;
}

static void
shquote(const char *s)
{
	if (!strpbrk(s, "\001\002\003\004\005\006\007\010"
	                "\011\012\013\014\015\016\017\020"
	                "\021\022\023\024\025\026\027\030"
	                "\031\032\033\034\035\036\037\040"
	                "`^#*[]=|\\?${}()'\"<>&;\127")) {
		fprintf(traceout, "%s", s);
		return;
	}

	fprintf(traceout, "\'");
	for (; *s; s++)
		if (*s == '\'')
			fprintf(traceout, "'\\''");
		else
			fprintf(traceout, "%c", *s);
	fprintf(traceout, "\'");
}

static int
trace()
{
	size_t i;

	for (i = 0; i < argslen; i++) {
		if (i > 0)
			fprintf(traceout, " ");
		shquote(args[i]);
	}
	fprintf(traceout, "\n");

	return 0;
}

static void
scanargs()
{
	char *s = buf;
	size_t i;

	for (i = 0; i < argslen; i++) {
		args[i] = s;
		s += strlen(s) + 1;
	}
	args[i] = 0;
}

static int
pusharg(const char *a)
{
	size_t l = strlen(a) + 1;   // including nul

	if (buflen >= argmax - l || argslen + 1 >= argscap) {
		push_overflowed = 1;
		return 0;
	}

	if (buflen + l > bufcap) {
		while (buflen + l > bufcap)
			bufcap *= 2;
		buf = realloc(buf, bufcap);
		if (!args)
			exit(1);
	}

	memcpy(buf + buflen, a, l);
	buflen += l;
	argslen++;

	return 1;
}

static int
run()
{
	pid_t pid;

	if (runjobs >= maxjobs)
		mywait();
	runjobs++;
	iterations++;

	scanargs();

	if (vflag || nflag)
		trace();

	if (nflag) {
		runjobs--;
		return 0;
	}

	pid = fork();
	if (pid == 0) {  // in child
		char iter[32];
		snprintf(iter, sizeof iter, "%ld", iterations);
		setenv("ITER", iter, 1);
		// redirect stdin to /dev/null
		int fd = open("/dev/null", O_RDONLY);
		if (fd >= 0) {
			if (dup2(fd, 0) != 0)
				exit(1);
			close(fd);
			execvp(args[0], args);
		}
		fprintf(stderr, "xe: %s: %s\n", args[0], strerror(errno));
		exit(errno == ENOENT ? 127 : 126);
	}

	if (pid < 0)
		exit(126);

	return 0;
}

int
main(int argc, char *argv[], char *envp[])
{
	char c;
	int i, cmdend;
	char *arg;

	bufcap = 4096;
	buf = malloc(bufcap);

	argscap = 8192;
	args = malloc(sizeof args[0] * argscap);

	if (!buf || !args)
		exit(1);

	argmax = sysconf(_SC_ARG_MAX);
	while (*envp)  // subtract size of environment
		argmax -= strlen(*envp++) + 1 + sizeof(*envp);
	argmax -= 4 * 1024;  // subtract 4k for safety
	if (argmax > 128 * 1024)  // upper bound
		argmax = 128 * 1024;
	if (argmax <= 0) {  // lower bound
		argmax = _POSIX_ARG_MAX;

	traceout = stdout;

	while ((c = getopt(argc, argv, "+0A:I:N:Raj:kns:v")) != -1)
		switch(c) {
		case '0': delim = '\0'; break;
		case 'A': argsep = optarg; Aflag++; break;
		case 'I': replace = optarg; break;
		case 'N': maxatonce = atoi(optarg); break;
		case 'R': Rflag++; break;
		case 'a': aflag++; break;
		case 'j': maxjobs = atoi(optarg); break;
		case 'k': kflag++; break;
		case 'n': nflag++; break;
		case 's': sflag = optarg; break;
		case 'v': vflag++; traceout = stderr; break;
		default:
			fprintf(stderr, 
			    "Usage: %s [-0Rknv] [-I arg] [-N maxargs] [-j maxjobs] COMMAND...\n"
			    "     | -s SHELLSCRIPT\n"
			    "     | -a COMMAND... -- ARGS...\n"
			    "     | -A ARGSEP COMMAND... ARGSEP ARGS...\n",
			    argv[0]);
			exit(1);
		}

	cmdend = argc;
	if (aflag) {  // find first -- in argv
		for (i = 1; i < argc; i++)
			if (strcmp(argv[i], "--") == 0) {
				args = argv + i+1;
				cmdend = i;
				break;
			}
		if (sflag) {  // e.g. on xe -s 'echo $1' -a 1 2 3
			cmdend = optind;
			args = argv + cmdend;
		}
	} else if (Aflag) {  // find first argsep after optind
		for (i = optind; i < argc; i++) {
			if (strcmp(argv[i], argsep) == 0) {
				args = argv + i+1;
				cmdend = i;
				break;
			}
		}
	}

	while ((arg = getarg())) {
keeparg:
		buflen = 0;
		argslen = 0;
		push_overflowed = 0;

		if (sflag) {
			pusharg("/bin/sh");
			pusharg("-c");
			pusharg(sflag);
			pusharg("-");
		} else if (optind == cmdend) {
			pusharg("printf");
			pusharg("%s\\n");
		}

		if (maxatonce == 1) {
			// substitute {}
			int substituted = 0;
			for (i = optind; i < cmdend; i++) {
				if (strcmp(argv[i], replace) == 0) {
					pusharg(arg);
					substituted = 1;
				} else {
					pusharg(argv[i]);
				}
			}
			if (!substituted)
				pusharg(arg);
		} else {
			// just append to cmd
			for (i = optind; i < cmdend; i++)
				pusharg(argv[i]);
			pusharg(arg);
			if (!push_overflowed) {
				for (i = 0; maxatonce < 1 || i < maxatonce - 1; i++) {
					arg = getarg();
					if (!arg)
						break;
					if (!pusharg(arg)) {
						run();
						goto keeparg;
					}
				}
			}
		}

		if (push_overflowed) {
			fprintf(stderr, "xe: fixed argument list too long\n");
			exit(1);
		}
		run();
	}

	while (mywait())
		;

	free(buf);
	free(args);
	free(line);

	if (Rflag && iterations == 0)
		return 122;
	return 0;
}
