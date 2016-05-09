/* See LICENSE file for license and copyright information */

#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 500

#include <regex.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <math.h>
#include <libsoup/soup.h>
#include <unique/unique.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <webkit/webkit.h>
#include <JavaScriptCore/JavaScript.h>

/* macros */
#define LENGTH(x) sizeof(x)/sizeof((x)[0])
#define ALL_MASK (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK)
#define GET_CURRENT_TAB_WIDGET() GET_NTH_TAB_WIDGET(gtk_notebook_get_current_page(Jumanji.UI.view))
#define GET_NTH_TAB_WIDGET(n) GTK_SCROLLED_WINDOW(gtk_notebook_get_nth_page(Jumanji.UI.view, n))
#define GET_CURRENT_TAB() GET_NTH_TAB(gtk_notebook_get_current_page(Jumanji.UI.view))
#define GET_NTH_TAB(n) GET_WEBVIEW(gtk_notebook_get_nth_page(Jumanji.UI.view, n))
#define GET_WEBVIEW(x) WEBKIT_WEB_VIEW(gtk_bin_get_child(GTK_BIN(x)))

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

/* enums */
enum {
  APPEND_URL = 1,
  BACKWARD,
  BOTTOM,
  BYPASS_CACHE,
  DEFAULT,
  DELETE_LAST_CHAR,
  DELETE_LAST_WORD,
  DOWN,
  ERROR,
  FORWARD,
  FULL_DOWN,
  FULL_UP,
  HALF_DOWN,
  HALF_UP,
  HIDE,
  HIGHLIGHT,
  LEFT,
  LEFT_MAX,
  NEXT,
  NEXT_CHAR,
  NEXT_GROUP,
  NEW_TAB,
  OPEN_EXTERNAL,
  PREVIOUS,
  PREVIOUS_CHAR,
  PREVIOUS_GROUP,
  RIGHT,
  RIGHT_MAX,
  SPECIFIC,
  TOP,
  UP,
  WARNING,
  XA_PRIMARY,
  XA_SECONDARY,
  XA_CLIPBOARD,
  ZOOM_IN,
  ZOOM_ORIGINAL,
  ZOOM_OUT
};

/* define modes */
enum mode {
  NORMAL             = 1 << 0,
  INSERT             = 1 << 1,
  VISUAL             = 1 << 2,
  FOLLOW             = 1 << 3,
  ADD_MARKER         = 1 << 4,
  EVAL_MARKER        = 1 << 5,
  PASS_THROUGH       = 1 << 6,
  PASS_THROUGH_NEXT  = 1 << 7,
  ALL                = 0x7fffffff
};

/* typedefs */
struct CElement
{
  char            *value;
  char            *description;
  struct CElement *next;
};

typedef struct CElement CompletionElement;

struct CGroup
{
  char              *value;
  CompletionElement *elements;
  struct CGroup     *next;
};

typedef struct CGroup CompletionGroup;

typedef struct
{
  CompletionGroup* groups;
} Completion;

typedef struct
{
  char*      command;
  char*      description;
  int        command_id;
  gboolean   is_group;
  GtkWidget* row;
} CompletionRow;

typedef struct
{
  int   n;
  void *data;
} Argument;

typedef struct
{
  char* name;
  int argument;
} ArgumentName;

typedef struct
{
  unsigned int mask;
  unsigned int key;
  void (*function)(Argument*);
  int mode;
  Argument argument;
} Shortcut;

typedef struct
{
  char* name;
  int mode;
} ModeName;

typedef struct
{
  char* name;
  void (*sc)(Argument*);
  void (*bcmd)(char*, Argument*);
} FunctionName;

typedef struct
{
  unsigned int mask;
  unsigned int key;
  void (*function)(Argument*);
  Argument argument;
} InputbarShortcut;

typedef struct
{
  char* command;
  char* abbr;
  gboolean (*function)(int, char**);
  Completion* (*completion)(char*);
  char* description;
} Command;

typedef struct
{
  char* regex;
  void (*function)(char*, Argument*);
  Argument argument;
} BufferCommand;

struct BCList
{
  BufferCommand  element;
  struct BCList *next;
};

typedef struct BCList BufferCommandList;

typedef struct
{
  char identifier;
  gboolean (*function)(char*, Argument*, gboolean);
  int always;
  Argument argument;
} SpecialCommand;

struct SCList
{
  Shortcut       element;
  struct SCList *next;
};

typedef struct SCList ShortcutList;

typedef struct
{
  unsigned int mask;
  unsigned int button;
  void (*function)(Argument*);
  int mode;
  Argument argument;
} Mouse;

typedef struct
{
  char* identifier;
  unsigned int key;
} GDKKey;

typedef struct
{
  char* name;
  void* variable;
  char* webkitvar;
  char  type;
  gboolean init_only;
  gboolean reload;
  gboolean webkitview;
  char* description;
} Setting;

struct SEList
{
  char* name;
  char* uri;
  struct SEList *next;
};

typedef struct SEList SearchEngineList;

struct SScript
{
  char* path;
  char* content;
  struct SScript *next;
};

typedef struct SScript ScriptList;

typedef struct
{
  int id;
  int tab_id;
  gdouble hadjustment;
  gdouble vadjustment;
  float zoom_level;
} Marker;

typedef struct
{
  gchar     *name;
  gchar     *uris;
} Session;

/* jumanji */
struct
{
  struct
  {
    GtkWidget       *window;
    GtkBox          *box;
    GtkWidget       *tabbar;
    GtkWidget       *statusbar;
    GtkBox          *statusbar_entries;
    GtkEntry        *inputbar;
    GtkNotebook     *view;
    GdkNativeWindow  embed;
    char            *winid;
  } UI;

  struct
  {
    GdkColor default_fg;
    GdkColor default_bg;
    GdkColor inputbar_fg;
    GdkColor inputbar_bg;
    GdkColor statusbar_fg;
    GdkColor statusbar_bg;
    GdkColor statusbar_ssl_fg;
    GdkColor statusbar_ssl_bg;
    GdkColor tabbar_fg;
    GdkColor tabbar_bg;
    GdkColor tabbar_focus_fg;
    GdkColor tabbar_focus_bg;
    GdkColor tabbar_separator;
    GdkColor completion_fg;
    GdkColor completion_bg;
    GdkColor completion_g_bg;
    GdkColor completion_g_fg;
    GdkColor completion_hl_fg;
    GdkColor completion_hl_bg;
    GdkColor notification_e_fg;
    GdkColor notification_e_bg;
    GdkColor notification_w_fg;
    GdkColor notification_w_bg;
    PangoFontDescription *font;
  } Style;

  struct
  {
    GString *buffer;
    GList   *command_history;
    int      mode;
    char   **arguments;
    GList   *markers;
    GList   *bookmarks;
    GList   *sessions;
    GList   *history;
    GList   *last_closed;
    SearchEngineList  *search_engines;
    ScriptList        *scripts;
    WebKitWebSettings *browser_settings;
    GdkKeymap         *keymap;
    gboolean init_ui;
  } Global;

  struct
  {
    SoupSession* session;
  } Soup;

  struct
  {
    GtkLabel *text;
    GtkLabel *buffer;
    GtkLabel *tabs;
    GtkLabel *position;
  } Statusbar;

  struct
  {
    ShortcutList  *sclist;
    BufferCommandList *bcmdlist;
  } Bindings;

} Jumanji;

/* function declarations */
void add_marker(int);
gboolean auto_save(gpointer);
void change_mode(int);
void close_tab(int);
GtkWidget* create_tab(char*, gboolean);
void eval_marker(int);
void init_data();
void init_directories();
void init_jumanji();
void init_keylist();
void init_settings();
void init_ui();
void load_all_scripts();
void notify(int, char*);
void new_window(char*);
void out_of_memory();
void open_uri(WebKitWebView*, char*);
void read_configuration();
char* read_file(const char*);
char* reference_to_string(JSContextRef, JSValueRef);
void run_script(char*, char**, char**);
gboolean search_and_highlight(Argument*);
gboolean sessionload(char*);
gboolean sessionsave(char*);
gboolean sessionswitch(char*);
void set_completion_row_color(GtkBox*, int, int);
void switch_view(GtkWidget*);
void update_status();
void update_uri();
void update_position();
GtkEventBox* create_completion_row(GtkBox*, char*, char*, gboolean);

Completion* completion_init();
CompletionGroup* completion_group_create(char*);
void completion_add_group(Completion*, CompletionGroup*);
void completion_free(Completion*);
void completion_group_add_element(CompletionGroup*, char*, char*);

/* shortcut declarations */
void sc_abort(Argument*);
void sc_change_buffer(Argument*);
void sc_change_mode(Argument*);
void sc_close_tab(Argument*);
void sc_focus_inputbar(Argument*);
void sc_follow_link(Argument*);
void sc_nav_history(Argument*);
void sc_nav_tabs(Argument*);
void sc_paste(Argument*);
void sc_reload(Argument*);
void sc_reopen(Argument*);
void sc_run_script(Argument*);
void sc_scroll(Argument*);
void sc_search(Argument*);
void sc_spawn(Argument*);
void sc_toggle_proxy(Argument*);
void sc_toggle_statusbar(Argument*);
void sc_toggle_sourcecode(Argument*);
void sc_toggle_tabbar(Argument*);
void sc_quit(Argument*);
void sc_yank(Argument*);
void sc_zoom(Argument*);

/* inputbar shortcut declarations */
void isc_abort(Argument*);
void isc_completion(Argument*);
void isc_command_history(Argument*);
void isc_string_manipulation(Argument*);

/* command declarations */
gboolean cmd_back(int, char**);
gboolean cmd_bmap(int, char**);
gboolean cmd_bookmark(int, char**);
gboolean cmd_forward(int, char**);
gboolean cmd_map(int, char**);
gboolean cmd_open(int, char**);
gboolean cmd_print(int, char**);
gboolean cmd_quit(int, char**);
gboolean cmd_quitall(int, char**);
gboolean cmd_reload(int, char**);
gboolean cmd_reload_all(int, char**);
gboolean cmd_saveas(int, char**);
gboolean cmd_script(int, char**);
gboolean cmd_search_engine(int, char**);
gboolean cmd_sessionload(int, char**);
gboolean cmd_sessionsave(int, char**);
gboolean cmd_sessionswitch(int, char**);
gboolean cmd_set(int, char**);
gboolean cmd_stop(int, char**);
gboolean cmd_tabopen(int, char**);
gboolean cmd_winopen(int, char**);
gboolean cmd_write(int, char**);

/* completion commands */
Completion* cc_open(char*);
Completion* cc_session(char*);
Completion* cc_set(char*);

/* buffer command declarations */
void bcmd_close_tab(char*, Argument*);
void bcmd_go_home(char*, Argument*);
void bcmd_go_parent(char*, Argument*);
void bcmd_nav_history(char*, Argument*);
void bcmd_nav_tabs(char*, Argument*);
void bcmd_paste(char*, Argument*);
void bcmd_quit(char*, Argument*);
void bcmd_scroll(char*, Argument*);
void bcmd_spawn(char*, Argument*);
void bcmd_toggle_sourcecode(char*, Argument*);
void bcmd_zoom(char*, Argument*);

/* special command delcarations */
gboolean scmd_search(char*, Argument*, gboolean);

/* callback declarations */
UniqueResponse cb_app_message_received(UniqueApp*, gint, UniqueMessageData*, guint, gpointer);
gboolean cb_blank();
gboolean cb_destroy(GtkWidget*, gpointer);
gboolean cb_inputbar_kb_pressed(GtkWidget*, GdkEventKey*, gpointer);
void cb_inputbar_changed(GtkEditable*, gpointer);
gboolean cb_inputbar_activate(GtkEntry*, gpointer);
gboolean cb_tab_kb_pressed(GtkWidget*, GdkEventKey*, gpointer);
gboolean cb_tab_clicked(GtkWidget*, GdkEventButton*, gpointer);
gboolean cb_wv_button_release_event(GtkWidget*, GdkEvent*, gpointer);
gboolean cb_wv_console(WebKitWebView*, char*, int, char*, gpointer);
GtkWidget* cb_wv_create_web_view(WebKitWebView*, WebKitWebFrame*, gpointer);
gboolean cb_wv_download_request(WebKitWebView*, WebKitDownload*, gpointer);
gboolean cb_wv_hover_link(WebKitWebView*, char*, char*, gpointer);
WebKitWebView* cb_wv_inspector_view(WebKitWebInspector*, WebKitWebView*, gpointer);
gboolean cb_wv_mimetype_policy_decision(WebKitWebView*, WebKitWebFrame*, WebKitNetworkRequest*, char*, WebKitWebPolicyDecision*, gpointer);
gboolean cb_wv_notify_progress(WebKitWebView*, GParamSpec*, gpointer);
gboolean cb_wv_notify_title(WebKitWebView*, GParamSpec*, gpointer);
gboolean cb_wv_nav_policy_decision(WebKitWebView*, WebKitWebFrame*, WebKitNetworkRequest*, WebKitWebNavigationAction*, WebKitWebPolicyDecision*, gpointer);
gboolean cb_wv_scrolled(GtkAdjustment*, gpointer);
gboolean cb_wv_window_policy_decision(WebKitWebView*, WebKitWebFrame*, WebKitNetworkRequest*, WebKitWebNavigationAction*, WebKitWebPolicyDecision*, gpointer);
gboolean cb_wv_window_object_cleared(WebKitWebView*, WebKitWebFrame*, gpointer, gpointer, gpointer);

/* configuration */
#include "config.h"

/* function implementation */
void
add_marker(int id)
{
  if((id < 0x30) || (id > 0x7A))
    return;

  GtkAdjustment* adjustment;
  adjustment = gtk_scrolled_window_get_vadjustment(GET_CURRENT_TAB_WIDGET());
  gdouble va = gtk_adjustment_get_value(adjustment);
  adjustment = gtk_scrolled_window_get_hadjustment(GET_CURRENT_TAB_WIDGET());
  gdouble ha = gtk_adjustment_get_value(adjustment);
  float zl   = webkit_web_view_get_zoom_level(GET_CURRENT_TAB());
  int ti     = gtk_notebook_get_current_page(Jumanji.UI.view);

  /* search if entry already exists */
  GList* list;
  for(list = Jumanji.Global.markers; list; list = g_list_next(list))
  {
    Marker* marker = (Marker*) list->data;

    if(marker->id == id)
    {
      marker->tab_id      = ti;
      marker->vadjustment = va;
      marker->hadjustment = ha;
      marker->zoom_level  = zl;
      return;
    }
  }

  /* add new marker */
  Marker* marker = malloc(sizeof(Marker));
  marker->id          = id;
  marker->tab_id      = ti;
  marker->vadjustment = va;
  marker->hadjustment = ha;
  marker->zoom_level  = zl;

  Jumanji.Global.markers = g_list_append(Jumanji.Global.markers, marker);
}

gboolean
auto_save(gpointer UNUSED(data))
{
  cmd_write(0, NULL);

  return TRUE;
}

void
change_mode(int mode)
{
  char* mode_text = NULL;

  switch(mode)
  {
    case INSERT:
      mode_text = "-- INSERT --";
      break;
    case VISUAL:
      mode_text = "-- VISUAL --";
      break;
    case ADD_MARKER:
      mode_text = "";
      break;
    case EVAL_MARKER:
      mode_text = "";
      break;
    case FOLLOW:
      mode_text = "Follow hint: ";
      break;
    case PASS_THROUGH:
      mode_text = "-- PASS THROUGH --";
      break;
    case PASS_THROUGH_NEXT:
      mode_text = "-- PASS THROUGH (next)--";
      break;
    default:
      mode_text = "";
      mode      = NORMAL;
      break;
  }

  Jumanji.Global.mode = mode;
  notify(DEFAULT, mode_text);
}

void
close_tab(int tab_id)
{
  GtkWidget* tab = GTK_WIDGET(GET_NTH_TAB_WIDGET(tab_id));

  /* remove markers for this tab
   * and update the others */
  GList* list = Jumanji.Global.markers;
  while (list) {
    Marker* marker = (Marker*) list->data;
    GList* next_marker = g_list_next(list);

    if (marker->tab_id == tab_id) {
      Jumanji.Global.markers = g_list_delete_link(Jumanji.Global.markers, list);
      free(marker);
    } else if (marker->tab_id > tab_id) {
      marker->tab_id -= 1;
    }

    list = next_marker;
  }

  gchar *uri = g_strdup((gchar *) webkit_web_view_get_uri(GET_CURRENT_TAB()));
  Jumanji.Global.last_closed = g_list_prepend(Jumanji.Global.last_closed, uri);

  if (gtk_notebook_get_n_pages(Jumanji.UI.view) > 1) {
    gtk_container_remove(GTK_CONTAINER(Jumanji.UI.tabbar), GTK_WIDGET(g_object_get_data(G_OBJECT(tab), "tab")));
    gtk_notebook_remove_page(Jumanji.UI.view, tab_id);
    update_status();
  } else {
    cb_destroy(NULL, NULL);
  }
}

