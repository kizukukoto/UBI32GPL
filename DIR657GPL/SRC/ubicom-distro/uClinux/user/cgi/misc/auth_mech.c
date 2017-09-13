#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/wait.h>

int call_cmd(char *cmd)
{
	int status, pret, ret = -1;
	pid_t pid;

	pid = fork();

	if (pid == -1)
		goto out;

	if (pid == 0) {
		execv(cmd, (char *[]){cmd, NULL});
		exit(-1);
	}

	if ((pret = waitpid(pid, &status, 0)) == -1)    /* NO PID RETURN */
		goto out;
	else if (pret == 0)                             /* NO CHILD RETURN */
		goto out;
	else if (WIFEXITED(status))                     /* EXIT NORMALLY */
		ret =  WEXITSTATUS(status);
out:
	return ret;
}

static int auth_local_precall()
{
	setenv("DB_INDEX", getenv("PARAM"), 1);
	return 0;
}

int auth_mech_main(int argc, char *argv[])
{
	int ret = -1;
	char *auth = getenv("AUTH_METHOD");
	struct {
		char *method;		/* Authentication Method */
		char *routine	;	/* Authentication Routine */
		int (*func)(void);	/* do something before routine called */
	} *p, t[] = {
		{ "DB", "/bin/auth_local", auth_local_precall},
		{ NULL, NULL, NULL }
	};

	system("env > /tmp/auth_mech");
	if (auth == NULL || getenv("PARAM") == NULL)
		goto out;

	for (p = t; p->method && p->routine; p++) {
		if (strcmp(auth, p->method) == 0) {
			if (p->func)
			       	if (p->func() != 0)
					continue;
			printf("call: %s\n", p->routine);
			return call_cmd(p->routine);
		}
	}

	ret = 0;
out:
	return ret;
}
