/*
 * xe - simple xargs and apply replacement
 *
 * To the extent possible under law, Leah Neukirchen <leah@vuxu.org>
 * has waived all copyright and related or neighboring rights to this work.
 * http://creativecommons.org/publicdomain/zero/1.0/
 */

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char delim = '\n';
static char default_replace[] = "{}";
static char *replace = default_replace;
static char *argsep;
static char *fflag;
static char *sflag;

static int maxatonce = 1;
static int maxjobs = 1;
static int runjobs = 0;
static int failed = 0;
static int Aflag, Fflag, Lflag, Rflag, aflag, nflag, pflag, qflag, vflag;
static long iterations = 0;
static FILE *traceout;
static FILE *input;

static size_t argmax;

static char *buf;
static size_t buflen;
static size_t bufcap;

static char **args;
static size_t argslen;
static size_t argscap;
static size_t argsresv;

struct child {
	pid_t pid;  // 0 if empty slot
	long iter;
};
static struct child *children;

static char **inargs;

static char *line = 0;
static size_t linelen = 0;

static char *
getarg()
{
	if (aflag || Aflag) {
		if (inargs && *inargs)
			return *inargs++;
		else
			return 0;
	}

	int read = getdelim(&line, &linelen, delim, input);
	if (read == -1) {
		if (feof(input))
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
	if (pid < 0)
		return !(errno == ECHILD);

	int i;
	for (i = 0; i < maxjobs; i++)
		if (children[i].pid == pid)
			goto my_child;

	return 1;

my_child:
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) >= 1 && WEXITSTATUS(status) <= 125) {
			if (Fflag) {
				fprintf(stderr, "xe: job %ld [pid %ld] exited with status %d\n", children[i].iter, (long)pid, WEXITSTATUS(status));
				exit(123);
			}
			failed = 1;
		} else if (WEXITSTATUS(status) == 255) {
			fprintf(stderr, "xe: job %ld [pid %ld] exited with status 255\n", children[i].iter, (long)pid);
			exit(124);
		} else if (WEXITSTATUS(status) > 125) {
			exit(WEXITSTATUS(status));
		}
	} else if (WIFSIGNALED(status)) {
		fprintf(stderr, "xe: job %ld [pid %ld] terminated by signal %d\n",
		    children[i].iter, (long)pid, WTERMSIG(status));
		exit(125);
	}

	if (vflag > 1) {
		fprintf(traceout, "%04ld< status %d\n",
		    children[i].iter, WEXITSTATUS(status));
		fflush(traceout);
	}

	children[i].pid = 0;
	runjobs--;

	return 1;
}

static void
sh_quote(const char *s)
{
	if (*s &&
	    !strpbrk(s, "\001\002\003\004\005\006\007\010"
	                "\011\012\013\014\015\016\017\020"
	                "\021\022\023\024\025\026\027\030"
	                "\031\032\033\034\035\036\037\040"
	                "`^#*[]=|\\?${}()'\"<>&;\177")) {
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
		sh_quote(args[i]);
	}
	fprintf(traceout, "\n");
	fflush(traceout);

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

	if (buflen >= argmax - l - argsresv || argslen + 1 >= argscap)
		return 0;

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
	pid_t pid, lpid;

	while (runjobs >= maxjobs)
		mywait();
	runjobs++;
	iterations++;

	scanargs();

	if (!args[0]) {
		fprintf(stderr, "xe: no command\n");
		exit(126);
	}

	if (nflag) {
		trace();
		runjobs--;
		return 0;
	}

	int pipefd[2];
	if (Lflag) {
		if (pipe(pipefd) < 0)
			exit(126);
	}

	pid = fork();
	if (pid == 0) {  // in child
		char iter[32];
		snprintf(iter, sizeof iter, "%ld", iterations);
		setenv("ITER", iter, 1);
		// redirect stdin to /dev/null when we read arguments from it
		// just close it if this fails
		if (input == stdin) {
			int fd = open("/dev/null", O_RDONLY);
			if (fd >= 0) {
				if (dup2(fd, 0) != 0)
					close(0);
				close(fd);
			} else {
				close(0);
			}
		}

		if (Lflag) {
			if (dup2(pipefd[1], 1) < 0)
				exit(126);
			close(pipefd[0]);
			close(pipefd[1]);
		}

		if (qflag) {
			int fd = open("/dev/null", O_RDONLY);
			if (fd >= 0) {
				dup2(fd, 2);
				dup2(fd, 1);
				close(fd);
			}
		}

		execvp(args[0], args);

		int status = (errno == ENOENT ? 127 : 126);
		fprintf(stderr, "xe: %s: %s\n", args[0], strerror(errno));
		exit(status);
	} else if (pid < 0) {  // fork failed
		fprintf(stderr, "xe: %s: %s\n", args[0], strerror(errno));
		exit(126);
	}

	if (Lflag) {
		long iter = iterations;
		lpid = fork();
		if (lpid == 0) {  // in line-logging child
			char *line = 0;
			size_t linelen = 0;

			close(0);
			close(pipefd[1]);
			FILE *input = fdopen(pipefd[0], "r");
			if (!input)
				exit(126);

			setvbuf(stdout, 0, _IOLBF, 0);

			while (1) {
				int rd = getdelim(&line, &linelen, '\n', input);
				if (rd == -1)
					exit(0);

				if (vflag > 1 || Lflag > 1)
					printf("%04ld= ", iter);
				fwrite(line, 1, rd, stdout);
			}
		}
		if (lpid < 0)
			exit(126);

		close(pipefd[0]);
		close(pipefd[1]);
	}

	if (vflag) {
		if (vflag > 1)
			fprintf(traceout, "%04ld> ", iterations);
		trace();
	}

	int i;
	for (i = 0; i < maxjobs; i++)
		if (!children[i].pid) {
			children[i].pid = pid;
			children[i].iter = iterations;
			break;
		}

	return 0;
}

