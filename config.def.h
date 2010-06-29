/* settings */
int   default_width  = 800;
int   default_height = 600;
float zoom_step      = 10;
float scroll_step    = 40;

/* completion */
static const char FORMAT_COMMAND[]     = "<b>%s</b>";
static const char FORMAT_DESCRIPTION[] = "<i>%s</i>";

/* directories and files */
static const char JUMANJI_DIR[]   = ".config/jumanji";
static const char JUMANJI_RC[]    = "jumanjirc";

/* browser specific settings */
char* home_page  = "http://www.pwmt.org";
char* user_agent = "jumanji";

/* look */
char* font                   = "monospace normal 8";
char* default_bgcolor        = "#000000";
char* default_fgcolor        = "#DDDDDD";
char* inputbar_bgcolor       = "#141414";
char* inputbar_fgcolor       = "#9FBC00";
char* statusbar_bgcolor      = "#000000";
char* statusbar_fgcolor      = "#FFFFFF";
char* tabbar_bgcolor         = "#000000";
char* tabbar_fgcolor         = "#FFFFFF";
char* tabbar_focus_bgcolor   = "#000000";
char* tabbar_focus_fgcolor   = "#9FBC00";
char* tabbar_separator_color = "#575757";
char* completion_fgcolor     = "#DDDDDD";
char* completion_bgcolor     = "#232323";
char* completion_g_fgcolor   = "#DEDEDE";
char* completion_g_bgcolor   = "#FF00FF";
char* completion_hl_fgcolor  = "#232323";
char* completion_hl_bgcolor  = "#9FBC00";
char* notification_e_bgcolor = "#FF1212";
char* notification_e_fgcolor = "#FFFFFF";
char* notification_w_bgcolor = "#FFF712";
char* notification_w_fgcolor = "#000000";

/* additional settings */
gboolean show_scrollbars = FALSE;
gboolean show_statusbar  = TRUE;
gboolean next_to_current = TRUE;

#define GDK_COSHIFT_MASK GDK_CONTROL_MASK | GDK_SHIFT_MASK

