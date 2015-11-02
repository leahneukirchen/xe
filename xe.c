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
static char **args;
static char *sflag;

static int maxatonce = 1;
static int maxjobs = 1;
static int runjobs = 0;
static int Rflag, Aflag, aflag, kflag, nflag, vflag;
static long iterations = 0;

static char *
xstrdup(const char *s)
{
	char *d = strdup(s);
	if (!d)
		exit(1);
	return d;
}

static char *getarg_line = 0;
static size_t getarg_len = 0;

static char *
getarg()
{
	if (aflag || Aflag) {
		if (args && *args)
			return xstrdup(*args++);
		else
			return 0;
	}

	int read = getdelim(&getarg_line, &getarg_len, delim, stdin);
	if (read == -1) {
		if (feof(stdin))
			return 0;
		else
			exit(1);
	}
	if (getarg_line[read-1] == delim)  // strip delimiter
		getarg_line[read-1] = 0;

	return xstrdup(getarg_line);
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
		printf("%s", s);
		return;
	}

	putchar('\'');
	for (; *s; s++)
		if (*s == '\'')
			printf("'\\''");
		else
			putchar(*s);
	putchar('\'');
}

static int
trace(char *cmd[])
{
	int i;
	
	for (i = 0; cmd[i]; i++) {
		if (i > 0)
			printf(" ");
		shquote(cmd[i]);
	}
	printf("\n");

	return 0;
}

static int
run(char *cmd[])
{
	pid_t pid;
	int i;

	if (runjobs >= maxjobs)
		mywait();
	runjobs++;
	iterations++;

	if (vflag || nflag)
		trace(cmd);
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
			execvp(cmd[0], cmd);
		}
		fprintf(stderr, "xe: %s: %s\n", cmd[0], strerror(errno));
		exit(errno == ENOENT ? 127 : 126);
	}

	if (pid < 0)
		exit(126);

	for (i = 0; cmd[i]; i++)
		free(cmd[i]);
	
	return 0;
}

int
main(int argc, char *argv[])
{
	char c;
	int i, cmdend;
	char *arg, **cmd;

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
		case 'v': vflag++; break;
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

	cmd = calloc(argc-optind+maxatonce+1+
	    (optind==cmdend ? 2 : 0)+(sflag ? 4 : 0), sizeof (char *));
	if (!cmd)
		exit(1);

	while ((arg = getarg())) {
		int l = 0;

		if (sflag) {
			cmd[l++] = xstrdup("/bin/sh");
			cmd[l++] = xstrdup("-c");
			cmd[l++] = xstrdup(sflag);
			cmd[l++] = xstrdup("-");
		} else if (optind == cmdend) {
			cmd[l++] = xstrdup("printf");
			cmd[l++] = xstrdup("%s\\n");
		}

		if (maxatonce == 1) {
			// substitute {}
			int substituted = 0;
			for (i = optind; i < cmdend; i++) {
				if (strcmp(argv[i], replace) == 0) {
					cmd[l++] = arg;
					substituted = 1;
				} else {
					cmd[l++] = xstrdup(argv[i]);
				}
			}
			if (!substituted)
				cmd[l++] = arg;
		} else {
			// just append to cmd
			for (i = optind; i < cmdend; i++)
				cmd[l++] = xstrdup(argv[i]);
			cmd[l++] = arg;
			for (i = 0; i < maxatonce - 1; i++) {
				cmd[l] = getarg();
				if (!cmd[l++])
					break;
			}
		}
		cmd[l] = 0;
		run(cmd);
	}

	while (mywait())
		;

	free(cmd);
	free(getarg_line);
	if (Rflag && iterations == 0)
		return 122;
	return 0;
}
