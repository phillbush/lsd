#include <err.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#define POSX    200
#define POSY    200
#define WIDTH   350
#define HEIGHT  250
#define BORDER  10

static Display *dpy;
static Window root;
static int screen;
static Atom utf8string;
static Atom wmhints;
static Atom netnumberofdesktops;
static Atom netcurrentdesktop;
static Atom netwmdesktop;
static Atom netdesktopnames;
static Atom netclientlist;

/* call calloc checking for errors */
static void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;
	if ((p = calloc(nmemb, size)) == NULL)
		err(1, "calloc");
	return p;
}

/* get aray of windows, return number of windows */
static unsigned long
getwinlist(Window **winlist)
{
	unsigned char *list;
	unsigned long len;
	unsigned long dl;   /* dummy variable */
	unsigned int du;    /* dummy variable */
	int di;             /* dummy variable */
	Window dw;          /* dummy variable */
	Atom da;            /* dummy variable */

	if (XGetWindowProperty(dpy, root, netclientlist, 0L, 1024, False,
	                       XA_WINDOW, &da, &di, &len, &dl, &list) ==
	                       Success && list) {
		*winlist = (Window *)list;
	} else if (XQueryTree(dpy, root, &dw, &dw, winlist, &du)) {
		len = (unsigned long)du;
	} else {
		errx(1, "could not get list of windows");
	}

	return len;
}

/* get array of desktop names, return size of array */
static unsigned long
getdesknames(char **desknames)
{
	unsigned char *str;
	unsigned long len;
	unsigned long dl;   /* dummy variable */
	int di;             /* dummy variable */
	Atom da;            /* dummy variable */


	if (XGetWindowProperty(dpy, root, netdesktopnames, 0, ~0, False,
	                       utf8string, &da, &di, &len, &dl, &str) ==
	                       Success && str) {
		*desknames = (char *)str;
	} else {
		*desknames = NULL;
		len = 0;
	}

	return len;
}

/* get cardinal property */
static unsigned long
getcardinalproperty(Window win, Atom prop)
{
	unsigned char *p = NULL;
	unsigned long dl;   /* dummy variable */
	int di;             /* dummy variable */
	Atom da;            /* dummy variable */
	unsigned long retval;

	if (XGetWindowProperty(dpy, win, prop, 0L, 1L, False, XA_CARDINAL,
	                       &da, &di, &dl, &dl, &p) == Success && p) {
		retval = *(unsigned long *)p;
		XFree(p);
	} else {
		errx(1, "XGetWindowProperty");
	}

	return retval;
}

/* select inputs we are interested in in all clients */
static void
selectwininput(void)
{
	unsigned long i, nwins;
	Window *wins;

	nwins = getwinlist(&wins);
	for (i = 0; i < nwins; i++)
		XSelectInput(dpy, wins[i], PropertyChangeMask | StructureNotifyMask);
	XFree(wins);
}

/* get properties and print information */
static void
printinfo(int tabs)
{
	unsigned long i, nwins, desk, ndesks, curdesk, len, nameslen;
	unsigned long *wdesk;
	int *urgdesks;
	Window *wins;
	XWMHints *hints;
	char *desknames;

	/* get variables */
	ndesks = getcardinalproperty(root, netnumberofdesktops);
	curdesk = getcardinalproperty(root, netcurrentdesktop);
	nameslen = getdesknames(&desknames);
	wdesk = ecalloc(ndesks, sizeof *wdesk);
	urgdesks = ecalloc(ndesks, sizeof *urgdesks);
	nwins = getwinlist(&wins);
	for (i = 0; i < nwins; i++) {
		desk = getcardinalproperty(wins[i], netwmdesktop);
		hints = XGetWMHints(dpy, wins[i]);
		if (desk < ndesks) {
			wdesk[desk]++;
			if (hints && hints->flags & XUrgencyHint) {
				urgdesks[desk] = 1;
			}
		}
		XFree(hints);
	}
	XFree(wins);

	/* list desktops */
	for (len = i = 0; i < ndesks; i++) {
		printf("%c%lu:%s%s",
		       (i == curdesk ? '*' : (urgdesks[i] ? '-' : ' ')),
		       wdesk[i],
		       (desknames && len < nameslen ? desknames+len : ""),
		       (tabs ? (i + 1 < ndesks ? "\t" : "\n") : "\n"));
		if (desknames && len < nameslen)
			len += strlen(desknames + len) + 1;
	}
	fflush(stdout);
	XFree(desknames);
	free(wdesk);
}

/* lsd: list desktops */
int
main(int argc, char *argv[])
{
	XEvent ev;

	/* open connection to the server */
	if ((dpy = XOpenDisplay(NULL)) == NULL)
		errx(1, "could not open display");
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);

	/* intern atoms */
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);;
	wmhints = XInternAtom(dpy, "WM_HINTS", False);;
	netclientlist = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	netnumberofdesktops = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
	netcurrentdesktop = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
	netwmdesktop = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
	netdesktopnames = XInternAtom(dpy, "_NET_DESKTOP_NAMES", False);

	(void)argv;
	if (argc == 1) {
		printinfo(0);
	} else {
		XSelectInput(dpy, root, PropertyChangeMask | SubstructureNotifyMask);
		selectwininput();
		printinfo(1);
		while (!XNextEvent(dpy, &ev)) {
			selectwininput();
			if (ev.type == ClientMessage ||
			   (ev.type == PropertyNotify &&
			   (ev.xproperty.atom == wmhints ||
			    ev.xproperty.atom == netclientlist ||
			    ev.xproperty.atom == netnumberofdesktops ||
			    ev.xproperty.atom == netcurrentdesktop ||
			    ev.xproperty.atom == netwmdesktop ||
			    ev.xproperty.atom == netdesktopnames))) {
				printinfo(1);
	 		}
		}
	}

	/* close connection to the server */
	XCloseDisplay(dpy);

	return 0;
}