/* shortcuts */
Shortcut shortcuts[] = {
  /* mask,             key,               function,             mode,                argument */
  {GDK_CONTROL_MASK,   GDK_c,             sc_abort,             ALL,                 {0} },
  {0,                  GDK_Escape,        sc_abort,             ALL,                 {0} },
  {0,                  GDK_i,             sc_change_mode,       NORMAL,              { INSERT } },
  {0,                  GDK_v,             sc_change_mode,       NORMAL,              { VISUAL } },
  {GDK_CONTROL_MASK,   GDK_w,             sc_close_tab,         NORMAL,              {0} },
  {0,                  GDK_slash,         sc_focus_inputbar,    NORMAL,              { .data = "/" } },
  {GDK_SHIFT_MASK,     GDK_slash,         sc_focus_inputbar,    NORMAL,              { .data = "/" } },
  {GDK_SHIFT_MASK,     GDK_question,      sc_focus_inputbar,    NORMAL,              { .data = "?" } },
  {0,                  GDK_colon,         sc_focus_inputbar,    NORMAL,              { .data = ":" } },
  {0,                  GDK_o,             sc_focus_inputbar,    NORMAL,              { .data = ":open " } },
  {0,                  GDK_O,             sc_focus_inputbar,    NORMAL,              { .data = ":open ", .n = APPEND_URL } },
  {0,                  GDK_t,             sc_focus_inputbar,    NORMAL,              { .data = ":tabopen " } },
  {0,                  GDK_T,             sc_focus_inputbar,    NORMAL,              { .data = ":tabopen ", .n = APPEND_URL } },
  {0,                  GDK_w,             sc_focus_inputbar,    NORMAL,              { .data = ":winopen " } },
  {0,                  GDK_W,             sc_focus_inputbar,    NORMAL,              { .data = ":winopen ", .n = APPEND_URL } },
  {GDK_CONTROL_MASK,   GDK_i,             sc_nav_history,       NORMAL,              { NEXT } },
  {GDK_CONTROL_MASK,   GDK_o,             sc_nav_history,       NORMAL,              { PREVIOUS } },
  {GDK_CONTROL_MASK,   GDK_Tab,           sc_nav_tabs,          NORMAL,              { NEXT } },
  {GDK_COSHIFT_MASK,   GDK_ISO_Left_Tab,  sc_nav_tabs,          NORMAL,              { PREVIOUS } },
  {0,                  GDK_r,             sc_reload,            NORMAL,              {0} },
  {0,                  GDK_h,             sc_scroll,            NORMAL,              { LEFT } },
  {0,                  GDK_j,             sc_scroll,            NORMAL,              { DOWN } },
  {0,                  GDK_k,             sc_scroll,            NORMAL,              { UP } },
  {0,                  GDK_l,             sc_scroll,            NORMAL,              { RIGHT } },
  {0,                  GDK_Left,          sc_scroll,            NORMAL,              { LEFT } },
  {0,                  GDK_Up,            sc_scroll,            NORMAL,              { UP } },
  {0,                  GDK_Down,          sc_scroll,            NORMAL,              { DOWN } },
  {0,                  GDK_Right,         sc_scroll,            NORMAL,              { RIGHT } },
  {GDK_CONTROL_MASK,   GDK_d,             sc_scroll,            NORMAL,              { HALF_DOWN } },
  {GDK_CONTROL_MASK,   GDK_u,             sc_scroll,            NORMAL,              { HALF_UP } },
  {GDK_CONTROL_MASK,   GDK_f,             sc_scroll,            NORMAL,              { FULL_DOWN } },
  {GDK_CONTROL_MASK,   GDK_b,             sc_scroll,            NORMAL,              { FULL_UP } },
  {0,                  GDK_space,         sc_scroll,            NORMAL,              { FULL_DOWN } },
  {GDK_SHIFT_MASK,     GDK_space,         sc_scroll,            NORMAL,              { FULL_UP } },
  {0,                  GDK_n,             sc_search,            NORMAL,              { FORWARD } },
  {0,                  GDK_N,             sc_search,            NORMAL,              { BACKWARD } },
  {GDK_CONTROL_MASK,   GDK_m,             sc_toggle_statusbar,  NORMAL,              {0} },
  {GDK_CONTROL_MASK,   GDK_q,             sc_quit,              ALL,                 {0} },
  {0,                  GDK_y,             sc_yank,              NORMAL,              {0} },
  {0,                  GDK_plus,          sc_zoom,              NORMAL,              { ZOOM_IN } },
  {0,                  GDK_minus,         sc_zoom,              NORMAL,              { ZOOM_OUT } },
};

/* inputbar shortcuts */
InputbarShortcut inputbar_shortcuts[] = {
  /* mask,             key,               function,                  argument */
  {0,                  GDK_Escape,        isc_abort,                 {0} },
  {GDK_CONTROL_MASK,   GDK_c,             isc_abort,                 {0} },
  {0,                  GDK_Up,            isc_command_history,       { PREVIOUS } },
  {0,                  GDK_Down,          isc_command_history,       { NEXT } },
  {0,                  GDK_Tab,           isc_completion,            { NEXT } },
  {GDK_CONTROL_MASK,   GDK_Tab,           isc_completion,            { NEXT_GROUP } },
  {0,                  GDK_ISO_Left_Tab,  isc_completion,            { PREVIOUS } },
  {GDK_CONTROL_MASK,   GDK_ISO_Left_Tab,  isc_completion,            { PREVIOUS_GROUP } },
  {0,                  GDK_BackSpace,     isc_string_manipulation,   { DELETE_LAST_CHAR } },
  {GDK_CONTROL_MASK,   GDK_h,             isc_string_manipulation,   { DELETE_LAST_CHAR } },
  {GDK_CONTROL_MASK,   GDK_w,             isc_string_manipulation,   { DELETE_LAST_WORD } },
  {GDK_CONTROL_MASK,   GDK_f,             isc_string_manipulation,   { NEXT_CHAR } },
  {GDK_CONTROL_MASK,   GDK_b,             isc_string_manipulation,   { PREVIOUS_CHAR } },
};

/* commands */
Command commands[] = {
  /* command,   abbreviation,   function,            completion,   description  */
  {"back",      "b",            cmd_back,            0,            "Go back in the browser history" },
  {"forward",   "f",            cmd_forward,         0,            "Go forward in the browser history" },
  {"map",       "m",            cmd_map,             0,            "Map a key sequence" },
  {"open",      "o",            cmd_open,            cc_open,      "Open URI in the current tab" },
  {"quit",      "q",            cmd_quit,            0,            "Quit current tab" },
  {"quitall",   0,              cmd_quitall,         0,            "Quit jumanji" },
  {"set",       "s",            cmd_set,             cc_set,       "Set an option" },
  {"tabopen",   "t",            cmd_tabopen,         cc_open,      "Open URI in a new tab" },
  {"winopen",   "w",            cmd_winopen,         cc_open,      "Open URI in a new window" },
};