GtkWidget*
create_tab(char* uri, gboolean background)
{
  if(!uri)
    return NULL;

  GtkWidget *tab = gtk_scrolled_window_new(NULL, NULL);
  GtkWidget *wv  = webkit_web_view_new();

  if(!tab || !wv)
    return NULL;

  int number_of_tabs = gtk_notebook_get_current_page(Jumanji.UI.view);
  int position       = (next_to_current) ? (number_of_tabs + 1) : -1;

  if(show_scrollbars)
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tab), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  else
  {
    WebKitWebFrame* mf = webkit_web_view_get_main_frame(WEBKIT_WEB_VIEW(wv));
    g_signal_connect(G_OBJECT(mf),  "scrollbars-policy-changed", G_CALLBACK(cb_blank), NULL);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(tab), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  }

  GtkAdjustment* adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(tab));

  /* connect webview callbacks */
  g_signal_connect(G_OBJECT(wv),  "console-message",                      G_CALLBACK(cb_wv_console),                  NULL);
  g_signal_connect(G_OBJECT(wv),  "create-web-view",                      G_CALLBACK(cb_wv_create_web_view),          NULL);
  g_signal_connect(G_OBJECT(wv),  "download-requested",                   G_CALLBACK(cb_wv_download_request),         NULL);
  g_signal_connect(G_OBJECT(wv),  "button-release-event",                 G_CALLBACK(cb_wv_button_release_event),     NULL);
  g_signal_connect(G_OBJECT(wv),  "hovering-over-link",                   G_CALLBACK(cb_wv_hover_link),               NULL);
  g_signal_connect(G_OBJECT(wv),  "mime-type-policy-decision-requested",  G_CALLBACK(cb_wv_mimetype_policy_decision), NULL);
  g_signal_connect(G_OBJECT(wv),  "navigation-policy-decision-requested", G_CALLBACK(cb_wv_nav_policy_decision),      NULL);
  g_signal_connect(G_OBJECT(wv),  "new-window-policy-decision-requested", G_CALLBACK(cb_wv_window_policy_decision),   NULL);
  g_signal_connect(G_OBJECT(wv),  "notify::progress",                     G_CALLBACK(cb_wv_notify_progress),          NULL);
  g_signal_connect(G_OBJECT(wv),  "notify::title",                        G_CALLBACK(cb_wv_notify_title),             NULL);
  g_signal_connect(G_OBJECT(wv),  "window-object-cleared",                G_CALLBACK(cb_wv_window_object_cleared),    NULL);

  /* connect tab callbacks */
  g_signal_connect(G_OBJECT(tab),        "key-press-event", G_CALLBACK(cb_tab_kb_pressed), NULL);
  g_signal_connect(G_OBJECT(adjustment), "value-changed",   G_CALLBACK(cb_wv_scrolled),    NULL);

  /* set default values */
  g_object_set_data(G_OBJECT(wv), "loaded_scripts", 0);
  g_object_set(G_OBJECT(wv), "full-content-zoom", full_content_zoom, NULL);

  /* apply browser setting */
  webkit_web_view_set_settings(WEBKIT_WEB_VIEW(wv), webkit_web_settings_copy(Jumanji.Global.browser_settings));

  /* set web inspector */
  WebKitWebInspector* web_inspector = webkit_web_view_get_inspector(WEBKIT_WEB_VIEW(wv));
  g_signal_connect(G_OBJECT(web_inspector), "inspect-web-view", G_CALLBACK(cb_wv_inspector_view), NULL);

  gtk_container_add(GTK_CONTAINER(tab), wv);
  gtk_widget_show_all(tab);
  gtk_notebook_insert_page(Jumanji.UI.view, tab, NULL, position);

  if(!background)
    gtk_notebook_set_current_page(Jumanji.UI.view, position);

  /* create tab label */
  GtkWidget *tab_label = gtk_label_new(NULL);
  gtk_label_set_width_chars(GTK_LABEL(tab_label), 1.0);
  gtk_misc_set_alignment(    GTK_MISC(tab_label), 0.0, 0.0);
  gtk_misc_set_padding(      GTK_MISC(tab_label), 4.0, 4.0);

  /* create tab container */
  GtkWidget *tev_box = gtk_event_box_new();
  GtkWidget *tab_box = gtk_hbox_new(FALSE, 0);
  GtkWidget *tab_sep = gtk_vseparator_new();

  /* tab style */
  gtk_widget_modify_font(tab_label, Jumanji.Style.font);
  gtk_widget_modify_bg(tab_sep,  GTK_STATE_NORMAL, &(Jumanji.Style.tabbar_separator));

  /* build tab */
  gtk_box_pack_start(GTK_BOX(tab_box), tab_label,  TRUE,  TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tab_box), tab_sep,   FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(tev_box), tab_box);

  /* tab clickable */
  g_signal_connect(GTK_OBJECT(tev_box), "button_press_event", G_CALLBACK(cb_tab_clicked), tab);

  /* add to tabbar */
  gtk_box_pack_start(GTK_BOX(Jumanji.UI.tabbar), tev_box, TRUE, TRUE, 0);
  gtk_box_reorder_child(GTK_BOX(Jumanji.UI.tabbar), tev_box, position);
  gtk_widget_show_all(tev_box);

  /* add reference to tab */
  g_object_set_data(G_OBJECT(tab), "tab",   (gpointer) tev_box);
  g_object_set_data(G_OBJECT(tab), "label", (gpointer) tab_label);

  gtk_widget_grab_focus(GTK_WIDGET(GET_CURRENT_TAB_WIDGET()));

  /* open uri */
  open_uri(WEBKIT_WEB_VIEW(wv), uri);

  return wv;
}


void
eval_marker(int id)
{
  if((id < 0x30) || (id > 0x7A))
    return;

  GList* list;
  for(list = Jumanji.Global.markers; list; list = g_list_next(list))
  {
    Marker* marker = (Marker*) list->data;

    if(marker->id == id)
    {
      gtk_notebook_set_current_page(Jumanji.UI.view, marker->tab_id);
      GtkAdjustment* adjustment;
      adjustment = gtk_scrolled_window_get_vadjustment(GET_CURRENT_TAB_WIDGET());
      gtk_adjustment_set_value(adjustment, marker->vadjustment);
      adjustment = gtk_scrolled_window_get_hadjustment(GET_CURRENT_TAB_WIDGET());
      gtk_adjustment_set_value(adjustment, marker->hadjustment);
      webkit_web_view_set_zoom_level(GET_CURRENT_TAB(), marker->zoom_level);
      update_status();
      return;
    }
  }
}

void
init_data()
{
  /* read bookmarks */
  char* bookmark_file = g_build_filename(g_get_home_dir(), JUMANJI_DIR, JUMANJI_BOOKMARKS, NULL);

  if(!bookmark_file)
    return;

  if(g_file_test(bookmark_file, G_FILE_TEST_IS_REGULAR))
  {
    char* content = NULL;

    if(g_file_get_contents(bookmark_file, &content, NULL, NULL))
    {
      gchar **lines = g_strsplit(content, "\n", -1);
      int     n     = g_strv_length(lines) - 1;

      int i;
      for(i = 0; i < n; i++)
      {
        if(!strlen(lines[i]))
          continue;

        Jumanji.Global.bookmarks = g_list_append(Jumanji.Global.bookmarks, lines[i]);
      }

      g_free(content);
      g_free(lines);
    }
  }

  g_free(bookmark_file);

  /* read history */
  char* history_file = g_build_filename(g_get_home_dir(), JUMANJI_DIR, JUMANJI_HISTORY, NULL);

  if(!history_file)
    return;

  if(g_file_test(history_file, G_FILE_TEST_IS_REGULAR))
  {
    char* content = NULL;

    if(g_file_get_contents(history_file, &content, NULL, NULL))
    {
      gchar **lines = g_strsplit(content, "\n", -1);
      int     n     = g_strv_length(lines) - 1;

      int i;
      for(i = 0; i < n; i++)
      {
        if(!strlen(lines[i]))
          continue;

        Jumanji.Global.history = g_list_prepend(Jumanji.Global.history, lines[i]);
      }

      Jumanji.Global.history = g_list_reverse(Jumanji.Global.history);

      g_free(content);
      g_free(lines);
    }
  }

  g_free(history_file);

  /* read sessions */
  gchar* sessions_file = g_build_filename(g_get_home_dir(), JUMANJI_DIR, JUMANJI_SESSIONS, NULL);

  if(!sessions_file)
    return;

  if(g_file_test(sessions_file, G_FILE_TEST_IS_REGULAR))
  {
    char* content = NULL;

    if(g_file_get_contents(sessions_file, &content, NULL, NULL))
    {
      gchar **lines = g_strsplit(content, "\n", -1);
      int     n     = g_strv_length(lines) - 1;

      for(int i = 0; i < n; i+=2)
      {
        if(!strlen(lines[i]) || !strlen(lines[i+1]))
          continue;

        Session* se = malloc(sizeof(Session));
        se->name = lines[i];
        se->uris = lines[i+1];

        Jumanji.Global.sessions = g_list_prepend(Jumanji.Global.sessions, se);
      }

      g_free(content);
      g_free(lines);
    }
  }

  g_free(sessions_file);

  /* load cookies */
  char* cookie_file        = g_build_filename(g_get_home_dir(), JUMANJI_DIR, JUMANJI_COOKIES, NULL);
  SoupCookieJar *cookiejar = soup_cookie_jar_text_new(cookie_file, FALSE);

  soup_session_add_feature(Jumanji.Soup.session, (SoupSessionFeature*) cookiejar);
  g_free(cookie_file);
}

void
init_directories()
{
  /* create jumanji directory */
  gchar *base_directory = g_build_filename(g_get_home_dir(), JUMANJI_DIR, NULL);
  g_mkdir_with_parents(base_directory,  0771);
  g_free(base_directory);
}

void
init_keylist()
{
  /* init shortcuts */
  ShortcutList* e = NULL;
  ShortcutList* p = NULL;

  for(unsigned int i = 0; i < LENGTH(shortcuts); i++)
  {
    e = malloc(sizeof(ShortcutList));
    if(!e)
      out_of_memory();

    e->element = shortcuts[i];
    e->next    = NULL;

    if(!Jumanji.Bindings.sclist)
      Jumanji.Bindings.sclist = e;
    if(p)
      p->next = e;

    p = e;
  }

  /* init buffered commands */
  BufferCommandList *b = NULL;
  BufferCommandList *f = NULL;

  for(unsigned int i = 0; i < LENGTH(buffer_commands); i++)
  {
    b = malloc(sizeof(BufferCommandList));
    if(!b)
      out_of_memory();

    b->element = buffer_commands[i];
    b->next    = NULL;

    if(!Jumanji.Bindings.bcmdlist)
      Jumanji.Bindings.bcmdlist = b;
    if(f)
      f->next = b;

    f = b;
  }
}

void
init_settings()
{
  /* parse colors */
  gdk_color_parse(default_fgcolor,        &(Jumanji.Style.default_fg));
  gdk_color_parse(default_bgcolor,        &(Jumanji.Style.default_bg));
  gdk_color_parse(inputbar_fgcolor,       &(Jumanji.Style.inputbar_fg));
  gdk_color_parse(inputbar_bgcolor,       &(Jumanji.Style.inputbar_bg));
  gdk_color_parse(statusbar_fgcolor,      &(Jumanji.Style.statusbar_fg));
  gdk_color_parse(statusbar_bgcolor,      &(Jumanji.Style.statusbar_bg));
  gdk_color_parse(statusbar_ssl_fgcolor,  &(Jumanji.Style.statusbar_ssl_fg));
  gdk_color_parse(statusbar_ssl_bgcolor,  &(Jumanji.Style.statusbar_ssl_bg));
  gdk_color_parse(tabbar_fgcolor,         &(Jumanji.Style.tabbar_fg));
  gdk_color_parse(tabbar_bgcolor,         &(Jumanji.Style.tabbar_bg));
  gdk_color_parse(tabbar_focus_fgcolor,   &(Jumanji.Style.tabbar_focus_fg));
  gdk_color_parse(tabbar_focus_bgcolor,   &(Jumanji.Style.tabbar_focus_bg));
  gdk_color_parse(tabbar_separator_color, &(Jumanji.Style.tabbar_separator));
  gdk_color_parse(completion_fgcolor,     &(Jumanji.Style.completion_fg));
  gdk_color_parse(completion_bgcolor,     &(Jumanji.Style.completion_bg));
  gdk_color_parse(completion_g_fgcolor,   &(Jumanji.Style.completion_g_fg));
  gdk_color_parse(completion_g_bgcolor,   &(Jumanji.Style.completion_g_bg));
  gdk_color_parse(completion_hl_fgcolor,  &(Jumanji.Style.completion_hl_fg));
  gdk_color_parse(completion_hl_bgcolor,  &(Jumanji.Style.completion_hl_bg));
  gdk_color_parse(notification_e_fgcolor, &(Jumanji.Style.notification_e_fg));
  gdk_color_parse(notification_e_bgcolor, &(Jumanji.Style.notification_e_bg));
  gdk_color_parse(notification_w_fgcolor, &(Jumanji.Style.notification_w_fg));
  gdk_color_parse(notification_w_bgcolor, &(Jumanji.Style.notification_w_bg));
  Jumanji.Style.font = pango_font_description_from_string(font);

  /* statusbar */
  gtk_widget_modify_bg(GTK_WIDGET(Jumanji.UI.statusbar),       GTK_STATE_NORMAL, &(Jumanji.Style.statusbar_ssl_bg));
  gtk_widget_modify_fg(GTK_WIDGET(Jumanji.Statusbar.text),     GTK_STATE_NORMAL, &(Jumanji.Style.statusbar_ssl_fg));
  gtk_widget_modify_fg(GTK_WIDGET(Jumanji.Statusbar.buffer),   GTK_STATE_NORMAL, &(Jumanji.Style.statusbar_ssl_fg));
  gtk_widget_modify_fg(GTK_WIDGET(Jumanji.Statusbar.tabs),     GTK_STATE_NORMAL, &(Jumanji.Style.statusbar_ssl_fg));
  gtk_widget_modify_fg(GTK_WIDGET(Jumanji.Statusbar.position), GTK_STATE_NORMAL, &(Jumanji.Style.statusbar_ssl_fg));

  gtk_widget_modify_font(GTK_WIDGET(Jumanji.Statusbar.text),     Jumanji.Style.font);
  gtk_widget_modify_font(GTK_WIDGET(Jumanji.Statusbar.buffer),   Jumanji.Style.font);
  gtk_widget_modify_font(GTK_WIDGET(Jumanji.Statusbar.tabs),     Jumanji.Style.font);
  gtk_widget_modify_font(GTK_WIDGET(Jumanji.Statusbar.position), Jumanji.Style.font);

  /* inputbar */
  gtk_widget_modify_base(GTK_WIDGET(Jumanji.UI.inputbar), GTK_STATE_NORMAL, &(Jumanji.Style.inputbar_bg));
  gtk_widget_modify_text(GTK_WIDGET(Jumanji.UI.inputbar), GTK_STATE_NORMAL, &(Jumanji.Style.inputbar_fg));
  gtk_widget_modify_font(GTK_WIDGET(Jumanji.UI.inputbar),                     Jumanji.Style.font);

  /* set window size */
  gtk_window_set_default_size(GTK_WINDOW(Jumanji.UI.window), default_width, default_height);

  /* set proxy */
  sc_toggle_proxy(NULL);

  /* apply user agent */
  if(!user_agent && browser_name)
  {
    char* current_user_agent = NULL;
    g_object_get(G_OBJECT(Jumanji.Global.browser_settings), "user-agent", &current_user_agent, NULL);

    char* new_user_agent = g_strconcat(current_user_agent, " ", browser_name, NULL);
    g_object_set(G_OBJECT(Jumanji.Global.browser_settings), "user-agent", new_user_agent, NULL);
    g_free(new_user_agent);
  }
  else if(user_agent)
  {
    g_object_set(G_OBJECT(Jumanji.Global.browser_settings), "user-agent", user_agent, NULL);
  }
}

void
load_all_scripts()
{
  int ls = (size_t) g_object_get_data(G_OBJECT(GET_CURRENT_TAB()), "loaded_scripts");

  if(!ls)
  {
    ScriptList* sl = Jumanji.Global.scripts;
    while(sl)
    {
      run_script(sl->content, NULL, NULL);
      sl = sl->next;
    }
  }

  g_object_set_data(G_OBJECT(GET_CURRENT_TAB()), "loaded_scripts",  (gpointer) 1);
}

