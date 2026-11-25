#ifndef BF_UI_H
#define BF_UI_H

#include <windows.h>

typedef enum {
    BF_BUTTON = 0,
    BF_LABEL,
    BF_TEXTBOX,
    BF_CHECKBOX,
    BF_RADIOBUTTON,
    BF_GROUPBOX,
    BF_PANEL,
    BF_PICTUREBOX,
    BF_LISTBOX,
    BF_COMBOBOX
} BfControlType;

typedef struct {
    char id[64];
    BfControlType type;
    int x, y, width, height;
    char text[256];
    HWND hwnd;
} BfControl;

typedef struct {
    char name[64];
    int width, height;
    BfControl controls[64];
    int control_count;
    HWND hwnd;
} BfForm;

HWND bf_create_form_window(BfForm* form, HINSTANCE hInstance);
void bf_add_control(BfForm* form, BfControlType type, const char* id, 
                    int x, int y, int w, int h, const char* text);
void bf_show_form(BfForm* form, int cmd_show);
void bf_run_form(BfForm* form);

#endif // BF_UI_H