// 6 april 2015
#include "uipriv_unix.h"

uiInitOptions options;

// kind of a hack
GThread* gtkthread;
GMutex glmutex;

static void _eventfilter(GdkEvent* evt, gpointer data)
{
    if (evt->type == GDK_EXPOSE)
    {
        g_mutex_lock(&glmutex);
        gtk_main_do_event(evt);
        g_mutex_unlock(&glmutex);
        return;
    }
    
    gtk_main_do_event(evt);
}

static void _eventfilterdestroy(gpointer data)
{
    printf("DELET\n");
}

const char *uiInit(uiInitOptions *o)
{
	GError *err = NULL;
	const char *msg;

	options = *o;
	if (gtk_init_with_args(NULL, NULL, NULL, NULL, NULL, &err) == FALSE) {
		msg = g_strdup(err->message);
		g_error_free(err);
		return msg;
	}
	initAlloc();
	loadFutures();
	
	gtkthread = g_thread_self();
	g_mutex_init(&glmutex);
	
	GList* iconlist = NULL;
	iconlist = g_list_append(iconlist, gdk_pixbuf_new_from_resource("/org/kuriboland/melonDS/icon/melon_16x16.png", NULL));
	iconlist = g_list_append(iconlist, gdk_pixbuf_new_from_resource("/org/kuriboland/melonDS/icon/melon_32x32.png", NULL));
	iconlist = g_list_append(iconlist, gdk_pixbuf_new_from_resource("/org/kuriboland/melonDS/icon/melon_48x48.png", NULL));
	iconlist = g_list_append(iconlist, gdk_pixbuf_new_from_resource("/org/kuriboland/melonDS/icon/melon_64x64.png", NULL));
	iconlist = g_list_append(iconlist, gdk_pixbuf_new_from_resource("/org/kuriboland/melonDS/icon/melon_128x128.png", NULL));
	
	gtk_window_set_default_icon_list(iconlist);
	
	g_mutex_init(&glmutex);
	
	gdk_event_handler_set(_eventfilter, NULL, _eventfilterdestroy);
	
	return NULL;
}

void uiUninit(void)
{
	uninitMenus();
	uninitAlloc();
}

void uiFreeInitError(const char *err)
{
	g_free((gpointer) err);
}

static gboolean (*iteration)(gboolean) = NULL;

void uiMain(void)
{
	iteration = gtk_main_iteration_do;
	gtk_main();
}

static gboolean stepsQuit = FALSE;

// the only difference is we ignore the return value from gtk_main_iteration_do(), since it will always be TRUE if gtk_main() was never called
// gtk_main_iteration_do() will still run the main loop regardless
static gboolean stepsIteration(gboolean block)
{
	gtk_main_iteration_do(block);
	return stepsQuit;
}

void uiMainSteps(void)
{
	iteration = stepsIteration;
}

int uiMainStep(int wait)
{
	gboolean block;

	block = FALSE;
	if (wait)
		block = TRUE;
	return (*iteration)(block) == FALSE;
}

// gtk_main_quit() may run immediately, or it may wait for other pending events; "it depends" (thanks mclasen in irc.gimp.net/#gtk+)
// PostQuitMessage() on Windows always waits, so we must do so too
// we'll do it by using an idle callback
static gboolean quit(gpointer data)
{
	if (iteration == stepsIteration)
		stepsQuit = TRUE;
		// TODO run a gtk_main() here just to do the cleanup steps of syncing the clipboard and other stuff gtk_main() does before it returns
	else
		gtk_main_quit();
	return FALSE;
}

void uiQuit(void)
{
	gdk_threads_add_idle(quit, NULL);
}

struct queued {
	void (*f)(void *);
	void *data;
};

static gboolean doqueued(gpointer data)
{
	struct queued *q = (struct queued *) data;

	(*(q->f))(q->data);
	g_free(q);
	return FALSE;
}

void uiQueueMain(void (*f)(void *data), void *data)
{
	struct queued *q;

	// we have to use g_new0()/g_free() because uiAlloc() is only safe to call on the main thread
	// for some reason it didn't affect me, but it did affect krakjoe
	q = g_new0(struct queued, 1);
	q->f = f;
	q->data = data;
	gdk_threads_add_idle(doqueued, q);
}