void init_ui()
{
  /* window */
  if(Jumanji.UI.embed)
    Jumanji.UI.window = gtk_plug_new(Jumanji.UI.embed);
  else
    Jumanji.UI.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /* UI */
  Jumanji.UI.box               = GTK_BOX(gtk_vbox_new(FALSE, 0));
  Jumanji.UI.statusbar         = gtk_event_box_new();
  Jumanji.UI.statusbar_entries = GTK_BOX(gtk_hbox_new(FALSE, 0));
  Jumanji.UI.tabbar            = gtk_hbox_new(TRUE, 0);
  Jumanji.UI.inputbar          = GTK_ENTRY(gtk_entry_new());
  Jumanji.UI.view              = GTK_NOTEBOOK(gtk_notebook_new());

  /* window */
  gtk_window_set_title(GTK_WINDOW(Jumanji.UI.window), "jumanji");
  GdkGeometry hints = { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  gtk_window_set_geometry_hints(GTK_WINDOW(Jumanji.UI.window), NULL, &hints, GDK_HINT_MIN_SIZE);
  g_signal_connect(G_OBJECT(Jumanji.UI.window), "destroy", G_CALLBACK(cb_destroy), NULL);

  /* box */
  gtk_box_set_spacing(Jumanji.UI.box, 0);
  gtk_container_add(GTK_CONTAINER(Jumanji.UI.window), GTK_WIDGET(Jumanji.UI.box));

  /* statusbar */
  Jumanji.Statusbar.text       = GTK_LABEL(gtk_label_new(NULL));
  Jumanji.Statusbar.buffer     = GTK_LABEL(gtk_label_new(NULL));
  Jumanji.Statusbar.tabs       = GTK_LABEL(gtk_label_new(NULL));
  Jumanji.Statusbar.position   = GTK_LABEL(gtk_label_new(NULL));

  gtk_misc_set_alignment(GTK_MISC(Jumanji.Statusbar.text),     0.0, 0.0);
  gtk_misc_set_alignment(GTK_MISC(Jumanji.Statusbar.buffer),   1.0, 0.0);
  gtk_misc_set_alignment(GTK_MISC(Jumanji.Statusbar.tabs),     1.0, 0.0);
  gtk_misc_set_alignment(GTK_MISC(Jumanji.Statusbar.position), 1.0, 0.0);

  gtk_misc_set_padding(GTK_MISC(Jumanji.Statusbar.text),     2.0, 4.0);
  gtk_misc_set_padding(GTK_MISC(Jumanji.Statusbar.buffer),   2.0, 4.0);
  gtk_misc_set_padding(GTK_MISC(Jumanji.Statusbar.tabs),     2.0, 4.0);
  gtk_misc_set_padding(GTK_MISC(Jumanji.Statusbar.position), 2.0, 4.0);

  gtk_box_pack_start(Jumanji.UI.statusbar_entries, GTK_WIDGET(Jumanji.Statusbar.text),     TRUE,   TRUE, 2);
  gtk_box_pack_start(Jumanji.UI.statusbar_entries, GTK_WIDGET(Jumanji.Statusbar.buffer),   FALSE, FALSE, 2);
  gtk_box_pack_start(Jumanji.UI.statusbar_entries, GTK_WIDGET(Jumanji.Statusbar.tabs),     FALSE, FALSE, 2);
  gtk_box_pack_start(Jumanji.UI.statusbar_entries, GTK_WIDGET(Jumanji.Statusbar.position), FALSE, FALSE, 2);

  gtk_container_add(GTK_CONTAINER(Jumanji.UI.statusbar), GTK_WIDGET(Jumanji.UI.statusbar_entries));

  /* inputbar */
  gtk_entry_set_inner_border(Jumanji.UI.inputbar, NULL);
  gtk_entry_set_has_frame(   Jumanji.UI.inputbar, FALSE);
  gtk_editable_set_editable( GTK_EDITABLE(Jumanji.UI.inputbar), TRUE);

  g_signal_connect(G_OBJECT(Jumanji.UI.inputbar), "key-press-event", G_CALLBACK(cb_inputbar_kb_pressed), NULL);
  g_signal_connect(GTK_EDITABLE(Jumanji.UI.inputbar), "changed",     G_CALLBACK(cb_inputbar_changed),    NULL);
  g_signal_connect(G_OBJECT(Jumanji.UI.inputbar), "activate",        G_CALLBACK(cb_inputbar_activate),   NULL);

  /* view */
  gtk_notebook_set_show_tabs(Jumanji.UI.view,   FALSE);
  gtk_notebook_set_show_border(Jumanji.UI.view, FALSE);

  /* packing */
  gtk_box_pack_start(Jumanji.UI.box, GTK_WIDGET(Jumanji.UI.tabbar),    FALSE, FALSE, 0);
  gtk_box_pack_start(Jumanji.UI.box, GTK_WIDGET(Jumanji.UI.view),       TRUE,  TRUE, 0);
  gtk_box_pack_start(Jumanji.UI.box, GTK_WIDGET(Jumanji.UI.statusbar), FALSE, FALSE, 0);
  gtk_box_pack_end(  Jumanji.UI.box, GTK_WIDGET(Jumanji.UI.inputbar),  FALSE, FALSE, 0);

  Jumanji.Global.init_ui = TRUE;
}

void notify(int level, char* message)
{
  if(!Jumanji.Global.init_ui)
  {
    if(message)
    {
      /* print error message to stdout while the ui has not been loaded */
      char* dmessage = g_strconcat("jumanjirc: ", message, NULL);
      printf("%s\n", dmessage);
      g_free(dmessage);
    }

    return;
  }

  if(!message || strlen(message) <= 0)
  {
    gtk_widget_hide(GTK_WIDGET(Jumanji.UI.inputbar));
    return;
  }

  if(!(GTK_WIDGET_VISIBLE(GTK_WIDGET(Jumanji.UI.inputbar))))
    gtk_widget_show(GTK_WIDGET(Jumanji.UI.inputbar));

  switch(level)
  {
    case ERROR:
      gtk_widget_modify_base(GTK_WIDGET(Jumanji.UI.inputbar), GTK_STATE_NORMAL, &(Jumanji.Style.notification_e_bg));
      gtk_widget_modify_text(GTK_WIDGET(Jumanji.UI.inputbar), GTK_STATE_NORMAL, &(Jumanji.Style.notification_e_fg));
      break;
    case WARNING:
      gtk_widget_modify_base(GTK_WIDGET(Jumanji.UI.inputbar), GTK_STATE_NORMAL, &(Jumanji.Style.notification_w_bg));
      gtk_widget_modify_text(GTK_WIDGET(Jumanji.UI.inputbar), GTK_STATE_NORMAL, &(Jumanji.Style.notification_w_fg));
      break;
    default:
      gtk_widget_modify_base(GTK_WIDGET(Jumanji.UI.inputbar), GTK_STATE_NORMAL, &(Jumanji.Style.inputbar_bg));
      gtk_widget_modify_text(GTK_WIDGET(Jumanji.UI.inputbar), GTK_STATE_NORMAL, &(Jumanji.Style.inputbar_fg));
      break;
  }

  if(message)
    gtk_entry_set_text(Jumanji.UI.inputbar, message);
}

void
init_jumanji()
{
  /* other */
  Jumanji.Global.mode                = NORMAL;
  Jumanji.Global.search_engines      = NULL;
  Jumanji.Global.command_history     = NULL;
  Jumanji.Global.scripts             = NULL;
  Jumanji.Global.markers             = NULL;
  Jumanji.Global.bookmarks           = NULL;
  Jumanji.Global.history             = NULL;
  Jumanji.Global.last_closed         = NULL;
  Jumanji.Global.init_ui             = FALSE;
  Jumanji.Bindings.sclist            = NULL;
  Jumanji.Bindings.bcmdlist          = NULL;

  /* webkit settings */
  Jumanji.Global.browser_settings = webkit_web_settings_new();

  /* GDK keymap */
  Jumanji.Global.keymap = gdk_keymap_get_default();

  /* libsoup session */
  Jumanji.Soup.session = webkit_get_default_session();
}

void
new_window(char* uri)
{
  if(!uri)
    return;

  char* nargv[6];
  if(Jumanji.UI.embed)
  {
    nargv[0] = *(Jumanji.Global.arguments);
    nargv[1] = "-e";
    nargv[2] = Jumanji.UI.winid;
    nargv[3] = uri;
    nargv[4] = NULL;
  }
  else
  {
    nargv[0] = *(Jumanji.Global.arguments);
    nargv[1] = uri;
    nargv[2] = NULL;
  }

  g_spawn_async(NULL, nargv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL);
}

void
out_of_memory()
{
  printf("error: out of memory\n");
  exit(-1);
}

void
open_uri(WebKitWebView* web_view, char* uri)
{
  if(!uri)
    return;

  while (*uri == ' ')
    uri++;

  gchar* new_uri = NULL;

  /* multiple argument given */
  char* uri_first_space = strchr(uri, ' ');
  if(uri_first_space)
  {
    unsigned int first_arg_length = uri_first_space - uri;

    SearchEngineList* se = Jumanji.Global.search_engines;
    while(se)
    {
      if(strlen(se->name) == first_arg_length && !strncmp(uri, se->name, first_arg_length))
        break;

      se = se->next;
    }
    /* first agrument contain "://"
     * -> it's a bookmark with tag
     */
    if(!se && strstr(uri, "://"))
    {
      new_uri = g_strndup(uri, first_arg_length);
    }
    /* first agrument doesn't contain "://"
     * -> use search engine
     */
    else
    {
      if(!se)
        se = Jumanji.Global.search_engines;
      else /* we remove the trailing arg since it's the se name */
        uri = uri + first_arg_length + 1;

      if(se)
      {
        /* there is at lease 1 search search engine */
        new_uri = g_strdup_printf(se->uri, uri);

        /* we change all the space with '+'
         * -2 for the '%s'
         */
        char* new_uri_it = new_uri + strlen(se->uri) - 2;

        while(*new_uri_it)
        {
          if(*new_uri_it == ' ')
            *new_uri_it = '+';

          new_uri_it++;
        }
      }
      else
      {
        /* there is 0 search engine (a very rare case...) */
        new_uri = g_strconcat("http://", uri, NULL);

        char* nu_first_space;

        /* we fill ' ' with '%20' */
        while ((nu_first_space = strchr(new_uri, ' ')))
        {
          /* we break new_uri at the first ' ' */
          *nu_first_space = '\0';
          char* nu_first_part = new_uri;
          char* nu_second_part = nu_first_space + 1;

          new_uri = g_strconcat(nu_first_part, "%20", nu_second_part, NULL);

          g_free(nu_first_part);
        }
      }
    }
  }
  /* no argument given */
  else if(strlen(uri) == 0)
  {
    new_uri = g_strdup(home_page);
  }
  /* only one argument given */
  else
  {
    /* uri reformating to get new_uri match
     * ^http://.*$
     * or
     * ^file://.*$
     */

    /* file path */
    if(uri[0] == '/' || strncmp(uri, "./", 2) == 0)
    {
      new_uri = g_strconcat("file://", uri, NULL);
    }
    /* uri does contain any ".", ":" or "/"
     * nor it start with "localhost"
     * -> default searchengine
     */
    else if(!strpbrk(uri, ".:/") && strncmp(uri, "localhost", 9))
    {
      if(Jumanji.Global.search_engines)
      {
        new_uri = g_strdup_printf(Jumanji.Global.search_engines->uri, uri);

        /* -2 for the '%s' */
        gchar* searchitem = new_uri + strlen(Jumanji.Global.search_engines->uri) - 2;

        while(*searchitem)
        {
          if(*searchitem == ' ')
            *searchitem = '+';

          searchitem += 1;
        }
      }
      else
        new_uri = g_strconcat("http://", uri, NULL);
    }
    else
      new_uri = strstr(uri, "://") ? g_strdup(uri) : g_strconcat("http://", uri, NULL);
  }

  webkit_web_view_load_uri(web_view, new_uri);

  /* update history */
  if(!private_browsing)
  {
    /* we verify if the new_uri is already present in the list*/
    GList* l = g_list_find_custom(Jumanji.Global.history, new_uri, (GCompareFunc)strcmp);
    if (l)
    {
      /* new_uri is already present : new move it to the end of the list */
      Jumanji.Global.history = g_list_remove_link(Jumanji.Global.history, l);
      Jumanji.Global.history = g_list_concat(l, Jumanji.Global.history);
    }
    else
    {
      Jumanji.Global.history = g_list_prepend(Jumanji.Global.history, g_strdup(new_uri));
    }
  }

  g_free(new_uri);

  update_status();
}

void
update_status()
{
  if(!Jumanji.UI.view || !gtk_notebook_get_n_pages(Jumanji.UI.view))
    return;

  update_uri();

  /* update title */
  const gchar* title = webkit_web_view_get_title(GET_CURRENT_TAB());
  if(title)
    gtk_window_set_title(GTK_WINDOW(Jumanji.UI.window), title);
  else
    gtk_window_set_title(GTK_WINDOW(Jumanji.UI.window), "jumanji");

  /* update tab position */
  int current_tab     = gtk_notebook_get_current_page(Jumanji.UI.view);
  int number_of_tabs  = gtk_notebook_get_n_pages(Jumanji.UI.view);

  gchar* tabs = g_strdup_printf("[%d/%d]", current_tab + 1, number_of_tabs);
  gtk_label_set_text((GtkLabel*) Jumanji.Statusbar.tabs, tabs);
  g_free(tabs);

  /* update tabbar */
  int tc = 0;
  for(tc = 0; tc < number_of_tabs; tc++)
  {
    GtkWidget* tab       = GTK_WIDGET(GET_NTH_TAB_WIDGET(tc));
    GtkWidget *tev_box   = GTK_WIDGET(g_object_get_data(G_OBJECT(tab), "tab"));
    GtkWidget *tab_label = GTK_WIDGET(g_object_get_data(G_OBJECT(tab), "label"));

    if(tc == current_tab)
    {
      gtk_widget_modify_bg(GTK_WIDGET(tev_box),   GTK_STATE_NORMAL, &(Jumanji.Style.tabbar_focus_bg));
      gtk_widget_modify_fg(GTK_WIDGET(tab_label), GTK_STATE_NORMAL, &(Jumanji.Style.tabbar_focus_fg));
    }
    else
    {
      gtk_widget_modify_bg(GTK_WIDGET(tev_box),   GTK_STATE_NORMAL, &(Jumanji.Style.tabbar_bg));
      gtk_widget_modify_fg(GTK_WIDGET(tab_label), GTK_STATE_NORMAL, &(Jumanji.Style.tabbar_fg));
    }

    const gchar* tab_title = webkit_web_view_get_title(GET_WEBVIEW(tab));
    int progress = webkit_web_view_get_progress(GET_WEBVIEW(tab)) * 100;
    gchar* n_tab_title = g_strdup_printf("%d | %s", tc + 1, tab_title ? tab_title : ((progress == 100) ? "Loading..." : "(Untitled)"));
    gtk_label_set_text((GtkLabel*) tab_label, n_tab_title);
    g_free(n_tab_title);
  }

  update_position();
}

void
update_uri()
{
  gchar* link  = (gchar*) webkit_web_view_get_uri(GET_CURRENT_TAB());
  int progress = webkit_web_view_get_progress(GET_CURRENT_TAB()) * 100;
  gchar* uri   = (progress != 100 && progress != 0) ? g_strdup_printf("Loading... %s (%d%%)", link ? link : "", progress) :
                 (link ? g_strdup(link) : NULL);

  /* check for https */
  GdkColor* fg;
  GdkColor* bg;

  gboolean ssl = link ? g_str_has_prefix(link, "https://") : FALSE;
  if(ssl)
  {
    fg = &(Jumanji.Style.statusbar_ssl_fg);
    bg = &(Jumanji.Style.statusbar_ssl_bg);
  }
  else
  {
    fg = &(Jumanji.Style.statusbar_fg);
    bg = &(Jumanji.Style.statusbar_bg);
  }

  gtk_widget_modify_bg(GTK_WIDGET(Jumanji.UI.statusbar),      GTK_STATE_NORMAL,  bg);
  gtk_widget_modify_fg(GTK_WIDGET(Jumanji.Statusbar.text),     GTK_STATE_NORMAL, fg);
  gtk_widget_modify_fg(GTK_WIDGET(Jumanji.Statusbar.buffer),   GTK_STATE_NORMAL, fg);
  gtk_widget_modify_fg(GTK_WIDGET(Jumanji.Statusbar.tabs),     GTK_STATE_NORMAL, fg);
  gtk_widget_modify_fg(GTK_WIDGET(Jumanji.Statusbar.position), GTK_STATE_NORMAL, fg);


  /* check for possible navigation */
  if(!uri)
    uri = g_strdup("[No name]");
  else
  {
    GString* navigation = g_string_new("");

    if(webkit_web_view_can_go_back(GET_CURRENT_TAB()))
      g_string_append_c(navigation, '+');
    if(webkit_web_view_can_go_forward(GET_CURRENT_TAB()))
      g_string_append_c(navigation, '-');

    if(navigation->len > 0)
    {
      char* new_uri = g_strconcat(uri, " [", navigation->str, "]", NULL);
      g_free(uri);
      uri = new_uri;
    }

    g_string_free(navigation, TRUE);
  }

  gtk_label_set_text((GtkLabel*) Jumanji.Statusbar.text, uri);
  g_free(uri);
}

void
update_position()
{
  if (gtk_notebook_get_current_page(Jumanji.UI.view) == -1)
    return;

  GtkAdjustment* adjustment = gtk_scrolled_window_get_vadjustment(GET_CURRENT_TAB_WIDGET());
  gdouble view_size         = gtk_adjustment_get_page_size(adjustment);
  gdouble value             = gtk_adjustment_get_value(adjustment);
  gdouble max               = gtk_adjustment_get_upper(adjustment) - view_size;

  gchar* position;
  if(max == 0)
    position = g_strdup("All");
  else if(value == max)
    position = g_strdup("Bot");
  else if(value == 0)
    position = g_strdup("Top");
  else
    position = g_strdup_printf("%2d%%", (int) ceil(((value / max) * 100)));

  gtk_label_set_text((GtkLabel*) Jumanji.Statusbar.position, position);
  g_free(position);
}

void
read_configuration()
{
  char* jumanjirc = g_build_filename(g_get_home_dir(), JUMANJI_DIR, JUMANJI_RC, NULL);

  if(!jumanjirc)
    return;

  if(g_file_test(jumanjirc, G_FILE_TEST_IS_REGULAR))
  {
    char* content = NULL;

    if(g_file_get_contents(jumanjirc, &content, NULL, NULL))
    {
      gchar **lines = g_strsplit(content, "\n", -1);
      int     n     = g_strv_length(lines) - 1;

      int i;
      for(i = 0; i <= n; i++)
      {
        if(!strlen(lines[i]))
          continue;

        gchar **pre_tokens = g_strsplit_set(lines[i], "\t ", -1);
        int     pre_length = g_strv_length(pre_tokens);

        gchar** tokens = g_malloc0(sizeof(gchar*) * (pre_length + 1));
        gchar** tokp =   tokens;
        int     length = 0;
        for (int i = 0; i != pre_length; ++i) {
          if (strlen(pre_tokens[i])) {
            *tokp++ = pre_tokens[i];
            ++length;
          }
        }

        if(!strcmp(tokens[0], "set"))
          cmd_set(length - 1, tokens + 1);
        else if(!strcmp(tokens[0], "map"))
          cmd_map(length - 1, tokens + 1);
        else if(!strcmp(tokens[0], "bmap"))
          cmd_bmap(length - 1, tokens + 1);
        else if(!strcmp(tokens[0], "searchengine"))
          cmd_search_engine(length - 1, tokens + 1);
        else if(!strcmp(tokens[0], "script"))
          cmd_script(length - 1, tokens + 1);

        g_free(tokens);
      }
    }
  }

  g_free(jumanjirc);
}

char*
read_file(const char* path)
{
  char* content = NULL;

  /* specify path max */
  size_t pm;
#ifdef PATH_MAX
  pm = PATH_MAX;
#else
  pm = pathconf(path,_PC_PATH_MAX);
  if(pm <= 0)
    pm = 4096;
#endif

  /* get filename */
  char* npath;

  if(path[0] == '~')
    npath = g_build_filename(g_get_home_dir(), path + 1, NULL);
  else
    npath = g_strdup(path);

  char* file = (char*) calloc(sizeof(char), pm);
  if(!file || !realpath(npath, file))
  {
    if(file)
      free(file);
    g_free(npath);
    return NULL;
  }

  g_free(npath);

  /* check if file exists */
  if(g_file_test(file, G_FILE_TEST_IS_REGULAR))
    if(g_file_get_contents(file, &content, NULL, NULL))
      return content;

  return NULL;
}

char*
reference_to_string(JSContextRef context, JSValueRef reference)
{
  if(!context || !reference)
    return NULL;

  JSStringRef ref_st = JSValueToStringCopy(context, reference, NULL);
  size_t      length = JSStringGetMaximumUTF8CStringSize(ref_st);
  gchar*      string = g_new(gchar, length);
  JSStringGetUTF8CString(ref_st, string, length);
  JSStringRelease(ref_st);

  return string;
}

void
run_script(char* script, char** value, char** error)
{
  if(!script)
    return;

  WebKitWebFrame *frame = webkit_web_view_get_main_frame(GET_CURRENT_TAB());

  if(!frame)
    return;

  JSContextRef context  = webkit_web_frame_get_global_context(frame);
  JSStringRef sc        = JSStringCreateWithUTF8CString(script);

  if(!context || !sc)
    return;

  JSObjectRef ob = JSContextGetGlobalObject(context);

  if(!ob)
    return;

  JSValueRef exception = NULL;
  JSValueRef va   = JSEvaluateScript(context, sc, ob, NULL, 0, &exception);
  JSStringRelease(sc);

  if(!va && error)
    *error = reference_to_string(context, exception);
  else if(value)
    *value = reference_to_string(context, va);
}

void
set_completion_row_color(GtkBox* results, int mode, int id)
{
  GtkEventBox *row   = (GtkEventBox*) g_list_nth_data(gtk_container_get_children(GTK_CONTAINER(results)), id);

  if(row)
  {
    GtkBox      *col   = (GtkBox*)      g_list_nth_data(gtk_container_get_children(GTK_CONTAINER(row)), 0);
    GtkLabel    *cmd   = (GtkLabel*)    g_list_nth_data(gtk_container_get_children(GTK_CONTAINER(col)), 0);
    GtkLabel    *cdesc = (GtkLabel*)    g_list_nth_data(gtk_container_get_children(GTK_CONTAINER(col)), 1);

    if(mode == NORMAL)
    {
      gtk_widget_modify_fg(GTK_WIDGET(cmd),   GTK_STATE_NORMAL, &(Jumanji.Style.completion_fg));
      gtk_widget_modify_fg(GTK_WIDGET(cdesc), GTK_STATE_NORMAL, &(Jumanji.Style.completion_fg));
      gtk_widget_modify_bg(GTK_WIDGET(row),   GTK_STATE_NORMAL, &(Jumanji.Style.completion_bg));
    }
    else
    {
      gtk_widget_modify_fg(GTK_WIDGET(cmd),   GTK_STATE_NORMAL, &(Jumanji.Style.completion_hl_fg));
      gtk_widget_modify_fg(GTK_WIDGET(cdesc), GTK_STATE_NORMAL, &(Jumanji.Style.completion_hl_fg));
      gtk_widget_modify_bg(GTK_WIDGET(row),   GTK_STATE_NORMAL, &(Jumanji.Style.completion_hl_bg));
    }
  }
}

void
switch_view(GtkWidget* UNUSED(widget))
{
  /*GtkWidget* child = gtk_bin_get_child(GTK_BIN(Jumanji.UI.viewport));*/
  /*if(child)*/
  /*{*/
    /*g_object_ref(child);*/
    /*gtk_container_remove(GTK_CONTAINER(Jumanji.UI.viewport), child);*/
  /*}*/

  /*gtk_container_add(GTK_CONTAINER(Jumanji.UI.viewport), GTK_WIDGET(widget));*/
}

GtkEventBox*
create_completion_row(GtkBox* results, char* command, char* description, gboolean group)
{
  GtkBox      *col = GTK_BOX(gtk_hbox_new(FALSE, 0));
  GtkEventBox *row = GTK_EVENT_BOX(gtk_event_box_new());

  GtkLabel *show_command     = GTK_LABEL(gtk_label_new(NULL));
  GtkLabel *show_description = GTK_LABEL(gtk_label_new(NULL));

  gtk_misc_set_alignment(GTK_MISC(show_command),     0.0, 0.0);
  gtk_misc_set_alignment(GTK_MISC(show_description), 0.0, 0.0);

  /*if(group)*/
  /*{*/
    /*gtk_misc_set_padding(GTK_MISC(show_command),     2.0, 4.0);*/
    /*gtk_misc_set_padding(GTK_MISC(show_description), 2.0, 4.0);*/
  /*}*/
  /*else*/
  /*{*/
    gtk_misc_set_padding(GTK_MISC(show_command),     1.0, 1.0);
    gtk_misc_set_padding(GTK_MISC(show_description), 1.0, 1.0);
  /*}*/

  gtk_label_set_use_markup(show_command,     TRUE);
  gtk_label_set_use_markup(show_description, TRUE);

  gtk_label_set_markup(show_command,     g_markup_printf_escaped(FORMAT_COMMAND,     command ? command : ""));
  gtk_label_set_markup(show_description, g_markup_printf_escaped(FORMAT_DESCRIPTION, description ? description : ""));

  if(group)
  {
    gtk_widget_modify_fg(GTK_WIDGET(show_command),     GTK_STATE_NORMAL, &(Jumanji.Style.completion_g_fg));
    gtk_widget_modify_fg(GTK_WIDGET(show_description), GTK_STATE_NORMAL, &(Jumanji.Style.completion_g_fg));
    gtk_widget_modify_bg(GTK_WIDGET(row),              GTK_STATE_NORMAL, &(Jumanji.Style.completion_g_bg));
  }
  else
  {
    gtk_widget_modify_fg(GTK_WIDGET(show_command),     GTK_STATE_NORMAL, &(Jumanji.Style.completion_fg));
    gtk_widget_modify_fg(GTK_WIDGET(show_description), GTK_STATE_NORMAL, &(Jumanji.Style.completion_fg));
    gtk_widget_modify_bg(GTK_WIDGET(row),              GTK_STATE_NORMAL, &(Jumanji.Style.completion_bg));
  }

  gtk_widget_modify_font(GTK_WIDGET(show_command),     Jumanji.Style.font);
  gtk_widget_modify_font(GTK_WIDGET(show_description), Jumanji.Style.font);

  gtk_box_pack_start(GTK_BOX(col), GTK_WIDGET(show_command),     TRUE,  TRUE,  2);
  gtk_box_pack_start(GTK_BOX(col), GTK_WIDGET(show_description), FALSE, FALSE, 2);

  gtk_container_add(GTK_CONTAINER(row), GTK_WIDGET(col));

  gtk_box_pack_start(results, GTK_WIDGET(row), FALSE, FALSE, 0);
  gtk_widget_show_all(GTK_WIDGET(row));

  return row;
}

Completion*
completion_init()
{
  Completion *completion = malloc(sizeof(Completion));
  if(!completion)
    out_of_memory();

  completion->groups = NULL;

  return completion;
}

CompletionGroup*
completion_group_create(char* name)
{
  CompletionGroup* group = malloc(sizeof(CompletionGroup));
  if(!group)
    out_of_memory();

  group->value    = name;
  group->elements = NULL;
  group->next     = NULL;

  return group;
}

void
completion_add_group(Completion* completion, CompletionGroup* group)
{
  CompletionGroup* cg = completion->groups;

  while(cg && cg->next)
    cg = cg->next;

  if(cg)
    cg->next = group;
  else
    completion->groups = group;
}

void completion_free(Completion* completion)
{
  CompletionGroup* group = completion->groups;
  CompletionElement *element;

  while(group)
  {
    element = group->elements;
    while(element)
    {
      CompletionElement* ne = element->next;
      free(element);
      element = ne;
    }

    CompletionGroup *ng = group->next;
    free(group);
    group = ng;
  }
}

void completion_group_add_element(CompletionGroup* group, char* name, char* description)
{
  CompletionElement* el = group->elements;

  while(el && el->next)
    el = el->next;

  CompletionElement* new_element = malloc(sizeof(CompletionElement));
  if(!new_element)
    out_of_memory();

  new_element->value       = name;
  new_element->description = description;
  new_element->next        = NULL;

  if(el)
    el->next = new_element;
  else
    group->elements = new_element;
}

/* shortcut implementation */
void
sc_abort(Argument* UNUSED(argument))
{
  /* Clear buffer */
  if(Jumanji.Global.buffer)
  {
    g_string_free(Jumanji.Global.buffer, TRUE);
    Jumanji.Global.buffer = NULL;
    gtk_label_set_text((GtkLabel*) Jumanji.Statusbar.buffer, "");
  }

  /* Clear hints */
  char* cmd = "clear()";
  run_script(cmd, NULL, NULL);

  /* Stop loading website */
  if(webkit_web_view_get_progress(GET_CURRENT_TAB()) == 1.0)
    cmd_stop(0, NULL);

  /* Set back to normal mode */
  change_mode(NORMAL);

  /* Hide inputbar */
  gtk_widget_hide(GTK_WIDGET(Jumanji.UI.inputbar));

  /* Unmark search results */
  webkit_web_view_unmark_text_matches(GET_CURRENT_TAB());

  gtk_widget_grab_focus(GTK_WIDGET(GET_CURRENT_TAB_WIDGET()));
}

void
sc_change_buffer(Argument* argument)
{
  if(!Jumanji.Global.buffer)
    return;

  int buffer_length = Jumanji.Global.buffer->len;

  if(argument->n == DELETE_LAST_CHAR)
  {
    if((buffer_length - 1) == 0)
    {
      g_string_free(Jumanji.Global.buffer, TRUE);
      Jumanji.Global.buffer = NULL;
      gtk_label_set_text((GtkLabel*) Jumanji.Statusbar.buffer, "");
    }
    else
    {
      GString* temp = g_string_new_len(Jumanji.Global.buffer->str, buffer_length - 1);
      g_string_free(Jumanji.Global.buffer, TRUE);
      Jumanji.Global.buffer = temp;
      gtk_label_set_text((GtkLabel*) Jumanji.Statusbar.buffer, Jumanji.Global.buffer->str);
    }

    if(Jumanji.Global.mode == FOLLOW)
    {
      Argument argument = {0, NULL};
      sc_follow_link(&argument);
    }
  }
}

void
sc_change_mode(Argument* argument)
{
  if(argument)
    change_mode(argument->n);
}

void
sc_close_tab(Argument* UNUSED(argument))
{
  int current_tab = gtk_notebook_get_current_page(Jumanji.UI.view);
  close_tab(current_tab);
}

void
sc_focus_inputbar(Argument* argument)
{
  if(argument->data)
  {
    char* data = argument->data;

    if(argument->n == APPEND_URL)
      data = g_strdup_printf("%s%s", data, webkit_web_view_get_uri(GET_CURRENT_TAB()));
    else
      data = g_strdup(data);

    notify(DEFAULT, data);
    g_free(data);

    /* we save the X clipboard that will be clear by "grab_focus" */
    gchar* x_clipboard_text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY));

    gtk_widget_grab_focus(GTK_WIDGET(Jumanji.UI.inputbar));
    gtk_editable_set_position(GTK_EDITABLE(Jumanji.UI.inputbar), -1);

    if (x_clipboard_text != NULL)
    {
      /* we reset the X clipboard with saved text */
      gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), x_clipboard_text, -1);

      g_free(x_clipboard_text);
    }
  }

  if(!(GTK_WIDGET_VISIBLE(GTK_WIDGET(Jumanji.UI.inputbar))))
    gtk_widget_show(GTK_WIDGET(Jumanji.UI.inputbar));
}

