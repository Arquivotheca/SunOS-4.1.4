#ifndef lint
static	char sccsid[] = "@(#)ypserv_net_secure.c 1.1 94/10/31 Copyr 1992 Sun Micro";
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <syslog.h>
#define ACCFILE "/var/yp/securenets"
#define MAXLINE 128
struct seclist {
     u_long mask;
     u_long net;
    struct seclist *next;
};
static struct seclist *slist ;
static int nofile = 0;
get_secure_nets(ypname)
char **ypname;
{
    FILE *fp;
    char strung[MAXLINE],nmask[MAXLINE],net[MAXLINE];
    unsigned long maskin, netin;
    struct seclist *tmp1,*tmp2;
    int line = 0;
    if (fp = fopen(ACCFILE,"r")) {
	tmp1 = (struct seclist *) malloc(sizeof (struct seclist));
	slist = tmp1;
	while (fgets(strung,MAXLINE,fp)) {
	    line++;
 	    if (strung[strlen(strung) - 1] != '\n'){
 		syslog(LOG_ERR|LOG_DAEMON,
 		       "%s: %s line %d: too long\n",*ypname,ACCFILE,line);
 		exit(1);
 	    }
 	    if (strung[0] == '#')
 		continue;
 	    if (sscanf(strung,"%16s%16s",nmask,net) < 2) {
 		syslog(LOG_ERR|LOG_DAEMON,
 		       "%s: %s line %d: missing fields\n",*ypname,ACCFILE,line);
 		exit(1);
 	    }
 	    maskin = inet_addr(nmask);
	    if (maskin == -1
	    && strcmp(nmask, "255.255.255.255") != 0
	    && strcmp(nmask, "host") != 0) {
 		syslog(LOG_ERR|LOG_DAEMON,
 		       "%s: %s line %d: error in netmask\n",*ypname,ACCFILE,line);
 		exit(1);
 	    }
 	    netin = inet_addr(net);
 	    if (netin == -1) {
 		syslog(LOG_ERR|LOG_DAEMON,
 		       "%s: %s line %d: error in address\n",*ypname,ACCFILE,line);
 		exit(1);
 	    }

 	    if ((maskin & netin) != netin) {
 		syslog(LOG_ERR|LOG_DAEMON,
 		       "%s: %s line %d: netmask does not match network\n",
 		       *ypname,ACCFILE,line);
 		exit(1);
 	    }
 	    tmp1->mask = maskin;
 	    tmp1->net = netin;
 	    tmp1->next = (struct seclist *) malloc(sizeof (struct seclist));
 	    tmp2 = tmp1;
 	    tmp1 = tmp1->next;
 	}
 	tmp2->next = NULL;

     }
     else {
 	syslog(LOG_WARNING|LOG_DAEMON,"%s: no %s file\n",*ypname,ACCFILE);
 	nofile = 1 ;
     }
 }

check_secure_net(caller, ypname)
struct sockaddr_in *caller;
char *ypname;
{

     struct seclist *tmp;
     tmp = slist ;
     if (nofile)
 	return(1);
     while (tmp != NULL) {
 	if ((caller->sin_addr.s_addr & tmp->mask) == tmp->net){
 	    return(1);
 	}
 	tmp = tmp->next;
     }
     syslog(LOG_ERR|LOG_DAEMON,"%s: access denied for %s\n",
 	   ypname,inet_ntoa(caller->sin_addr));
     return(0);
}

