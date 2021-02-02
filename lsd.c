#include <err.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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
static Atom netnumberofdesktops;
static Atom netcurrentdesktop;
static Atom netwmdesktop;

/* call calloc checking for errors */
static void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;
	if ((p = calloc(nmemb, size)) == NULL)
		err(1, "calloc");
	return p;
}

/* get list of windows, return number of windows */
static unsigned long
getwinlist(Window **winlist)
{
	unsigned char *list;
	unsigned long len;
	Atom netclientlist;
	unsigned long dl;   /* dummy variable */
	unsigned int du;    /* dummy variable */
	int di;             /* dummy variable */
	Window dw;          /* dummy variable */
	Atom da;            /* dummy variable */

	netclientlist = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
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

/* xhello: create and display a basic window with default white background */
int
main(void)
{
	unsigned long i, nwins, desk, ndesks, curdesk;
	unsigned long *wdesk;
	Window *wins;

	/* open connection to the server */
	if ((dpy = XOpenDisplay(NULL)) == NULL)
		errx(1, "could not open display");
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);

	/* intern atoms */
	netnumberofdesktops = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
	netcurrentdesktop = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
	netwmdesktop = XInternAtom(dpy, "_NET_WM_DESKTOP", False);

	/* get variables */
	ndesks = getcardinalproperty(root, netnumberofdesktops);
	curdesk = getcardinalproperty(root, netcurrentdesktop);
	wdesk = ecalloc(ndesks, sizeof *wdesk);
	nwins = getwinlist(&wins);
	for (i = 0; i < nwins; i++) {
		desk = getcardinalproperty(wins[i], netwmdesktop);
		if (desk < ndesks) {
			wdesk[desk]++;
		}
	}
	XFree(wins);

	/* list desktops */
	for (i = 0; i < ndesks; i++) {
		printf("%c%lu:%lu\n", (i == curdesk ? '*' : ' '), (unsigned long)i, (unsigned long)wdesk[i]);
	}

	/* close connection to the server */
	XCloseDisplay(dpy);

	return 0;
}