void
sc_follow_link(Argument* argument)
{
  static gboolean follow_links = FALSE;
  static int      open_mode    = -1;
  GdkEventKey *key = (GdkEventKey*)argument->data;

  /* update open mode */
  if(argument->n < 0)
    open_mode = argument->n;

  /* show all links */
  if(!follow_links || Jumanji.Global.mode != FOLLOW)
  {
    run_script("show_hints()", NULL, NULL);
    change_mode(FOLLOW);
    follow_links = TRUE;
    return;
  }

  char* value = NULL;
  char* cmd   = NULL;

  if (argument && argument->n == 10)
    cmd = g_strdup("get_active()");
  else if (key && key->keyval == GDK_Tab) {
    if ( key->state & GDK_CONTROL_MASK)
      cmd = g_strdup("focus_prev()");
    else
      cmd = g_strdup("focus_next()");
  }
  else if(Jumanji.Global.buffer && Jumanji.Global.buffer->len > 0)
    cmd = g_strconcat("update_hints(\"", Jumanji.Global.buffer->str, "\")", NULL);

  run_script(cmd, &value, NULL);
  g_free(cmd);

  if(value && strcmp(value, "undefined"))
  {
    if(open_mode == -1)
      open_uri(GET_CURRENT_TAB(), value);
    else
      create_tab(value, TRUE);

    sc_abort(NULL);
  }
}

void
sc_nav_history(Argument* argument)
{
  if(argument->n == NEXT)
    webkit_web_view_go_forward(GET_CURRENT_TAB());
  else if(argument->n == PREVIOUS)
    webkit_web_view_go_back(GET_CURRENT_TAB());
}

void
sc_nav_tabs(Argument* argument)
{
  bcmd_nav_tabs(NULL, argument);
}

void
sc_paste(Argument* argument)
{
  gchar* text = gtk_clipboard_wait_for_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY));

  if(argument->n == NEW_TAB)
    create_tab(text, FALSE);
  else
    open_uri(GET_CURRENT_TAB(), text);

  g_free(text);
}

void
sc_reload(Argument* argument)
{
  if(argument->n == BYPASS_CACHE)
    webkit_web_view_reload_bypass_cache(GET_CURRENT_TAB());
  else
    webkit_web_view_reload(GET_CURRENT_TAB());
}

void
sc_reopen(Argument* argument)
{
  GList *last_closed = g_list_first(Jumanji.Global.last_closed);

  if(last_closed)
  {
    if(argument && argument->n)
      create_tab(last_closed->data, TRUE);
    else
      create_tab(last_closed->data, FALSE);
    Jumanji.Global.last_closed = g_list_remove(Jumanji.Global.last_closed, last_closed->data);
  }
}

