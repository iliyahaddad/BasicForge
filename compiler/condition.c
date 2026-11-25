// BasicForge-generated C code
// CODEGEN_V2_MARKER_FINAL
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <io.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#ifndef WINAPI_FAMILY_PARTITION
#define WINAPI_FAMILY_PARTITION(x) 1
#endif

void bf_print_string(const char* s);
void bf_print_int(int64_t v);
void bf_print_double(double v);
void bf_print_bool(int v);
char* bf_str_concat(char*, char*);
int64_t bf_cint(double);
int64_t bf_clng(double);
double bf_cdbl(int64_t);
char* bf_cstr(int64_t);
char* bf_cstr_double(double);
int bf_cbool(int64_t);
int64_t bf_len(const char*);
char* bf_trim(char*);
char* bf_left(char*, int64_t);
char* bf_right(char*, int64_t);
char* bf_mid(char*, int64_t, int64_t);
int64_t bf_instr(const char*, const char*);
char* bf_replace(char*, char*, char*);
char* bf_format(const char*, ...);
double bf_sqr(double);
double bf_abs(double);
int64_t bf_sgn(double);
double bf_fix(double);
double bf_int(double);
double bf_rnd(void);
int64_t bf_form_new(const char*, int, int);
int bf_button_new(int64_t, const char*, int, int, int, int);
int bf_label_new(int64_t, const char*, int, int, int, int);
int bf_textbox_new(int64_t, const char*, int, int, int, int);
int bf_checkbox_new(int64_t, const char*, int, int, int, int);
int bf_radiobutton_new(int64_t, const char*, int, int, int, int);
int bf_groupbox_new(int64_t, const char*, int, int, int, int);
int bf_panel_new(int64_t, const char*, int, int, int, int);
int bf_picturebox_new(int64_t, const char*, int, int, int, int);
int bf_listbox_new(int64_t, const char*, int, int, int, int);
int bf_combobox_new(int64_t, const char*, int, int, int, int);
int bf_trackbar_new(int64_t, const char*, int, int, int, int);
int bf_progressbar_new(int64_t, const char*, int, int, int, int);
int bf_datetimepicker_new(int64_t, const char*, int, int, int, int);
int bf_listview_new(int64_t, const char*, int, int, int, int);
int bf_treeview_new(int64_t, const char*, int, int, int, int);
int bf_richtextbox_new(int64_t, const char*, int, int, int, int);
int bf_tabcontrol_new(int64_t, const char*, int, int, int, int);
int bf_toolbar_new(int64_t, const char*, int, int, int, int);
int bf_statusbar_new(int64_t, const char*, int, int, int, int);
int bf_updown_new(int64_t, const char*, int, int, int, int);
int bf_monthcalendar_new(int64_t, const char*, int, int, int, int);
int bf_scrollbar_new(int64_t, const char*, int, int, int, int);
int bf_hotkey_new(int64_t, const char*, int, int, int, int);
void bf_set_click(int, int);
void bf_set_textchanged(int, int);
void bf_set_doubleclick(int, int);
void bf_set_selection(int, int);
void bf_set_scroll(int, int);
void bf_set_focus(int, int);
void bf_set_blur(int, int);
void bf_set_keydown(int, int);
void bf_set_keyup(int, int);
void bf_set_mousemove(int, int);
void bf_set_mousewheel(int, int);
void bf_set_paint(int, int);
void bf_set_drop(int, int);
char* bf_get_dropped_file(void);
void bf_form_run(int64_t);
char* bf_get_text(int);
void bf_set_text(int, const char*);
void bf_ui_set_name(int, const char*);
char* bf_get_text_name(const char*);
void bf_set_text_name(const char*, const char*);
int bf_form_test_click(int);
int bf_form_close_first(void);
int bf_file_exists(const char*);
char* bf_read_all_text(const char*);
int bf_write_all_text(const char*, const char*);
int bf_append_all_text(const char*, const char*);
char* bf_get_env(const char*);
int bf_set_env(const char*, const char*);
void bf_sleep(int);
void bf_beep(int, int);
void bf_clear_screen(void);
void bf_set_title(const char*);
char* bf_read_line(void);
void bf_message_box(const char*, const char*);
void bf_shell_execute(const char*);
char* bf_clipboard_get(void);
void bf_clipboard_set(const char*);
char* bf_upper(const char*);
char* bf_lower(const char*);
char* bf_chr(int);
int bf_asc(const char*);
int64_t bf_year(double);
int64_t bf_month(double);
int64_t bf_day(double);
double bf_now(void);
static char* bf_str_copy(char*, const char*);
static int bf_str_len(const char*);

void bf_throw(const char* msg);
int bf_try_begin(void);
void bf_try_end(void);
void bf_catch_begin(void);
void bf_catch_end(void);
void bf_finally_begin(void);
void bf_finally_end(void);

char** bf_split(const char* s, const char* delim, int* out_count);

int bf_fopen(const char* filename, const char* mode);
void bf_fclose(int handle);
void bf_fprint(int handle, const char* s);
void bf_fprint_int(int handle, int64_t v);
void bf_fprint_double(int handle, double v);
char* bf_fgetline(int handle);
int bf_feof(int handle);

void Main();

#line 1 ".bf"
void Main() {
  int64_t x;
  x = (int64_t)(5);
  int _r0 = (x > 3);
  if (_r0) goto L0;
  goto L2;
L0:
;
  bf_print_string("Greater");
  goto L1;
L2:
;
  bf_print_string("Less");
  goto L1;
L1:
;
  return;
}