void
toolong()
{
	fprintf(stderr, "xe: fixed argument list too long\n");
	exit(1);
}

int
parse_jobs(char *s)
{
	char *e;
	int n;

	if (*s && s[strlen(s) - 1] == 'x') {
#ifdef _SC_NPROCESSORS_ONLN
		n = (int)sysconf(_SC_NPROCESSORS_ONLN);
#else
		n = 1;
#endif
		double d = 0.0;
		errno = 0;
		d = strtod(s, &e);
		if (errno != 0 || *e != 'x' || d <= 0) {
			fprintf(stderr, "xe: can't parse multiplier '%s'.\n", s);
			exit(1);
		}
		n = (int)(d * n);
		if (n < 1)
			n = 1;
	} else if (strcmp(s, "j") == 0) {
		n = -1;
	} else {
		errno = 0;
		n = strtol(s, &e, 10);
		if (errno != 0 || *e) {
			fprintf(stderr, "xe: can't parse number '%s'.\n", s);
			exit(1);
		}
	}

#ifdef _SC_NPROCESSORS_ONLN
	if (n <= 0)
		n = (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif
	if (n <= 0)
		n = 1;

	return n;
}

// perc is a variant of fnmatch, implemented here using left derivatives:
//
// differences to fnmatch:
// ** matches any substring
// % matches any nonempty substring (longest first), is kept for substitution
// [!...] and [^...] are supported
// [] matches nothing
// [...] never matches /
// {a,b,c} alternation
// {} matches empty string
// no special prefix . handling: *le matches .profile
// no collations in []

size_t perc_len;
char *perc_str;

char *
perc(char *pat, char *str, int lvl)
{
	char *s, *e;
	int neg, matched;
	size_t l;

	switch (*pat) {
	case '\0':
		if (lvl > 0) {
			fprintf(stderr, "xe: unmatched { in glob\n");
			exit(1);
		}
		return *str ? 0 : str;
	case '?':
		if (!*str || *str == '/')
			return 0;
		return perc(pat+1, str+1, lvl);
	case '/':
		if (*str != '/')
			return 0;
		while (*pat == '/')
			pat++;
		while (*str == '/')
			str++;
		return perc(pat, str, lvl);
	case '*':
		pat++;
		if (*pat == '*') {  // any substring
			while (*pat == '*')
				pat++;
			l = strlen(str) + 1;
		} else {  // any substring but no more than the next /
			if ((e = strchr(str, '/')))
				l = e - str + 1;
			else
				l = strlen(str) + 1;
		}
		while (l--)
			if ((s = perc(pat, str+l, lvl)))
				return s;
		return 0;
	case '%':
		// any nonempty substring
		for (l = strlen(str); l >= 1; l--)
			if ((s = perc(pat+1, str+l, lvl))) {
				perc_str = str;
				perc_len = l;
				return s;
			}
		return 0;
	case '[':
		if (!*str || *str == '/')
			return 0;
		pat++;
		neg = 0;
		if (*pat == '^' || *pat == '!') {
			pat++;
			neg = 1;
		}
		for (matched = 0; *pat && *pat != ']'; pat++) {
			if (*pat == '\\')
				pat++;
			if (pat[1] == '-' && pat[2] != ']') {
				if (pat[0] <= *str && *str <= pat[2])
					matched = 1;
				pat += 2;
			} else if (*str == *pat) {
				matched = 1;
			}
		}
		if (*pat != ']') {
			fprintf(stderr, "xe: unmatched [ in glob\n");
			exit(1);
		}
		return (neg ^ matched) ? perc(pat+1, str+1, lvl) : 0;
	case '{':
		e = 0;
		while (*pat++ != '}') {
			if (!e)
				e = perc(pat, str, lvl+1);
			// skip to next , or }
			for (l = 0;
			     *pat && !(l == 0 && (*pat == ',' || *pat == '}'));
			     pat++)
				if (*pat == '{')
					l++;
				else if (*pat == '}')
					l--;
				else if (*pat == '[')
					while (*pat && *pat != ']') {
						if (*pat == '\\' && pat[1])
							pat++;
						pat++;
					}
		}
		return e ? perc(pat, e, lvl) : 0;
	case ',':
	case '}':
		if (lvl > 0)
			return str;
		/* FALL THROUGH */
	default:
		return (*pat == *str) ? perc(pat+1, str+1, lvl) : 0;
	}
}

int
perc_match(char *pat, char *arg)
{
	if (!strchr(pat, '/') && !strstr(pat, "**")) {
		char *d = strrchr(arg, '/');
		if (d)
			arg = d + 1;
	}

	perc_len = 0;
	perc_str = 0;

	return perc(pat, arg, 0) != 0;
}

char *
perc_subst(char *pat, char *base, char *arg)
{
	static char buf[2048];
	size_t l;
	char *dir = base;

	if (strcmp(arg, replace) == 0)
		return base;

	if (!strchr(pat, '/') && !strstr(pat, "**")) {
		char *d = strrchr(base, '/');
		if (d)
			base = d + 1;
	}

	char *t = strchr(arg, '%');

	if (!t)
		return arg;

	if (perc_len)
		l = snprintf(buf, sizeof buf, "%.*s%.*s%.*s%.*s",
		    (int)(base - dir),
		    dir,

		    (int)(t - arg),
		    arg,

		    (int)perc_len,
		    perc_str,

		    (int)(arg + strlen(arg) - t),
		    t+1);
	else
		l = snprintf(buf, sizeof buf, "%.*s%.*s%s%.*s",
		    (int)(base - dir),
		    dir,

		    (int)(t - arg),
		    arg,

		    base,

		    (int)(arg + strlen(arg) - t),
		    t+1);

	if (l >= sizeof buf) {
		fprintf(stderr, "xe: result of percent substitution too long\n");
		exit(1);
	}

	return buf;
}

static void
trace_to_stderr()
{
	FILE *linestderr = fdopen(2, "w");
	if (linestderr) {
		setvbuf(linestderr, 0, _IOLBF, 0);
		traceout = linestderr;
	} else {
		traceout = stderr;
	}
}

int
main(int argc, char *argv[], char *envp[])
{
	int c, i, j, cmdend;
	char *arg;

	bufcap = 4096;
	buf = malloc(bufcap);

	argscap = 8192;
	args = malloc(sizeof args[0] * argscap);

	if (!buf || !args)
		exit(1);

	argmax = sysconf(_SC_ARG_MAX);
	while (*envp)  // subtract size of environment
		argmax -= strlen(*envp++) + 1 + sizeof *envp;
	argmax -= 4 * 1024;  // subtract 4k for safety
	if (argmax > 128 * 1024)  // upper bound
		argmax = 128 * 1024;
	if (argmax <= 0)  // lower bound
		argmax = _POSIX_ARG_MAX;

	traceout = stdout;

	while ((c = getopt(argc, argv, "+0A:FI:LN:Raf:j:npqs:v")) != -1)
		switch (c) {
		case '0': delim = '\0'; break;
		case 'A': argsep = optarg; Aflag++; break;
		case 'I': replace = optarg; break;
		case 'L': Lflag++; break;
		case 'N': maxatonce = atoi(optarg); break;
		case 'F': Fflag++; break;
		case 'R': Rflag++; break;
		case 'a': aflag++; break;
		case 'f': fflag = optarg; break;
		case 'j': maxjobs = parse_jobs(optarg); break;
		case 'n': nflag++; break;
		case 'p': pflag++; break;
		case 'q': qflag++; break;
		case 's': sflag = optarg; break;
		case 'v': vflag++; break;
		default:
			fprintf(stderr,
			    "Usage: %s [-0FLRnqv] [-I arg] [-N maxargs] [-j maxjobs] COMMAND...\n"
			    "     | -p PATTERN COMMAND... [+ PATTERN COMMAND...]...\n"
			    "     | -f ARGFILE COMMAND...\n"
			    "     | -s SHELLSCRIPT\n"
			    "     | -a COMMAND... -- ARGS...\n"
			    "     | -A ARGSEP COMMAND... ARGSEP ARGS...\n",
			    argv[0]);
			exit(1);
		}

	children = calloc(sizeof (struct child), maxjobs);
	if (!children)
		exit(1);

	if (!Lflag || vflag <= 1)
		trace_to_stderr();

	if (aflag || Aflag) {
		input = 0;
	} else if (!fflag || strcmp("-", fflag) == 0) {
		input = stdin;
	} else {
		input = fopen(fflag, "rb");
		if (!input) {
			fprintf(stderr, "xe: opening %s: %s\n",
			    fflag, strerror(errno));
			exit(1);
		}
		fcntl(fileno(input), F_SETFD, FD_CLOEXEC);
	}

	cmdend = argc;
	if (aflag) {  // find first -- in argv
		for (i = 1; i < argc; i++)
			if (strcmp(argv[i], "--") == 0) {
				inargs = argv + i+1;
				cmdend = i;
				break;
			}
		if (sflag) {  // e.g. on xe -s 'echo $1' -a 1 2 3
			cmdend = optind;
			inargs = argv + cmdend;
		}
	} else if (Aflag) {  // find first argsep after optind
		for (i = optind; i < argc; i++) {
			if (strcmp(argv[i], argsep) == 0) {
				inargs = argv + i+1;
				cmdend = i;
				break;
			}
		}
		if (i >= argc) {
			fprintf(stderr, "xe: '-A %s' used but no separator "
			    "'%s' found in command line.\n", argsep, argsep);
			exit(1);
		}
	}

	int keeparg = 0;

	if (pflag) {
		if (maxatonce != 1) {
			fprintf(stderr,
			    "xe: -p cannot be used together with -N.\n");
			exit(1);
		}

		while (1) {
			buflen = 0;
			argslen = 0;

			while (runjobs >= maxjobs)
				mywait();
			if (!(arg = getarg()))
				break;

			int n;
			for (n = optind, i = n + 1; n < cmdend; n = i + 1) {
				char *pat = argv[n];

				int matched = perc_match(pat, arg);

				if (sflag && matched) {
					pusharg("/bin/sh");
					pusharg("-c");
					pusharg(sflag);
					pusharg("/bin/sh");
				}

				for (i = n + 1; i < cmdend; i++) {
					if (argv[i][0] == '+' &&
					    argv[i][1] == '\0')
						break;

					if (matched &&
					    !pusharg(perc_subst(pat, arg, argv[i])))
						toolong();
				}

				if (matched) {
					run();
					break;
				}
			}
		}
	} else {
		while (1) {
			// check if there is an arg from a previous iteration
			if (!keeparg) {
				while (runjobs >= maxjobs)
					mywait();

				arg = getarg();
				if (!arg)
					break;
			}
			keeparg = 0;

			buflen = 0;
			argslen = 0;

			if (sflag) {
				pusharg("/bin/sh");
				pusharg("-c");
				pusharg(sflag);
				pusharg("/bin/sh");
			} else if (optind >= cmdend) {
				pusharg("printf");
				pusharg("%s\\n");
			}

			for (i = optind; i < cmdend; i++) {
				if (*replace && strcmp(argv[i], replace) == 0)
					break;
				if (!pusharg(argv[i]))
					toolong();
			}

			if (!pusharg(arg))
				toolong();
			i++;

			// reserve space for final arguments
			for (argsresv = 0, j = i; j < cmdend; j++)
				argsresv += 1 + strlen(argv[j]);

			// fill with up to maxatonce arguments
			for (j = 0; maxatonce < 1 || j < maxatonce-1; j++) {
				arg = getarg();
				if (!arg)
					break;
				if (!pusharg(arg)) {
					// we are out of space,
					// deal with this arg in the next iter
					keeparg = 1;
					break;
				}
			}

			for (argsresv = 0, j = i; j < cmdend; j++)
				if (!pusharg(argv[j]))
					toolong();

			run();
		}
	}

	while (mywait())
		;

	free(buf);
	free(args);
	free(line);

	if (Rflag && iterations == 0)
		return 122;
	if (failed)
		return 123;
	return 0;
}