void
sc_run_script(Argument* argument)
{
  if(argument->data)
    run_script(argument->data, NULL, NULL);
}

void
sc_scroll(Argument* argument)
{
  GtkAdjustment* adjustment;

  if( (argument->n == LEFT) || (argument->n == RIGHT) || (argument->n == LEFT_MAX) || (argument->n == RIGHT_MAX) )
    adjustment = gtk_scrolled_window_get_hadjustment(GET_CURRENT_TAB_WIDGET());
  else
    adjustment = gtk_scrolled_window_get_vadjustment(GET_CURRENT_TAB_WIDGET());

  gdouble view_size  = gtk_adjustment_get_page_size(adjustment);
  gdouble value      = gtk_adjustment_get_value(adjustment);
  gdouble max        = gtk_adjustment_get_upper(adjustment) - view_size;

  if(argument->n == FULL_UP)
    gtk_adjustment_set_value(adjustment, (value - view_size) < 0 ? 0 : (value - view_size));
  else if(argument->n == FULL_DOWN)
    gtk_adjustment_set_value(adjustment, (value + view_size) > max ? max : (value + view_size));
  else if(argument->n == HALF_UP)
    gtk_adjustment_set_value(adjustment, (value - (view_size / 2)) < 0 ? 0 : (value - (view_size / 2)));
  else if(argument->n == HALF_DOWN)
    gtk_adjustment_set_value(adjustment, (value + (view_size / 2)) > max ? max : (value + (view_size / 2)));
  else if((argument->n == LEFT) || (argument->n == UP))
    gtk_adjustment_set_value(adjustment, (value - scroll_step) < 0 ? 0 : (value - scroll_step));
  else if(argument->n == TOP || argument->n == LEFT_MAX)
    gtk_adjustment_set_value(adjustment, 0);
  else if(argument->n == BOTTOM || argument->n == RIGHT_MAX)
    gtk_adjustment_set_value(adjustment, max);
  else
    gtk_adjustment_set_value(adjustment, (value + scroll_step) > max ? max : (value + scroll_step));
}

void
sc_search(Argument* argument)
{
  search_and_highlight(argument);
}

gboolean
sessionsave(char* session_name)
{
  GString* session_uris = g_string_new("");

  for (int i = 0; i < gtk_notebook_get_n_pages(Jumanji.UI.view); i++)
  {
    gchar* tab_uri_t = (gchar*) webkit_web_view_get_uri(GET_NTH_TAB(i));
    gchar* tab_uri   = g_strconcat(tab_uri_t, " ", NULL);
    session_uris     = g_string_append(session_uris, tab_uri);

    g_free(tab_uri);
  }

  GList* se_list = Jumanji.Global.sessions;
  while(se_list)
  {
    Session* se = se_list->data;

    if(g_strcmp0(se->name, session_name) == 0)
    {
      g_free(se->uris);
      se->uris = session_uris->str;

      break;
    }

    se_list = g_list_next(se_list);
  }

  /* if there was no session with name == session_name */
  if(!se_list)
  {
    Session* se = malloc(sizeof(Session));
    se->name = g_strdup(session_name);
    se->uris = session_uris->str;

    Jumanji.Global.sessions = g_list_prepend(Jumanji.Global.sessions, se);
  }

  /* we don't free session_uris->str , just session_uris */
  g_string_free(session_uris, FALSE);

  return TRUE;
}

gboolean
sessionswitch(char* session_name)
{
  // search for session
  gchar** se_uris = NULL;

  GList* se_list = Jumanji.Global.sessions;
  while(se_list)
  {
    Session* se = se_list->data;

    if(g_strcmp0(se->name, session_name) == 0)
    {
      se_uris = g_strsplit(se->uris, " ", -1);
      break ;
    }

    se_list = g_list_next(se_list);
  }

  if(!se_uris)
    return FALSE;

  int nb_uris = g_strv_length(se_uris) - 1;

  if(nb_uris <= 0)
    return FALSE;

  /* a session have been found
   * we can start removing all the current tabs
   * and attached informations */

  // remove all markers
  GList* list = Jumanji.Global.markers;
  while(list)
  {
    GList* next_marker = g_list_next(list);

    Marker* marker = (Marker*) list->data;
    Jumanji.Global.markers = g_list_delete_link(Jumanji.Global.markers, list);
    free(marker);

    list = next_marker;
  }

  /* remove all the tabs
   * without updating the status bar */
  for (int i = gtk_notebook_get_n_pages(Jumanji.UI.view) - 1; i != -1; --i)
  {
    GtkWidget* tab = GTK_WIDGET(GET_NTH_TAB_WIDGET(i));

    gtk_container_remove(GTK_CONTAINER(Jumanji.UI.tabbar), GTK_WIDGET(g_object_get_data(G_OBJECT(tab), "tab")));

    gtk_notebook_remove_page(Jumanji.UI.view, i);
  }

  // load the session
  for(int i = 0; i < nb_uris; i++)
    create_tab(se_uris[i], TRUE);

  g_strfreev(se_uris);
  return TRUE;
}

gboolean
sessionload(char* session_name)
{
  GList* se_list = Jumanji.Global.sessions;
  while(se_list)
  {
    Session* se = se_list->data;

    if(g_strcmp0(se->name, session_name) == 0)
    {
      gchar** uris = g_strsplit(se->uris, " ", -1);
      int     n    = g_strv_length(uris) - 1;

      if(n <= 0)
        return FALSE;

      for(int i = 0; i < n; i++)
        create_tab(uris[i], TRUE);

      g_strfreev(uris);
      return TRUE;
    }

    se_list = g_list_next(se_list);
  }

  return FALSE;
}

void
sc_spawn(Argument* argument)
{
  bcmd_spawn(NULL, argument);
}

void
sc_toggle_proxy(Argument* UNUSED(argument))
{
  static gboolean enable = FALSE;

  if(enable)
  {
    g_object_set(Jumanji.Soup.session, "proxy-uri", NULL, NULL);

    if(Jumanji.Global.init_ui)
      notify(DEFAULT, "Proxy deactivated");
  }
  else
  {
    char* purl = (proxy) ? proxy : (char*) g_getenv("http_proxy");
    if(!purl)
    {
      if(Jumanji.Global.init_ui)
        notify(DEFAULT, "No proxy defined");
      return;
    }

    char* uri = strstr(purl, "://") ? g_strdup(purl) : g_strconcat("http://", purl, NULL);
    SoupURI* proxy_uri = soup_uri_new(uri);

    g_object_set(Jumanji.Soup.session, "proxy-uri", proxy_uri, NULL);

    soup_uri_free(proxy_uri);
    g_free(uri);

    if(Jumanji.Global.init_ui)
      notify(DEFAULT, "Proxy activated");
  }

  enable = !enable;
}

void
sc_toggle_statusbar(Argument* UNUSED(argument))
{
  if(GTK_WIDGET_VISIBLE(GTK_WIDGET(Jumanji.UI.statusbar)))
    gtk_widget_hide(GTK_WIDGET(Jumanji.UI.statusbar));
  else
    gtk_widget_show(GTK_WIDGET(Jumanji.UI.statusbar));
}

void
sc_toggle_sourcecode(Argument* UNUSED(argument))
{
  gchar* uri = (char*) webkit_web_view_get_uri(GET_CURRENT_TAB());

  if(webkit_web_view_get_view_source_mode(GET_CURRENT_TAB()))
    webkit_web_view_set_view_source_mode(GET_CURRENT_TAB(), FALSE);
  else
    webkit_web_view_set_view_source_mode(GET_CURRENT_TAB(), TRUE);

  open_uri(GET_CURRENT_TAB(), uri);
}

void
sc_toggle_tabbar(Argument* UNUSED(argument))
{
  if(GTK_WIDGET_VISIBLE(GTK_WIDGET(Jumanji.UI.tabbar)))
    gtk_widget_hide(GTK_WIDGET(Jumanji.UI.tabbar));
  else
    gtk_widget_show(GTK_WIDGET(Jumanji.UI.tabbar));
}

void
sc_quit(Argument* UNUSED(argument))
{
  cb_destroy(NULL, NULL);
}

void
sc_yank(Argument* argument)
{
  gchar* uri = (gchar*) webkit_web_view_get_uri(GET_CURRENT_TAB());
  if (argument->n == XA_CLIPBOARD)
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), uri, -1);
  else if (argument->n == XA_SECONDARY)
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_SECONDARY), uri, -1);
  else
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), uri, -1);

  gchar* message = g_strdup_printf("Yanked %s", uri);
  notify(DEFAULT, message);
  g_free(message);
}

void
sc_zoom(Argument* argument)
{
  bcmd_zoom(NULL, argument);
}

/* inputbar shortcut declarations */
void
isc_abort(Argument* UNUSED(argument))
{
  Argument arg = { HIDE, NULL };
  isc_completion(&arg);

  notify(DEFAULT, "");
  gtk_widget_grab_focus(GTK_WIDGET(GET_CURRENT_TAB_WIDGET()));
  gtk_widget_hide(GTK_WIDGET(Jumanji.UI.inputbar));
}

void
isc_completion(Argument* argument)
{
  gchar *input      = gtk_editable_get_chars(GTK_EDITABLE(Jumanji.UI.inputbar), 0, -1);
  gchar  identifier = input[0];
  gchar *input_m    = input + 1;
  int    length     = strlen(input_m);

  if(!length && !identifier)
  {
    g_free(input);
    return;
  }

  /* get current information*/
  char* first_space = strstr(input_m, " ");
  char* current_command;
  char* current_parameter;
  int   current_command_length;

  if(!first_space)
  {
    current_command          = input_m;
    current_command_length   = length;
    current_parameter        = NULL;
  }
  else
  {
    int offset               = abs(input_m - first_space);
    current_command          = g_strndup(input_m, offset);
    current_command_length   = strlen(current_command);
    current_parameter        = input_m + offset + 1;
  }

  /* if the identifier does not match the command sign and
   * the completion should not be hidden, leave this function */
  if((identifier != ':') && (argument->n != HIDE))
  {
    g_free(input);
    return;
  }

  /* static elements */
  static GtkBox        *results = NULL;
  static CompletionRow *rows    = NULL;

  static int current_item = 0;
  static int n_items      = 0;

  static char *previous_command   = NULL;
  static char *previous_parameter = NULL;
  static int   previous_id        = 0;
  static int   previous_length    = 0;

  static gboolean command_mode = TRUE;

  /* delete old list iff
   *   the completion should be hidden
   *   the current command differs from the previous one
   *   the current parameter differs from the previous one
   */
  if( (argument->n == HIDE) ||
      (current_parameter && previous_parameter && strcmp(current_parameter, previous_parameter)) ||
      (current_command && previous_command && strcmp(current_command, previous_command)) ||
      (previous_length != length)
    )
  {
    if(results)
      gtk_widget_destroy(GTK_WIDGET(results));

    results = NULL;

    if(rows)
      free(rows);

    rows         = NULL;
    current_item = 0;
    n_items      = 0;
    command_mode = TRUE;

    if(argument->n == HIDE)
    {
      g_free(input);
      return;
    }
  }

  /* create new list iff
   *  there is no current list
   *  the current command differs from the previous one
   *  the current parameter differs from the previous one
   */
  if( (!results) )
  {
    results = GTK_BOX(gtk_vbox_new(FALSE, 0));

    /* create list based on parameters iff
     *  there is a current parameter given
     *  there is an old list with commands (or not)
     *  the current command does not differ from the previous one
     *  the current command has an completion function
     */
    if(strchr(input_m, ' '))
    {
      gboolean search_matching_command = FALSE;

      for(unsigned int i = 0; i < LENGTH(commands); i++)
      {
        int abbr_length = commands[i].abbr ? strlen(commands[i].abbr) : 0;
        int cmd_length  = commands[i].command ? strlen(commands[i].command) : 0;

        if( ((current_command_length <= cmd_length)  && !strncmp(current_command, commands[i].command, current_command_length)) ||
            ((current_command_length <= abbr_length) && !strncmp(current_command, commands[i].abbr,    current_command_length))
          )
        {
          if(commands[i].completion)
          {
            previous_command = current_command;
            previous_id = i;
            search_matching_command = TRUE;
          }
          else
          {
            g_free(input);
            return;
          }
        }
      }

      if(!search_matching_command)
      {
        g_free(input);
        return;
      }

      Completion *result = commands[previous_id].completion(current_parameter ? current_parameter : "");

      if(!result || !result->groups)
      {
        g_free(input);
        return;
      }

      command_mode               = FALSE;
      CompletionGroup* group     = NULL;
      CompletionElement* element = NULL;

      rows = malloc(sizeof(CompletionRow));
      if(!rows)
        out_of_memory();

      for(group = result->groups; group != NULL; group = group->next)
      {
        int group_elements = 0;

        for(element = group->elements; element != NULL; element = element->next)
        {
          if(element->value)
          {
            if (group->value && !group_elements)
            {
              rows = realloc(rows, (n_items + 1) * sizeof(CompletionRow));
              rows[n_items].command     = group->value;
              rows[n_items].description = NULL;
              rows[n_items].command_id  = -1;
              rows[n_items].is_group    = TRUE;
              rows[n_items++].row       = GTK_WIDGET(create_completion_row(results, group->value, NULL, TRUE));
            }

            rows = realloc(rows, (n_items + 1) * sizeof(CompletionRow));
            rows[n_items].command     = element->value;
            rows[n_items].description = element->description;
            rows[n_items].command_id  = previous_id;
            rows[n_items].is_group    = FALSE;
            rows[n_items++].row       = GTK_WIDGET(create_completion_row(results, element->value, element->description, FALSE));
            group_elements++;
          }
        }
      }

      /* clean up */
      completion_free(result);
    }
    /* create list based on commands */
    else
    {
      command_mode = TRUE;

      rows = malloc(LENGTH(commands) * sizeof(CompletionRow));
      if(!rows)
        out_of_memory();

      for(unsigned int i = 0; i < LENGTH(commands); i++)
      {
        int abbr_length = commands[i].abbr ? strlen(commands[i].abbr) : 0;
        int cmd_length  = commands[i].command ? strlen(commands[i].command) : 0;

        /* add command to list iff
         *  the current command would match the command
         *  the current command would match the abbreviation
         */
        if( ((current_command_length <= cmd_length)  && !strncmp(current_command, commands[i].command, current_command_length)) ||
            ((current_command_length <= abbr_length) && !strncmp(current_command, commands[i].abbr,    current_command_length))
          )
        {
          rows[n_items].command     = commands[i].command;
          rows[n_items].description = commands[i].description;
          rows[n_items].command_id  = i;
          rows[n_items].is_group    = FALSE;
          rows[n_items++].row       = GTK_WIDGET(create_completion_row(results, commands[i].command, commands[i].description, FALSE));
        }
      }

      rows = realloc(rows, n_items * sizeof(CompletionRow));
    }

    gtk_box_pack_start(Jumanji.UI.box, GTK_WIDGET(results), FALSE, FALSE, 0);
    gtk_widget_show(GTK_WIDGET(results));

    current_item = (argument->n == NEXT) ? -1 : 0;
  }

  /* update coloring iff
   *  there is a list with items
   */
  if( (results) && (n_items > 0) )
  {
    set_completion_row_color(results, NORMAL, current_item);
    char* temp;
    int i = 0, next_group = 0;

    for(i = 0; i < n_items; i++)
    {
      if(argument->n == NEXT || argument->n == NEXT_GROUP)
        current_item = (current_item + n_items + 1) % n_items;
      else if(argument->n == PREVIOUS || argument->n == PREVIOUS_GROUP)
        current_item = (current_item + n_items - 1) % n_items;

      if(rows[current_item].is_group)
      {
        if(!command_mode && (argument->n == NEXT_GROUP || argument->n == PREVIOUS_GROUP))
          next_group = 1;
        continue;
      }
      else
      {
        if(!command_mode && (next_group == 0) && (argument->n == NEXT_GROUP || argument->n == PREVIOUS_GROUP))
          continue;
        break;
      }
    }

    set_completion_row_color(results, HIGHLIGHT, current_item);

    /* hide other items */
    int uh = ceil(n_completion_items / 2);
    int lh = floor(n_completion_items / 2);

    for(i = 0; i < n_items; i++)
    {
     if((n_items > 1) && (
        (i >= (current_item - lh) && (i <= current_item + uh)) ||
        (i < n_completion_items && current_item < lh) ||
        (i >= (n_items - n_completion_items) && (current_item >= (n_items - uh))))
       )
        gtk_widget_show(rows[i].row);
      else
        gtk_widget_hide(rows[i].row);
    }

    if(command_mode)
      temp = g_strconcat(":", rows[current_item].command, (n_items == 1) ? " "  : NULL, NULL);
    else
      temp = g_strconcat(":", previous_command, " ", rows[current_item].command, NULL);

    gtk_entry_set_text(Jumanji.UI.inputbar, temp);
    gtk_editable_set_position(GTK_EDITABLE(Jumanji.UI.inputbar), -1);
    g_free(temp);

    previous_command   = (command_mode) ? rows[current_item].command : current_command;
    previous_parameter = (command_mode) ? current_parameter : rows[current_item].command;
    previous_length    = strlen(previous_command);
    if(command_mode)
      previous_length += length - current_command_length;
    else
      previous_length += strlen(previous_parameter) + 1;

    previous_id        = rows[current_item].command_id;
  }

  g_free(input);
}

