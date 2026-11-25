// Native Win32 UI runtime for BasicForge Form Designer
// This provides native Windows controls for forms

#include <windows.h>
#include <stdint.h>
#include <string.h>

// Control types
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

// Control structure
typedef struct {
    char id[64];
    BfControlType type;
    int x, y, width, height;
    char text[256];
    HWND hwnd;
} BfControl;

// Form structure
typedef struct {
    char name[64];
    int width, height;
    BfControl controls[64];
    int control_count;
    HWND hwnd;
} BfForm;

// Create a native window
HWND bf_create_form_window(BfForm* form, HINSTANCE hInstance) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "BasicForgeForm";
    RegisterClass(&wc);
    
    form->hwnd = CreateWindow(
        "BasicForgeForm",
        form->name,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        form->width, form->height,
        NULL, NULL, hInstance, NULL
    );
    
    return form->hwnd;
}

// Add a control to the form
void bf_add_control(BfForm* form, BfControlType type, const char* id, 
                    int x, int y, int w, int h, const char* text) {
    if (form->control_count >= 64) return;
    
    BfControl* ctrl = &form->controls[form->control_count++];
    strncpy(ctrl->id, id, 63);
    ctrl->type = type;
    ctrl->x = x; ctrl->y = y;
    ctrl->width = w; ctrl->height = h;
    strncpy(ctrl->text, text, 255);
    
    // Create native control
    const char* class_name = "BUTTON";
    DWORD style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
    
    switch (type) {
        case BF_BUTTON: class_name = "BUTTON"; style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON; break;
        case BF_LABEL: class_name = "STATIC"; style = WS_CHILD | WS_VISIBLE; break;
        case BF_TEXTBOX: class_name = "EDIT"; style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL; break;
        case BF_CHECKBOX: class_name = "BUTTON"; style = WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX; break;
        case BF_RADIOBUTTON: class_name = "BUTTON"; style = WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON; break;
        case BF_GROUPBOX: class_name = "BUTTON"; style = WS_CHILD | WS_VISIBLE | BS_GROUPBOX; break;
        case BF_PANEL: class_name = "STATIC"; style = WS_CHILD | WS_VISIBLE | SS_LEFT; break;
        default: class_name = "STATIC"; style = WS_CHILD | WS_VISIBLE; break;
    }
    
    ctrl->hwnd = CreateWindow(
        class_name, text, style,
        x, y, w, h,
        form->hwnd, NULL, 
        (HINSTANCE)GetWindowLongPtr(form->hwnd, GWLP_HINSTANCE),
        NULL
    );
}

// Show the form
void bf_show_form(BfForm* form, int cmd_show) {
    ShowWindow(form->hwnd, cmd_show);
    UpdateWindow(form->hwnd);
}

// Message loop
void bf_run_form(BfForm* form) {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}