/* buffer commands */
BufferCommand buffer_commands[] = {
  /* regex,        function,       argument */
  {"^gg$",         bcmd_goto,      { TOP } },
  {"^G$",          bcmd_goto,      { BOTTOM } },
  {"^gt$",         bcmd_nav_tabs,  { NEXT } },
  {"^gT$",         bcmd_nav_tabs,  { PREVIOUS } },
  {"^[0-9]+gt$",   bcmd_nav_tabs,  { SPECIFIC } },
  {"^[0-9]+gT$",   bcmd_nav_tabs,  { SPECIFIC } },
  {"^[0-9]+%$",    bcmd_scroll,    {0} },
  {"^zI$",         bcmd_zoom,      { ZOOM_IN } },
  {"^zO$",         bcmd_zoom,      { ZOOM_OUT } },
  {"^z0$",         bcmd_zoom,      { ZOOM_ORIGINAL } },
  {"^[0-9]+Z$",    bcmd_zoom,      { SPECIFIC } },
};

/* special commands */
SpecialCommand special_commands[] = {
  /* identifier,   function,      a,   argument */
  {'/',            scmd_search,   1,   { DOWN } },
  {'?',            scmd_search,   1,   { UP } },
};

/* settings */
Setting settings[] = {
  /* name,                   variable,                           type, re-init, description */
  {"home_page",              &(home_page),                       's',  FALSE,   "Home page"},
  {"height",                 &(default_height),                  'i',  FALSE,   "Default window height"},
  {"next_to_current",        &(next_to_current),                 'b',  FALSE,   "Open new tab next to the current one"},
  {"scrollbars",             &(show_scrollbars),                 'b',  TRUE,    "Show scrollbars"},
  {"scroll_step",            &(scroll_step),                     'f',  TRUE,    "Scroll step"},
  {"show_statusbar",         &(show_statusbar),                  'b',  TRUE,    "Show statusbar"},
  {"user_agent",             &(user_agent),                      's',  TRUE,    "User agent"},
  {"width",                  &(default_height),                  'i',  FALSE,   "Default window width"},
  {"zoom_step",              &(zoom_step),                       'f',  TRUE,    "Zoom step"},
};

/* shortcut names */
ShortcutName shortcut_names[] = {
  {"quit", sc_quit},
  {"yank", sc_yank},
  {"zoom", sc_zoom},
};

/* argument names */
ArgumentName argument_names[] = {
  {"add_marker",  ADD_MARKER},
  {"down",        DOWN},
  {"eval_marker", EVAL_MARKER},
  {"next",        NEXT},
  {"up",          UP},

};

/* mode names */
ModeName mode_names[] = {
  {"all",        ALL},
};

/* special keys */
GDKKey gdk_keys[] = {
  {"<BackSpace>", GDK_BackSpace},
  {"<CapsLock>",  GDK_Caps_Lock},
  {"<Down>",      GDK_Down},
  {"<Esc>",       GDK_Escape},
  {"<F10>",       GDK_F10},
  {"<F11>",       GDK_F11},
  {"<F12>",       GDK_F12},
  {"<F1>",        GDK_F1},
  {"<F2>",        GDK_F2},
  {"<F3>",        GDK_F3},
  {"<F4>",        GDK_F4},
  {"<F5>",        GDK_F5},
  {"<F6>",        GDK_F6},
  {"<F7>",        GDK_F7},
  {"<F8>",        GDK_F8},
  {"<F9>",        GDK_F9},
  {"<Left>",      GDK_Left},
  {"<PageDown>",  GDK_Page_Down},
  {"<PageUp>",    GDK_Page_Up},
  {"<Return>",    GDK_Return},
  {"<Right>",     GDK_Right},
  {"<Space>",     GDK_space},
  {"<Super>",     GDK_Super_L},
  {"<Tab>",       GDK_Tab},
  {"<Up>",        GDK_Up},
};