void
isc_command_history(Argument* argument)
{
  static int current = 0;
  int        length  = g_list_length(Jumanji.Global.command_history);

  if(length > 0)
  {
    if(argument->n == NEXT)
      current = (length + current + 1) % length;
    else
      current = (length + current - 1) % length;

    gchar* command = (gchar*) g_list_nth_data(Jumanji.Global.command_history, current);
    notify(DEFAULT, command);
    gtk_editable_set_position(GTK_EDITABLE(Jumanji.UI.inputbar), -1);
  }
}

void
isc_string_manipulation(Argument* argument)
{
  gchar *input  = gtk_editable_get_chars(GTK_EDITABLE(Jumanji.UI.inputbar), 0, -1);
  int    length = strlen(input);
  int pos       = gtk_editable_get_position(GTK_EDITABLE(Jumanji.UI.inputbar));

  if(argument->n == DELETE_LAST_WORD)
  {
    int i = pos - 1;

    if(!pos)
    {
      g_free(input);
      return;
    }

    /* remove trailing spaces */
    for(; i >= 0 && input[i] == ' '; i--);

    /* find the beginning of the word */
    while((i == (pos - 1)) || (((i) > 0) && (input[i] != ' ')
          && (input[i] != '/') && (input[i] != '.')
          && (input[i] != '-') && (input[i] != '=')
          && (input[i] != '&') && (input[i] != '#')
          && (input[i] != '?')
          ))
      i--;

    gtk_editable_delete_text(GTK_EDITABLE(Jumanji.UI.inputbar),  i+1, pos);
    gtk_editable_set_position(GTK_EDITABLE(Jumanji.UI.inputbar), i+1);
  }
  else if(argument->n == DELETE_LAST_CHAR)
  {
    if((length - 1) <= 0)
      isc_abort(NULL);

    gtk_editable_delete_text(GTK_EDITABLE(Jumanji.UI.inputbar), pos - 1, pos);
  }
  else if(argument->n == NEXT_CHAR)
    gtk_editable_set_position(GTK_EDITABLE(Jumanji.UI.inputbar), pos+1);
  else if(argument->n == PREVIOUS_CHAR)
    gtk_editable_set_position(GTK_EDITABLE(Jumanji.UI.inputbar), (pos == 0) ? 0 : pos - 1);

  g_free(input);
}

/* command implementation */
gboolean
cmd_back(int UNUSED(argc), char** UNUSED(argv))
{
  Argument argument;
  argument.n = PREVIOUS;
  sc_nav_history(&argument);

  return TRUE;
}

gboolean
cmd_bmap(int argc, char** argv)
{
  if(argc < 2)
    return TRUE;

  /* search for the right buffered command function mapping */
  int bc_id = -1;

  for(unsigned int bc_c = 0; bc_c < LENGTH(function_names); bc_c++)
  {
    if(!strcmp(argv[1], function_names[bc_c].name) && function_names[bc_c].bcmd)
    {
      bc_id = bc_c;
      break;
    }
  }

  if(bc_id == -1)
  {
    notify(WARNING, "No such buffered command function exists");
    return FALSE;
  }

  /* parse argument */
  Argument arg = {0, 0};

  if(argc >= 3)
  {
    int arg_id = -1;

    /* compare argument with given argument names... */
    for(unsigned int arg_c = 0; arg_c < LENGTH(argument_names); arg_c++)
    {
      if(!strcmp(argv[2], argument_names[arg_c].name))
      {
        arg_id = argument_names[arg_c].argument;
        break;
      }
    }

    /* if not, save it do .data */
    if(arg_id == -1)
      arg.data = argv[2];
    else
      arg.n = arg_id;
  }

  /* search for existing buffered command to overwrite it */
  BufferCommandList* bc = Jumanji.Bindings.bcmdlist;
  while(bc && bc->next != NULL)
  {
    if(!strcmp(bc->element.regex, argv[0]))
    {
      bc->element.function = function_names[bc_id].bcmd;
      bc->element.argument = arg;
      return TRUE;
    }

    bc = bc->next;
  }

  /* create new entry */
  BufferCommandList* entry = malloc(sizeof(BufferCommandList));
  if(!entry)
    out_of_memory();

  entry->element.regex    = argv[0];
  entry->element.function = function_names[bc_id].bcmd;
  entry->element.argument = arg;
  entry->next             = NULL;

  /* append to list */
  if(!Jumanji.Bindings.bcmdlist)
    Jumanji.Bindings.bcmdlist = entry;

  if(bc)
    bc->next = entry;

  return TRUE;
}

gboolean
cmd_bookmark(int argc, char** argv)
{
  char* bookmark = g_strdup(webkit_web_view_get_uri(GET_CURRENT_TAB()));

  /* at first we verify that bookmark (without tag) isn't already in the list */
  unsigned int bookmark_length = strlen(bookmark);
  for(GList* l = Jumanji.Global.bookmarks; l; l = g_list_next(l))
  {
    if(!strncmp(bookmark, (char*) l->data, bookmark_length))
    {
      /* we remove the former bookmark so tags will be updated by the new ones */
      g_free(l->data);
      Jumanji.Global.bookmarks = g_list_delete_link(Jumanji.Global.bookmarks, l);
      break;
    }
  }

  /* I am sure argv end with NULL since it was generate by g_strsplit()
   * in cb_inputbar_activate.
   * Even if I know (argv[argc] == NULL) I test it since apply g_strjoinv
   * to a non NULL terminated array would be VERY problematic.
   * So I protect cmd_bookmark from bad cb_inputbar_activate modification.
   */

  /* if there is tags we add it to the bookmark string
   * -> append it to the bookmark
   */
  if(argc >= 1 && argv[argc] == NULL)
  {
    char* tags = g_strjoinv(" ", argv);
    char* bookmark_temp = bookmark;
    bookmark = g_strjoin(" ", bookmark, tags, NULL);

    g_free(bookmark_temp);
    g_free(tags);
  }

  Jumanji.Global.bookmarks = g_list_append(Jumanji.Global.bookmarks, bookmark);

  return TRUE;
}

gboolean
cmd_forward(int UNUSED(argc), char** UNUSED(argv))
{
  Argument argument;
  argument.n = NEXT;
  sc_nav_history(&argument);

  return TRUE;
}

gboolean
cmd_map(int argc, char** argv)
{
  if(argc < 2)
    return TRUE;

  char* ks = argv[0];

  /* search for the right shortcut function */
  int sc_id = -1;

  for(unsigned int sc_c = 0; sc_c < LENGTH(function_names); sc_c++)
  {
    if(!strcmp(argv[1], function_names[sc_c].name) && function_names[sc_c].sc)
    {
      sc_id = sc_c;
      break;
    }
  }

  if(sc_id == -1)
  {
    notify(WARNING, "No such shortcut function exists");
    return FALSE;
  }

  /* parse modifier and key */
  unsigned int mask = 0;
  unsigned int key  = 0;
  int keyl = strlen(ks);
  int mode = NORMAL;

  // single key (e.g.: g)
  if(keyl == 1)
    key = ks[0];

  // modifier and key (e.g.: <S-g>
  // special key or modifier and key/special key (e.g.: <S-g>, <Space>)

  else if(keyl >= 3 && ks[0] == '<' && ks[keyl-1] == '>')
  {
    char* specialkey = NULL;

    /* check for modifier */
    if(keyl >= 5 && ks[2] == '-')
    {
      /* evaluate modifier */
      switch(ks[1])
      {
        case 'S':
          mask = GDK_SHIFT_MASK;
          break;
        case 'C':
          mask = GDK_CONTROL_MASK;
          break;
        case 'W':
          mask = GDK_SUPER_MASK | GDK_MOD4_MASK;
          break;
        case 'A':
          mask = GDK_MOD1_MASK;
          break;
      }

      /* no valid modifier */
      if(!mask)
      {
        notify(WARNING, "No valid modifier given.");
        return FALSE;
      }

      /* modifier and special key */
      if(keyl > 5)
        specialkey = g_strndup(ks + 3, keyl - 4);
      else
        key = ks[3];
    }
    else
      specialkey = ks;

    /* search special key */
    for(unsigned int g_c = 0; specialkey && g_c < LENGTH(gdk_keys); g_c++)
    {
      if(!strcmp(specialkey, gdk_keys[g_c].identifier))
      {
        key = gdk_keys[g_c].key;
        break;
      }
    }

    if(specialkey)
      g_free(specialkey);
  }

  if(!key)
  {
    notify(WARNING, "No valid key binding given.");
    return FALSE;
  }

  /* parse argument */
  Argument arg = {0, 0};

  if(argc >= 3)
  {
    int arg_id = -1;

    /* compare argument with given argument names... */
    for(unsigned int arg_c = 0; arg_c < LENGTH(argument_names); arg_c++)
    {
      if(!strcmp(argv[2], argument_names[arg_c].name))
      {
        arg_id = argument_names[arg_c].argument;
        break;
      }
    }

    /* if not, save it do .data */
    if(arg_id == -1)
      arg.data = argv[2];
    else
      arg.n = arg_id;
  }

  /* parse mode */
  if(argc >= 4)
  {
    for(unsigned int mode_c = 0; mode_c < LENGTH(mode_names); mode_c++)
    {
      if(!strcmp(argv[3], mode_names[mode_c].name))
      {
        mode = mode_names[mode_c].mode;
        break;
      }
    }
  }

  /* search for existing binding to overwrite it */
  ShortcutList* sc = Jumanji.Bindings.sclist;
  while(sc && sc->next != NULL)
  {
    if(
       sc->element.key == key
       && sc->element.mask == mask
       && sc->element.mode == mode
      )
    {
      sc->element.function = function_names[sc_id].sc;
      sc->element.argument = arg;
      return TRUE;
    }

    sc = sc->next;
  }

  /* create new entry */
  ShortcutList* entry = malloc(sizeof(ShortcutList));
  if(!entry)
    out_of_memory();

  entry->element.mask     = mask;
  entry->element.key      = key;
  entry->element.function = function_names[sc_id].sc;
  entry->element.mode     = mode;
  entry->element.argument = arg;
  entry->next             = NULL;

  /* append to list */
  if(!Jumanji.Bindings.sclist)
    Jumanji.Bindings.sclist = entry;

  if(sc)
    sc->next = entry;

  return TRUE;
}

gboolean
cmd_open(int argc, char** argv)
{
  if(argc <= 0)
    return TRUE;

  if(argv[argc] != NULL)
    return TRUE;

  char* uri = g_strjoinv(" ", argv);

  open_uri(GET_CURRENT_TAB(), uri);
  g_free(uri);

  return TRUE;
}

gboolean
cmd_print(int UNUSED(argc), char** UNUSED(argv))
{
  WebKitWebFrame* frame = webkit_web_view_get_main_frame(GET_CURRENT_TAB());

  if(!frame)
    return FALSE;

  webkit_web_frame_print(frame);

  return TRUE;
}

gboolean
cmd_quit(int UNUSED(argc), char** UNUSED(argv))
{
  sc_close_tab(NULL);
  return TRUE;
}

gboolean
cmd_quitall(int UNUSED(argc), char** UNUSED(argv))
{
  cb_destroy(NULL, NULL);
  return TRUE;
}

gboolean
cmd_reload(int UNUSED(argc), char** UNUSED(argv))
{
  Argument argument = {0, 0};
  sc_reload(&argument);

  return TRUE;
}

gboolean
cmd_reload_all(int UNUSED(argc), char** UNUSED(argv))
{
  int number_of_tabs = gtk_notebook_get_n_pages(Jumanji.UI.view);
  int i;

  for(i = 0; i < number_of_tabs; i++)
    webkit_web_view_reload_bypass_cache(GET_NTH_TAB(i));

  return TRUE;
}

gboolean
cmd_saveas(int argc, char** argv)
{
  char* filename;

  if(argc > 0)
    filename = argv[0];
  else
    filename = (char*) webkit_web_view_get_title(GET_CURRENT_TAB());

  char* uri       = (char*) webkit_web_view_get_uri(GET_CURRENT_TAB());
  char* file      = g_strconcat(download_dir, filename ? filename : uri, NULL);
  char* command   = g_strdup_printf(download_command, uri, file);

  g_spawn_command_line_async(command, NULL);

  g_free(command);
  g_free(file);

  return TRUE;
}

gboolean
cmd_script(int argc, char** argv)
{
  if(argc < 1)
    return TRUE;

  char* path    = argv[0];
  char* content = read_file(path);

  if(!content)
  {
    gchar* message = g_strdup_printf("Could not open or read file '%s'", path);
    notify(ERROR, message);
    g_free(message);
    return FALSE;
  }

  /* search for existing script to overwrite or reread it */
  ScriptList* sl = Jumanji.Global.scripts;
  while(sl && sl->next != NULL)
  {
    if(!strcmp(sl->path, path))
    {
      sl->path    = path;
      sl->content = content;
      return TRUE;
    }

    sl = sl->next;
  }

  /* load new script */
  ScriptList* entry = malloc(sizeof(ScriptList));
  if(!entry)
    out_of_memory();

  entry->path    = path;
  entry->content = content;
  entry->next    = NULL;

  /* append to list */
  if(!Jumanji.Global.scripts)
    Jumanji.Global.scripts = entry;

  if(sl)
    sl->next = entry;

  return TRUE;
}

gboolean
cmd_search_engine(int argc, char** argv)
{
  if(argc < 2)
    return TRUE;

  char* name = argv[0];
  char* uri  = argv[1];

  /* search for existing search engine to overwrite it */
  SearchEngineList* se = Jumanji.Global.search_engines;
  while(se && se->next != NULL)
  {
    if(!strcmp(se->name, name))
    {
      se->uri = uri;
      return TRUE;
    }

    se = se->next;
  }

  /* create new engine */
  SearchEngineList* entry = malloc(sizeof(SearchEngineList));
  if(!entry)
    out_of_memory();

  entry->name = name;
  entry->uri  = uri;
  entry->next = NULL;

  /* append to list */
  if(!Jumanji.Global.search_engines)
    Jumanji.Global.search_engines = entry;

  if(se)
    se->next = entry;

  return TRUE;
}

gboolean
cmd_set(int argc, char** argv)
{
  if(argc <= 0)
    return TRUE;

  /* get webkit settings */
  WebKitWebSettings* browser_settings;
  if(Jumanji.UI.view && gtk_notebook_get_current_page(Jumanji.UI.view) >= 0)
    browser_settings = webkit_web_view_get_settings(GET_CURRENT_TAB());
  else
    browser_settings = Jumanji.Global.browser_settings;

  WebKitWebView* current_wv = NULL;
  if(Jumanji.UI.view && gtk_notebook_get_current_page(Jumanji.UI.view) >= 0)
    current_wv = GET_CURRENT_TAB();

  for(unsigned int i = 0; i < LENGTH(settings); i++)
  {
    if(!strcmp(argv[0], settings[i].name))
    {
      /* check var type */
      if(settings[i].type == 'b')
      {
        gboolean value = TRUE;

        if(argv[1])
        {
          if(!strcmp(argv[1], "false") || !strcmp(argv[1], "0"))
            value = FALSE;
          else
            value = TRUE;
        }

        if(settings[i].variable)
        {
          gboolean *x = (gboolean*) (settings[i].variable);
          *x = !(*x);

          if(argv[1])
            *x = value;
        }

        /* check browser settings */
        if(settings[i].webkitvar)
          g_object_set(G_OBJECT(browser_settings), settings[i].webkitvar, value, NULL);
        if(settings[i].webkitview)
          g_object_set(G_OBJECT(current_wv), settings[i].webkitvar, value, NULL);
      }
      else if(settings[i].type == 'i')
      {
        if(argc != 2)
          return TRUE;

        int id = -1;
        for(unsigned int arg_c = 0; arg_c < LENGTH(argument_names); arg_c++)
        {
          if(!strcmp(argv[1], argument_names[arg_c].name))
          {
            id = argument_names[arg_c].argument;
            break;
          }
        }

        if(id == -1)
          id = atoi(argv[1]);

        if(settings[i].variable)
        {
          int *x = (int*) (settings[i].variable);
          *x = id;
        }

        /* check browser settings */
        if(settings[i].webkitvar)
          g_object_set(G_OBJECT(browser_settings), settings[i].webkitvar, id, NULL);
      }
      else if(settings[i].type == 'f')
      {
        if(argc != 2)
          return TRUE;

        float value = atof(argv[1]);

        if(settings[i].variable)
        {
          float *x = (float*) (settings[i].variable);
          *x = value;
        }

        /* check browser settings */
        if(settings[i].webkitvar)
          g_object_set(G_OBJECT(browser_settings), settings[i].webkitvar, value, NULL);
      }
      else if(settings[i].type == 's')
      {
        if(argc < 2)
          return TRUE;

        /* assembly the arguments back to one string */
        gchar* s = g_strjoinv(" ", &(argv[1]));

        if(settings[i].variable)
        {
          char **x = (char**) settings[i].variable;
          *x = s;
        }

        /* check browser settings */
        if(settings[i].webkitvar)
          g_object_set(G_OBJECT(browser_settings), settings[i].webkitvar, s, NULL);

        // a memory leak can append here
      }
      else if(settings[i].type == 'c')
      {
        if(argc != 2)
          return TRUE;

        char value = argv[1][0];

        if(settings[i].variable)
        {
          char *x = (char*) (settings[i].variable);
          *x = value;
        }

        /* check browser settings */
        if(settings[i].webkitvar)
          g_object_set(G_OBJECT(browser_settings), settings[i].webkitvar, value, NULL);
      }

      /* reload */
      if(settings[i].reload && Jumanji.UI.view)
        if(gtk_notebook_get_current_page(Jumanji.UI.view) >= 0)
          webkit_web_view_reload(GET_CURRENT_TAB());
    }
  }

  /* check specific settings */
  if(Jumanji.UI.statusbar)
  {
    if(show_statusbar)
      gtk_widget_show(GTK_WIDGET(Jumanji.UI.statusbar));
    else
      gtk_widget_hide(GTK_WIDGET(Jumanji.UI.statusbar));
  }

  if(Jumanji.UI.tabbar)
  {
    if(show_tabbar)
      gtk_widget_show(GTK_WIDGET(Jumanji.UI.tabbar));
    else
      gtk_widget_hide(GTK_WIDGET(Jumanji.UI.tabbar));
  }

  update_status();
  return TRUE;
}

gboolean
cmd_stop(int UNUSED(argc), char** UNUSED(argv))
{
  webkit_web_view_stop_loading(GET_CURRENT_TAB());
  return TRUE;
}

gboolean
cmd_tabopen(int argc, char** argv)
{
  if(argc <= 0)
    return TRUE;

  int i;
  GString *uri = g_string_new("");

  for(i = 0; i < argc; i++)
  {
    if(i != 0)
      uri = g_string_append_c(uri, ' ');

    uri = g_string_append(uri, argv[i]);
  }

  create_tab(uri->str, FALSE);

  g_string_free(uri, FALSE);

  return TRUE;
}

gboolean
cmd_winopen(int argc, char** argv)
{
  if(argc <= 0)
    return TRUE;

  int i;
  GString *uri = g_string_new("");

  for(i = 0; i < argc; i++)
  {
    if(i != 0)
      uri = g_string_append_c(uri, ' ');

    uri = g_string_append(uri, argv[i]);
  }

  new_window(uri->str);

  g_string_free(uri, FALSE);

  return TRUE;
}

gboolean
cmd_write(int UNUSED(argc), char** UNUSED(argv))
{
  /* save bookmarks */
  GString *bookmark_list = g_string_new("");

  for(GList* l = Jumanji.Global.bookmarks; l; l = g_list_next(l))
  {
    char* bookmark = g_strconcat((char*) l->data, "\n", NULL);
    bookmark_list = g_string_append(bookmark_list, bookmark);
    g_free(bookmark);
  }

  char* bookmark_file = g_build_filename(g_get_home_dir(), JUMANJI_DIR, JUMANJI_BOOKMARKS, NULL);
  g_file_set_contents(bookmark_file, bookmark_list->str, -1, NULL);

  g_free(bookmark_file);
  g_string_free(bookmark_list, TRUE);

  /* save history */
  GString *history_list = g_string_new("");

  int h_counter = 0;
  for(GList* h = Jumanji.Global.history; h && (!history_limit || h_counter < history_limit); h = g_list_next(h))
  {
    char* uri = g_strconcat((char*) h->data, "\n", NULL);
    history_list = g_string_append(history_list, uri);
    g_free(uri);

    h_counter += 1;
  }

  char* history_file = g_build_filename(g_get_home_dir(), JUMANJI_DIR, JUMANJI_HISTORY, NULL);
  g_file_set_contents(history_file, history_list->str, -1, NULL);

  g_free(history_file);
  g_string_free(history_list, TRUE);

  if(!default_session_name)
    return TRUE;

  /* save session */
  sessionsave(default_session_name);

  GString* session_list = g_string_new("");

  for(GList* se_list = Jumanji.Global.sessions; se_list; se_list = g_list_next(se_list))
  {
    Session* se = se_list->data;

    gchar* session_lines = g_strconcat(se->name, "\n", se->uris, "\n", NULL);
    session_list = g_string_append(session_list, session_lines);

    g_free(session_lines);
  }

  gchar* session_file = g_build_filename(g_get_home_dir(), JUMANJI_DIR, JUMANJI_SESSIONS, NULL);
  g_file_set_contents(session_file, session_list->str, -1, NULL);

  g_free(session_file);
  g_string_free(session_list, TRUE);

  return TRUE;
}

gboolean
cmd_sessionsave(int argc, char** argv)
{
  if(argc <= 0 || argv[argc] != NULL)
    return FALSE;

  gchar* session_name   = g_strjoinv(" ", argv);

  gboolean to_return = sessionsave(session_name);

  g_free(session_name);

  return to_return;
}

gboolean
cmd_sessionswitch(int argc, char** argv)
{
  if(argc <= 0 || argv[argc] != NULL)
    return FALSE;

  gchar* session_name   = g_strjoinv(" ", argv);

  gboolean to_return = sessionswitch(session_name);

  g_free(session_name);

  return to_return;
}

gboolean
cmd_sessionload(int argc, char** argv)
{
  if(argc <= 0 || argv[argc] != NULL)
    return FALSE;

  gchar* session_name = g_strjoinv(" ", argv);

  gboolean to_return = sessionload(session_name);

  g_free(session_name);

  return to_return;
}

/* completion command implementation */
Completion*
cc_open(char* input)
{
  Completion* completion = completion_init();

  /* search engines */
  CompletionGroup* search_engines = completion_group_create("Search engines");
  SearchEngineList* se = Jumanji.Global.search_engines;

  /*if(se)*/
    completion_add_group(completion, search_engines);

  while(se)
  {
    if(!strncmp(se->name, input, strlen(input)))
      completion_group_add_element(search_engines, se->name, NULL);
    se = se->next;
  }

  /* we make bookmark and history completion case insensitive */
  gchar* lowercase_input = g_utf8_strdown(input, -1);

  /* bookmarks */
  if(Jumanji.Global.bookmarks)
  {
    CompletionGroup* bookmarks = completion_group_create("Bookmarks");
    completion_add_group(completion, bookmarks);

    for(GList* l = Jumanji.Global.bookmarks; l; l = g_list_next(l))
    {
      char* bookmark = (char*) l->data;
      gchar* lowercase_bookmark = g_utf8_strdown(bookmark, -1);

      /* case insensitive search */
      if(strstr(lowercase_bookmark, lowercase_input))
        completion_group_add_element(bookmarks, bookmark, NULL);

      g_free(lowercase_bookmark);
    }
  }

  /* history */
  if(Jumanji.Global.history)
  {
    CompletionGroup* history = completion_group_create("History");
    completion_add_group(completion, history);

    for(GList* h = Jumanji.Global.history; h; h = g_list_next(h))
    {
      char* uri = (char*) h->data;
      gchar* lowercase_uri = g_utf8_strdown(uri, -1);

      /* case insensitive search */
      if(strstr(lowercase_uri, lowercase_input))
        completion_group_add_element(history, uri, NULL);

      g_free(lowercase_uri);
    }
  }

  g_free(lowercase_input);

  return completion;
}

Completion*
cc_session(char* input)
{
  Completion* completion = completion_init();
  CompletionGroup* group = completion_group_create(NULL);

  completion_add_group(completion, group);

  for(GList* l = Jumanji.Global.sessions; l; l = g_list_next(l))
  {
    Session* se = l->data;

    if(strstr(se->name, input))
      completion_group_add_element(group, se->name, NULL);
  }

  return completion;
}

Completion*
cc_set(char* input)
{
  Completion* completion = completion_init();
  CompletionGroup* group = completion_group_create(NULL);

  completion_add_group(completion, group);

  unsigned int input_length = input ? strlen(input) : 0;

  for(unsigned int i = 0; i < LENGTH(settings); i++)
  {
    if( (input_length <= strlen(settings[i].name)) &&
        !strncmp(input, settings[i].name, input_length) &&
        !settings[i].init_only)
    {
      completion_group_add_element(group, settings[i].name, settings[i].description);
    }
  }

  return completion;
}

/* buffer command implementation */
void
bcmd_close_tab(char* UNUSED(buffer), Argument* argument)
{
  sc_close_tab(argument);
}

void
bcmd_go_home(char* UNUSED(buffer), Argument* argument)
{
  if(argument->n == NEW_TAB)
    create_tab(home_page, FALSE);
  else
    open_uri(GET_CURRENT_TAB(), home_page);
}

void
bcmd_go_parent(char* buffer, Argument* UNUSED(argument))
{
  char* current_uri = (char*) webkit_web_view_get_uri(GET_CURRENT_TAB());
  if(!current_uri)
    return;

  /* calcuate root */
  int   o = g_str_has_prefix(current_uri, "https://") ? 8 : 7;
  int  rl = 0;
  char* r = current_uri + o;

  while(r && *r != '/')
    rl++, r++;

  char* root = g_strndup(current_uri, o + rl + 1);

  /* go to the root of the website */
  if(!strcmp(buffer, "gU"))
    open_uri(GET_CURRENT_TAB(), root);
  else
  {
    int count = 1;

    if(strlen(buffer) > 2)
      count = atoi(g_strndup(buffer, strlen(buffer) - 2));

    if(count <= 0)
      count = 1;

    char* directories = g_strndup(current_uri + strlen(root), strlen(current_uri) - strlen(root));

    if(strlen(directories) <= 0)
      open_uri(GET_CURRENT_TAB(), root);
    else
    {
      gchar **tokens = g_strsplit(directories, "/", -1);
      int     length = g_strv_length(tokens) - 1;

      GString* tmp = g_string_new("");

      int i;
      for(i = 0; i < length - count; i++)
        g_string_append(tmp, tokens[i]);

      char* new_uri = g_strconcat(root, tmp->str, NULL);
      open_uri(GET_CURRENT_TAB(), new_uri);

      g_free(new_uri);
      g_string_free(tmp, TRUE);
      g_strfreev(tokens);
    }

    g_free(directories);
  }

  g_free(root);
}

void
bcmd_nav_history(char* UNUSED(buffer), Argument* argument)
{
  sc_nav_history(argument);
}

void
bcmd_nav_tabs(char* buffer, Argument* argument)
{
  int current_tab     = gtk_notebook_get_current_page(Jumanji.UI.view);
  int number_of_tabs  = gtk_notebook_get_n_pages(Jumanji.UI.view);
  int step            = 1;

  if(argument->n == PREVIOUS)
    step = -1;

  int new_tab = (current_tab + step) % number_of_tabs;

  if(argument->n == SPECIFIC)
  {
    if (argument->data)
      buffer = argument->data;

    int digit_end = 0;
    while(g_ascii_isdigit(buffer[digit_end]))
      digit_end = digit_end + 1;

    char* number = g_strndup(buffer, digit_end);
    new_tab      = atoi(number) - 1;
    g_free(number);
  }

  gtk_notebook_set_current_page(Jumanji.UI.view, new_tab);
  gtk_widget_grab_focus(GTK_WIDGET(GET_CURRENT_TAB_WIDGET()));

  update_status();
}

void
bcmd_paste(char* UNUSED(buffer), Argument* argument)
{
  sc_paste(argument);
}

void
bcmd_quit(char* UNUSED(buffer), Argument* UNUSED(argument))
{
  cmd_quitall(0, NULL);
}

void
bcmd_scroll(char* buffer, Argument* argument)
{
  if(argument->n == TOP || argument->n == BOTTOM)
  {
    sc_scroll(argument);
    return;
  }

  GtkAdjustment* adjustment = gtk_scrolled_window_get_vadjustment(GET_CURRENT_TAB_WIDGET());

  gdouble view_size = gtk_adjustment_get_page_size(adjustment);
  gdouble max       = gtk_adjustment_get_upper(adjustment) - view_size;

  int number     = atoi(g_strndup(buffer, strlen(buffer) - 1));
  int percentage = (number < 0) ? 0 : (number > 100) ? 100 : number;
  gdouble value  = (max / 100.0f) * (float) percentage;

  gtk_adjustment_set_value(adjustment, value);
}

void
bcmd_spawn(char* UNUSED(buffer), Argument* argument)
{
  char* uri     = (char*) webkit_web_view_get_uri(GET_CURRENT_TAB());
  char* command = g_strdup_printf(argument->data, uri);

  g_spawn_command_line_async(command, NULL);

  g_free(command);
}

void
bcmd_toggle_sourcecode(char* UNUSED(buffer), Argument* argument)
{
  if(argument->n == OPEN_EXTERNAL)
  {
    gchar* uri    = (gchar*) webkit_web_view_get_uri(GET_CURRENT_TAB());
    char* command = g_strdup_printf(spawn_editor, uri);

    g_spawn_command_line_async(command, NULL);

    g_free(command);
  }
  else
    sc_toggle_sourcecode(argument);
}

void
bcmd_zoom(char* buffer, Argument* argument)
{
  float zoom_level = webkit_web_view_get_zoom_level(GET_CURRENT_TAB());

  if(argument->n == ZOOM_IN)
    webkit_web_view_set_zoom_level(GET_CURRENT_TAB(), zoom_level + (float) (zoom_step / 100));
  else if(argument->n == ZOOM_OUT)
    webkit_web_view_set_zoom_level(GET_CURRENT_TAB(), zoom_level - (float) (zoom_step / 100));
  else if(argument->n == ZOOM_ORIGINAL)
    webkit_web_view_set_zoom_level(GET_CURRENT_TAB(), 1.0f);
  else if(argument->n == SPECIFIC)
  {
    char* number = g_strndup(buffer, strlen(buffer) - 1);
    webkit_web_view_set_zoom_level(GET_CURRENT_TAB(), (float) (atoi(number) / 100));
    g_free(number);
  }
}

/* search (special)command global variables */
char* search_item = NULL;
gboolean search_item_changed = FALSE;

/* special command implementation */
gboolean
scmd_search(char* input, Argument* argument, gboolean activate)
{
  static guint search_and_highlight_id;

  if((input && !strlen(input)) || activate)
    return TRUE;

  argument->data = input;

  g_free(search_item);
  search_item = g_strdup(input);
  search_item_changed = TRUE;

  gboolean source_removed = FALSE;
  if(search_and_highlight_id)
  {
    source_removed = g_source_remove(search_and_highlight_id);
    search_and_highlight_id = 0;
  }

  /* if function was launched by cb_inputbar_activate */
  if(activate)
  {
    if(source_removed)
      search_and_highlight(argument);
  }
  else
    search_and_highlight_id = g_timeout_add(search_delay, (GSourceFunc)sc_search, argument);

  return TRUE;
}

gboolean
search_and_highlight(Argument* argument)
{
  static WebKitWebView* last_wv = NULL;

  if(search_item && !strlen(search_item))
    return FALSE;

  WebKitWebView* current_wv = GET_CURRENT_TAB();

  if(search_item_changed || last_wv != current_wv)
  {
    webkit_web_view_unmark_text_matches(current_wv);
    webkit_web_view_mark_text_matches(current_wv, search_item, FALSE, 0);
    webkit_web_view_set_highlight_text_matches(current_wv, TRUE);

    last_wv = current_wv;
  }

  gboolean direction = (argument->n == BACKWARD) ? FALSE : TRUE;
  webkit_web_view_search_text(current_wv, search_item, FALSE, direction, TRUE);

  search_item_changed = FALSE;
  return FALSE;
}

/* callback implementation */
UniqueResponse
cb_app_message_received(UniqueApp* UNUSED(application), gint UNUSED(command), UniqueMessageData* message_data, guint UNUSED(time), gpointer UNUSED(data))
{
  if(message_data)
    create_tab(unique_message_data_get_text(message_data), FALSE);

  return UNIQUE_RESPONSE_OK;
}

gboolean
cb_blank()
{
  return TRUE;
}

gboolean
cb_destroy(GtkWidget* UNUSED(widget), gpointer UNUSED(data))
{
  pango_font_description_free(Jumanji.Style.font);

  /* write bookmarks and history */
  cmd_write(0, NULL);

  /* clear bookmarks */
  GList* list;
  for(list = Jumanji.Global.bookmarks; list; list = g_list_next(list))
    free(list->data);

  g_list_free(Jumanji.Global.bookmarks);

  /* clear history */
  for(list = Jumanji.Global.history; list; list = g_list_next(list))
    free(list->data);

  g_list_free(Jumanji.Global.history);
  g_list_free(Jumanji.Global.last_closed);

  /* clean shortcut list */
  ShortcutList* sc = Jumanji.Bindings.sclist;

  while(sc)
  {
    ShortcutList* ne = sc->next;
    free(sc);
    sc = ne;
  }

  /* clean loaded scripts */
  SearchEngineList* se = Jumanji.Global.search_engines;

  while(se)
  {
    SearchEngineList* ne = se->next;
    free(se);
    se = ne;
  }

  /* clean loaded scripts */
  ScriptList* sl = Jumanji.Global.scripts;

  while(sl)
  {
    ScriptList* ne = sl->next;
    if(sl->content)
      free(sl->content);
    free(sl);
    sl = ne;
  }

  /* clean markers */
  for(list = Jumanji.Global.markers; list; list = g_list_next(list))
    free(list->data);

  g_list_free(Jumanji.Global.markers);

  /* clean command history */
  for(list = Jumanji.Global.command_history; list; list = g_list_next(list))
    free(list->data);

  g_list_free(Jumanji.Global.command_history);

  gtk_main_quit();

  return TRUE;
}

gboolean
cb_inputbar_kb_pressed(GtkWidget* UNUSED(widget), GdkEventKey* event, gpointer UNUSED(data))
{
  guint keyval;
  GdkModifierType consumed_modifiers;

  gdk_keymap_translate_keyboard_state(
      Jumanji.Global.keymap, event->hardware_keycode, event->state, event->group, /* inner */
      &keyval, NULL, NULL, &consumed_modifiers); /* outer */

  /* inputbar shortcuts */
  for(unsigned int i = 0; i < LENGTH(inputbar_shortcuts); i++)
  {
    if (keyval == inputbar_shortcuts[i].key                                               /* test key  */
        && (event->state & ~consumed_modifiers & ALL_MASK) == inputbar_shortcuts[i].mask) /* test mask */
    {
      inputbar_shortcuts[i].function(&(inputbar_shortcuts[i].argument));
      return TRUE;
    }
  }

  return FALSE;
}

void
cb_inputbar_changed(GtkEditable* UNUSED(editable), gpointer UNUSED(data))
{
  /* special commands */
  gchar *input  = gtk_editable_get_chars(GTK_EDITABLE(Jumanji.UI.inputbar), 0, -1);
  char identifier = input[0];
  for(unsigned int i = 0; i < LENGTH(special_commands); i++)
  {
    if((identifier == special_commands[i].identifier) &&
       (special_commands[i].always == 1))
    {
      special_commands[i].function(input + 1, &(special_commands[i].argument), FALSE);
      g_free(input);
      return;
    }
  }

  g_free(input);
  return;
}

gboolean
cb_inputbar_activate(GtkEntry* entry, gpointer UNUSED(data))
{
  gchar  *input  = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
  char identifier = input[0];
  gboolean  retv = FALSE;
  gboolean  succ = FALSE;

  /* no input */
  if(strlen(input) <= 1)
  {
    isc_abort(NULL);
    g_free(input);
    return FALSE;
  }

  /* special commands */
  for(unsigned int i = 0; i < LENGTH(special_commands); i++)
  {
    if(identifier == special_commands[i].identifier)
    {
      retv = special_commands[i].function(input + 1, &(special_commands[i].argument), TRUE);
      if(retv) isc_abort(NULL);
      gtk_widget_grab_focus(GTK_WIDGET(GET_CURRENT_TAB_WIDGET()));
      g_free(input);
      return TRUE;
    }
  }

  gchar **tokens = g_strsplit(input + 1, " ", -1);
  g_free(input);
  gchar *command = tokens[0];
  int     length = g_strv_length(tokens);

  /* append input to the command history */
  if(!private_browsing)
    Jumanji.Global.command_history = g_list_append(Jumanji.Global.command_history, g_strdup(gtk_entry_get_text(entry)));

  /* search commands */
  for(unsigned int i = 0; i < LENGTH(commands); i++)
  {
    if((g_strcmp0(command, commands[i].command) == 0) ||
       (g_strcmp0(command, commands[i].abbr)    == 0))
    {
      retv = commands[i].function(length - 1, tokens + 1);
      succ = TRUE;
      break;
    }
  }

  if(retv)
    isc_abort(NULL);

  if(!succ)
    notify(ERROR, "Unknown command.");

  Argument arg = { HIDE, NULL };
  isc_completion(&arg);

  gtk_widget_grab_focus(GTK_WIDGET(GET_CURRENT_TAB_WIDGET()));
  g_strfreev(tokens);

  return TRUE;
}

gboolean
cb_tab_kb_pressed(GtkWidget* UNUSED(widget), GdkEventKey* event, gpointer UNUSED(data))
{
  guint keyval;
  GdkModifierType consumed_modifiers;

  gdk_keymap_translate_keyboard_state(
      Jumanji.Global.keymap, event->hardware_keycode, event->state, event->group, /* inner */
      &keyval, NULL, NULL, &consumed_modifiers); /* outer */

  for(ShortcutList* sc = Jumanji.Bindings.sclist; sc; sc = sc->next)
  {
    if(
       keyval == sc->element.key                                              /* test key  */
       && (event->state & ~consumed_modifiers & ALL_MASK) == sc->element.mask /* test mask */
       && Jumanji.Global.mode & sc->element.mode                              /* test mode */
       && sc->element.function /* a function have to be defined */
       /* if the buffer isn't empty we don't launch the function
        * exept if the sc mode is set to ALL or have a non nul mask
        */
       && (
            !(Jumanji.Global.buffer && strlen(Jumanji.Global.buffer->str))
            || sc->element.mode == ALL
            || sc->element.mask
          )
      )
    {
      sc->element.function(&(sc->element.argument));
      return TRUE;
    }
  }

  switch(Jumanji.Global.mode)
  {
    case PASS_THROUGH :
      return FALSE;
    case PASS_THROUGH_NEXT :
      change_mode(NORMAL);
      return FALSE;
    case ADD_MARKER :
      add_marker(keyval);
      change_mode(NORMAL);
      return TRUE;
    case EVAL_MARKER :
      eval_marker(keyval);
      change_mode(NORMAL);
      return TRUE;
  }

  /* append only numbers and characters to buffer */
  if(isascii(keyval))
  {
    if(!Jumanji.Global.buffer)
      Jumanji.Global.buffer = g_string_new("");

    Jumanji.Global.buffer = g_string_append_c(Jumanji.Global.buffer, keyval);
    gtk_label_set_text((GtkLabel*) Jumanji.Statusbar.buffer, Jumanji.Global.buffer->str);
  }

  /* follow hints */
  if(Jumanji.Global.mode == FOLLOW)
  {
    Argument argument = {0, event};
    sc_follow_link(&argument);
    return TRUE;
  }

  /* search buffer commands */
  if(Jumanji.Global.buffer)
  {
    BufferCommandList* bc = Jumanji.Bindings.bcmdlist;
    while(bc)
    {
      regex_t regex;
      int     status;

      regcomp(&regex, bc->element.regex, REG_EXTENDED);
      status = regexec(&regex, Jumanji.Global.buffer->str, (size_t) 0, NULL, 0);
      regfree(&regex);

      if(status == 0)
      {
        bc->element.function(Jumanji.Global.buffer->str, &(bc->element.argument));
        g_string_free(Jumanji.Global.buffer, TRUE);
        Jumanji.Global.buffer = NULL;
        gtk_label_set_text((GtkLabel*) Jumanji.Statusbar.buffer, "");

        return TRUE;
      }

      bc = bc->next;
    }
  }

  return FALSE;
}

gboolean
cb_tab_clicked(GtkWidget* UNUSED(widget), GdkEventButton* event, gpointer data)
{
  int position = gtk_notebook_page_num(Jumanji.UI.view, (GtkWidget*) data);

  if(position == -1) {
    return FALSE;
  }

  switch (event->button) {
    case 1:
      gtk_notebook_set_current_page(Jumanji.UI.view, position);
      gtk_widget_grab_focus(GTK_WIDGET(GET_CURRENT_TAB_WIDGET()));
      update_status();
      break;
    case 2:
      close_tab(position);
      break;
  }

  return TRUE;
}

gboolean
cb_wv_button_release_event(GtkWidget* UNUSED(widget), GdkEvent* event, gpointer UNUSED(data))
{
  for(unsigned int i = 0; i < LENGTH(mouse); i++)
  {
    if(
       event->button.button == mouse[i].button              /* test button */
       && (event->button.state & ALL_MASK) == mouse[i].mask /* test mask */
       && Jumanji.Global.mode & mouse[i].mode               /* test mode */
       && mouse[i].function /* a function have to be declared */
      )
    {
      mouse[i].function(&(mouse[i].argument));
      return TRUE;
    }
  }

  return FALSE;
}

gboolean
cb_wv_console(WebKitWebView* UNUSED(wv), char* message, int UNUSED(line),
    char* UNUSED(source), gpointer UNUSED(data))
{
  if(!strcmp(message, "hintmode_off") || !strcmp(message, "insertmode_off"))
    change_mode(NORMAL);
  else if(!strcmp(message, "insertmode_on"))
    change_mode(INSERT);

  return TRUE;
}

GtkWidget*
cb_wv_create_web_view(WebKitWebView* wv, WebKitWebFrame* UNUSED(frame), gpointer UNUSED(data))
{
  char* uri = (char*) webkit_web_view_get_uri(wv);
  GtkWidget* tab = create_tab(uri, TRUE);

  return tab;
}

gboolean
cb_wv_download_request(WebKitWebView* UNUSED(wv), WebKitDownload* download, gpointer UNUSED(data))
{
  const char* uri      = webkit_download_get_uri(download);
  const char* filename = webkit_download_get_suggested_filename(download);

  if(!uri)
  {
    notify(DEFAULT, "Could not retreive download uri");
    return FALSE;
  }

  /* create download directory directory */
  char* download_path = NULL;
  if(download_dir[0] == '~')
    download_path = g_build_filename(g_get_home_dir(), download_dir + 1, NULL);
  else
    download_path = g_strdup(download_dir);

  g_mkdir_with_parents(download_path,  0771);

  /* download file */
  char* file      = g_build_filename(download_path, filename ? filename : uri, NULL);
  char* command   = g_strdup_printf(download_command, uri, file);

  g_spawn_command_line_async(command, NULL);

  g_free(file);
  g_free(command);
  g_free(download_path);

  return TRUE;
}

gboolean
cb_wv_mimetype_policy_decision(WebKitWebView* wv, WebKitWebFrame* UNUSED(frame),
    WebKitNetworkRequest* UNUSED(request), char* mimetype, WebKitWebPolicyDecision* decision,
    gpointer UNUSED(data))
{
  if(!webkit_web_view_can_show_mime_type(wv, mimetype))
  {
    webkit_web_policy_decision_download(decision);
    return TRUE;
  }

  return FALSE;
}

gboolean
cb_wv_hover_link(WebKitWebView* UNUSED(wv), char* UNUSED(title), char* link, gpointer UNUSED(data))
{
  if(link)
  {
    link = g_strconcat("Link: ", link, NULL);
    gtk_label_set_text((GtkLabel*) Jumanji.Statusbar.text, link);
    g_free(link);
  }
  else
    gtk_label_set_text((GtkLabel*) Jumanji.Statusbar.text, webkit_web_view_get_uri(GET_CURRENT_TAB()));

  return TRUE;
}

WebKitWebView*
cb_wv_inspector_view(WebKitWebInspector* UNUSED(inspector), WebKitWebView* wv, gpointer UNUSED(data))
{
  GtkWidget* window;
  GtkWidget* webview;

  /* create window */
  if(Jumanji.UI.embed)
    window = gtk_plug_new(Jumanji.UI.embed);
  else
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /* set title */
  gchar* title = g_strdup_printf("WebInspector (%s) - jumanji", webkit_web_view_get_uri(wv));
  gtk_window_set_title(GTK_WINDOW(window), title);
  g_free(title);

  /* create view */
  webview = webkit_web_view_new();

  gtk_container_add(GTK_CONTAINER(window), webview);
  gtk_widget_show_all(window);

  return WEBKIT_WEB_VIEW(webview);
}

gboolean
cb_wv_nav_policy_decision(WebKitWebView* UNUSED(wv), WebKitWebFrame* UNUSED(frame),
    WebKitNetworkRequest* request, WebKitWebNavigationAction* action,
    WebKitWebPolicyDecision* decision, gpointer UNUSED(data))
{
  switch(webkit_web_navigation_action_get_button(action))
  {
    case 1: /* left mouse button */
      return FALSE;
    case 2: /* middle mouse button */
      create_tab((char*) webkit_network_request_get_uri(request), TRUE);
      webkit_web_policy_decision_ignore(decision);
      return TRUE;
    case 3: /* right mouse button */
      return FALSE;
    default:
      return FALSE;;
  }
}

gboolean
cb_wv_notify_progress(WebKitWebView* wv, GParamSpec* UNUSED(pspec), gpointer UNUSED(data))
{
  if(wv == GET_CURRENT_TAB() && gtk_notebook_get_current_page(Jumanji.UI.view) != -1)
    update_uri();

  return TRUE;
}

gboolean
cb_wv_notify_title(WebKitWebView* wv, GParamSpec* UNUSED(pspec), gpointer UNUSED(data))
{
  const char* title = webkit_web_view_get_title(wv);
  if(title)
  {
    gtk_window_set_title(GTK_WINDOW(Jumanji.UI.window), title);
    update_status();
  }

  return TRUE;
}

gboolean
cb_wv_scrolled(GtkAdjustment* UNUSED(adjustment), gpointer UNUSED(data))
{
  update_position();
  return TRUE;
}

gboolean
cb_wv_window_policy_decision(WebKitWebView* UNUSED(wv), WebKitWebFrame* UNUSED(frame), WebKitNetworkRequest* request,
    WebKitWebNavigationAction* action, WebKitWebPolicyDecision* decision, gpointer UNUSED(data))
{
  if(webkit_web_navigation_action_get_reason(action) == WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED)
  {
    webkit_web_policy_decision_ignore(decision);
    new_window((char*) webkit_network_request_get_uri(request));
    return TRUE;
  }

  return FALSE;
}

gboolean
cb_wv_window_object_cleared(WebKitWebView* UNUSED(wv), WebKitWebFrame* UNUSED(frame), gpointer context,
    gpointer UNUSED(window_object), gpointer UNUSED(data))
{
  /* load all added scripts */

  JSStringRef script;
  JSValueRef exc;
  GString *buffer = g_string_new(NULL);

  for (ScriptList *l = Jumanji.Global.scripts; l; l=l->next) {
    g_string_append(buffer, l->content);
  }
  script = JSStringCreateWithUTF8CString(buffer->str);
  JSEvaluateScript((JSContextRef)context, script, JSContextGetGlobalObject((JSContextRef)context), NULL, 0, &exc);
  JSStringRelease(script);
  g_string_free(buffer, true);
  load_all_scripts();
  return TRUE;
}

/* main function */
int main(int argc, char* argv[])
{
#ifndef G_THREADS_ENABLED
  g_thread_init(NULL);
#endif
  gtk_init(&argc, &argv);

  /* parse arguments & embed */
  Jumanji.UI.embed = 0;
  Jumanji.UI.winid = 0;
  Jumanji.Global.arguments = argv;

  int i;
  for(i = 1; i < argc && argv[i][0] == '-' && argv[i][1] != '\0'; i++)
  {
    switch(argv[i][1])
    {
      case 'e':
        if(++i < argc)
        {
          Jumanji.UI.embed = atoi(argv[i]);
          Jumanji.UI.winid = argv[i];
        }
        break;
    }
  }

  /* init webkit settings and read configuration */
  init_jumanji();
  init_directories();
  init_keylist();
  read_configuration();

  /* single instance */
  UniqueApp* application = NULL;
  if(single_instance)
  {
    application = unique_app_new_with_commands("pwmt.jumanji", NULL, "open", 1, NULL);

    if(unique_app_is_running(application))
    {
      for(; i < argc; i++)
      {
        UniqueMessageData* data = unique_message_data_new();
        unique_message_data_set_text(data, argv[i], strlen(argv[i]));

        unique_app_send_message(application, 1, data);
        unique_message_data_free(data);
      }

      g_object_unref(application);
      return 0;
    }
    else
      g_signal_connect(G_OBJECT(application), "message-received", G_CALLBACK(cb_app_message_received), NULL);
  }

  /* init jumanji and read configuration */
  init_ui();
  init_settings();
  init_data();

  /* init autosave */
  if(auto_save_interval)
    g_timeout_add_seconds(auto_save_interval, auto_save, NULL);

  /* create tab */
  if(argc < 2)
  {
    gboolean session_restored = FALSE;

    // we restore session only if this feature is activated
    // and the current jumanji instance is the only one
    if(default_session_name && application && !unique_app_is_running(application))
      session_restored = sessionload(default_session_name);

    if(!session_restored)
      create_tab(home_page, TRUE);
  }
  else
  {
    for(; i < argc; i++)
      create_tab(argv[i], TRUE);
  }

  gtk_widget_show_all(GTK_WIDGET(Jumanji.UI.window));
  gtk_widget_hide(GTK_WIDGET(Jumanji.UI.inputbar));

  if(!show_statusbar)
    gtk_widget_hide(GTK_WIDGET(Jumanji.UI.statusbar));
  if(!show_tabbar)
    gtk_widget_hide(GTK_WIDGET(Jumanji.UI.tabbar));

  gtk_main();

  return 0;
}
