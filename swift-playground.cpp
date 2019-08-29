#include <codecvt>
#include <iostream>
#include <locale>
#include <string>
#include <mutex>

#include "CommandLineOptions.h"
#include "REPL.h"
#include "Strings.h"

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>
#include <richedit.h>
#include <namedpipeapi.h>
#include <io.h>
#include <fcntl.h>

#define BACKGROUND_COLOR RGB(0x1E, 0x1E, 0x1E)
#define FOREGROUND_COLOR RGB(0xDC, 0xDC, 0xDC)
#define BUTTON_WIDTH 150
#define BUTTON_HEIGHT 50

CommandLineOptions g_opts;

HANDLE g_stdout_write_pipe;
HANDLE g_stdout_read_pipe;
HWND g_output;
HWND g_ui;
std::vector<char> g_output_text;
std::mutex g_output_text_lock;

void SetFontToConsolas(HWND window_handle)
{
    HFONT font_handle = CreateFont(
        18, 0, 0, 0,
        FW_NORMAL,
        false, false, false,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        FF_DONTCARE,
        "Consolas");
    SendMessage(window_handle, WM_SETFONT, reinterpret_cast<WPARAM>(font_handle), true);
}

HWND CreateRichEdit(HINSTANCE instance_handle,
                    HWND owner_handle,
                    int x, int y,
                    int width, int height)
{
    LoadLibrary(TEXT("Riched32.dll"));
    HWND edit_handle = CreateWindowEx(
        0, "RICHEDIT", TEXT(""),
        ES_MULTILINE | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL,
        x, y, width, height,
        owner_handle, nullptr, instance_handle, nullptr);

    SendMessage(edit_handle, EM_SETEVENTMASK, 0,
                static_cast<LPARAM>(ENM_CHANGE | ENM_SCROLL));
    SetFontToConsolas(edit_handle);

    SendMessage(edit_handle, EM_SETBKGNDCOLOR, 0, static_cast<LPARAM>(BACKGROUND_COLOR));
    CHARFORMAT fmt = {};
    fmt.cbSize = sizeof(fmt);
    fmt.dwMask = CFM_COLOR;
    fmt.crTextColor = FOREGROUND_COLOR;
    SendMessage(edit_handle, EM_SETCHARFORMAT,
                static_cast<WPARAM>(SPF_SETDEFAULT),
                reinterpret_cast<LPARAM>(&fmt));
    return edit_handle;
}

HWND CreateButton(HINSTANCE instance_handle,
                  HWND owner_handle,
                  int x, int y,
                  int width, int height,
                  const char *text)
{
    HWND btn_handle = CreateWindowEx(
        0, "BUTTON", text,
        WS_VISIBLE | WS_CHILD,
        x, y, width, height,
        owner_handle, nullptr, instance_handle, nullptr);
    Button_Enable(btn_handle, true);
    return btn_handle;
}

std::string GetTextboxText(HWND textbox)
{
    GETTEXTLENGTHEX length_info = { GTL_CLOSE, CP_ACP };
    LONG buff_size = SendMessage(textbox, EM_GETTEXTLENGTHEX, reinterpret_cast<WPARAM>(&length_info), 0) + 1; // Add 1 for null terminator
    std::vector<char> buff(buff_size);
    GETTEXTEX get_text_info = { static_cast<DWORD>(buff_size), GT_DEFAULT, CP_ACP, nullptr, nullptr };
    SendMessage(textbox, EM_GETTEXTEX, reinterpret_cast<WPARAM>(&get_text_info), reinterpret_cast<LPARAM>(buff.data()));
    return std::string(buff.data());
}

unsigned long UpdateOutputTextbox(void *)
{
    for(;;)
    {
        char buff[128];
        DWORD bytes_read;
        ReadFile(g_stdout_read_pipe, buff, sizeof(buff), &bytes_read, nullptr);

        {
            std::lock_guard<std::mutex> guard(g_output_text_lock);
            g_output_text.insert(g_output_text.end(),
                                 buff, buff + bytes_read);
            g_output_text.push_back('\0');
            Edit_SetText(g_output, g_output_text.data());
            g_output_text.pop_back();
        }
    }
}

void ClearOutputTextBox()
{
    std::lock_guard<std::mutex> guard(g_output_text_lock);
    g_output_text.clear();
    char null_char = '\0';
    Edit_SetText(g_output, &null_char);
}

struct Playground
{
    void LayoutWindow();
    void ResetREPL();
    void UpdateContinueButtonText();
    void HandleTextChange();
    void RecompileEverything();
    void ContinueExecution();
    void UpdateLineNumbers(int num_lines);

    HWND m_window;
    HWND m_recompile_btn;
    HWND m_continue_btn;
    HWND m_text;
    HWND m_line_numbers;
    int m_min_line;
    int m_num_lines;
    int m_line_numbers_width;
    std::string m_prev_text;
    std::string m_curr_text;
    std::unique_ptr<REPL> m_repl;
};

void Playground::LayoutWindow()
{
    RECT window_rect;
    GetClientRect(m_window, &window_rect);
    LONG x_recompile = window_rect.left;
    LONG y_recompile = window_rect.top;
    LONG width_recompile = BUTTON_WIDTH;
    LONG height_recompile = BUTTON_HEIGHT;

    LONG x_continue = width_recompile;
    LONG y_continue = y_recompile;
    LONG width_continue = width_recompile * 2;
    LONG height_continue = height_recompile;

    LONG x_lines = x_recompile;
    LONG y_lines = height_recompile;
    LONG width_lines = m_line_numbers_width;
    LONG height_lines = (window_rect.bottom - height_recompile);

    LONG x_text = x_lines + width_lines;
    LONG y_text = y_lines;
    LONG width_text = (window_rect.right - x_lines) / 2;
    LONG height_text = height_lines;

    LONG x_output = x_text + width_text;
    LONG y_output = y_text;
    LONG width_output = (window_rect.right - x_lines) - width_text;
    LONG height_output = height_text / 2;

    LONG x_ui = x_output;
    LONG y_ui = y_output + height_output;
    LONG width_ui = width_output;
    LONG height_ui = window_rect.bottom - (height_output + y_output);

    SetWindowPos(m_recompile_btn, HWND_BOTTOM,
                 x_recompile, y_recompile, width_recompile, height_recompile,
                 SWP_NOZORDER);
    SetWindowPos(m_continue_btn, HWND_BOTTOM,
                 x_continue, y_continue, width_continue, height_continue,
                 SWP_NOZORDER);
    SetWindowPos(m_line_numbers, HWND_BOTTOM,
                 x_lines, y_lines, width_lines, height_lines,
                 SWP_NOZORDER);
    SetWindowPos(m_text, HWND_BOTTOM,
                 x_text, y_text, width_text, height_text,
                 SWP_NOZORDER);
    SetWindowPos(g_output, HWND_BOTTOM,
                 x_output, y_output, width_output, height_output,
                 SWP_NOZORDER);
    SetWindowPos(g_ui, HWND_BOTTOM,
                 x_ui, y_ui, width_ui, height_ui,
                 SWP_NOZORDER);

    RedrawWindow(g_output, nullptr, nullptr, RDW_INVALIDATE);
    RedrawWindow(g_ui, nullptr, nullptr, RDW_INVALIDATE);
}

void Playground::ResetREPL()
{
    llvm::Expected<std::unique_ptr<REPL>> repl = REPL::Create(g_opts.is_playground);
    if(!repl)
    {
        std::string err_str;
        llvm::raw_string_ostream stream(err_str);
        stream << repl.takeError();
        stream.flush();
        SetCurrentLoggingArea(LoggingArea::All);
        Log(err_str, LoggingPriority::Error);
    }

    std::for_each(g_opts.include_paths.begin(), g_opts.include_paths.end(),
                  [&](auto s) { (*repl)->AddModuleSearchPath(s); });
    std::for_each(g_opts.link_paths.begin(), g_opts.link_paths.end(),
                  [&](auto s) { (*repl)->AddLoadSearchPath(s); });

    m_repl = std::move(*repl);
}

void Playground::UpdateContinueButtonText()
{
    std::string new_btn_text = "Continue Execution from Line " + std::to_string(m_min_line);
    if(m_min_line == 0)
        new_btn_text = "Recompile Everything";
    Button_SetText(m_continue_btn, new_btn_text.c_str());
}

void Playground::UpdateLineNumbers(int num_lines)
{
    //TODO(sasha): Handle word wrapping
    std::string line_numbers_text = "";
    for(int i = 1; i <= num_lines; i++)
        line_numbers_text += std::to_string(i) + ".\n";
    Edit_SetText(m_line_numbers, line_numbers_text.c_str());
    SendMessage(m_line_numbers, EM_SHOWSCROLLBAR, SB_VERT, false);
}

void Playground::HandleTextChange()
{
    m_curr_text = GetTextboxText(m_text);

    int num_lines_raw = std::count(m_curr_text.begin(), m_curr_text.end(), '\n') + 1;
    m_line_numbers_width = max(static_cast<int>(log10(num_lines_raw) + 1.0f) * 15, 45);
    LayoutWindow();
    UpdateLineNumbers(num_lines_raw);

    Trim(m_curr_text);
    m_curr_text.erase(std::remove(m_curr_text.begin(), m_curr_text.end(), '\r'), m_curr_text.end()); // Effectively changes \r\n to \n
    if(!StartsWith(m_curr_text, m_prev_text))
        m_min_line = 0;
    m_num_lines = std::count(m_curr_text.begin(), m_curr_text.end(), '\n') + 1;
    UpdateContinueButtonText();
}

void Playground::RecompileEverything()
{
    ResetREPL();
    ClearOutputTextBox();
    m_repl->ExecuteSwift(m_curr_text);
    m_prev_text = m_curr_text;
    m_min_line = m_num_lines;
    UpdateContinueButtonText();
}

void Playground::ContinueExecution()
{
    if(m_min_line == 0)
        return RecompileEverything();

    int i = 0;
    for(int curr_line = 1; i < m_curr_text.size(); i++)
    {
        if(m_curr_text[i] == '\n')
            curr_line++;
        if(curr_line == m_min_line + 1)
            break;
    }
    std::string to_execute = m_curr_text.substr(i);
    m_repl->ExecuteSwift(to_execute);
    m_prev_text = m_curr_text;
    m_min_line = m_num_lines;
    UpdateContinueButtonText();
}

LRESULT CALLBACK PlaygroundWindowProc(HWND window_handle, UINT message, WPARAM wparam, LPARAM lparam)
{
    Playground *playground = reinterpret_cast<Playground *>(GetWindowLongPtr(window_handle, GWLP_USERDATA));
    assert(playground);
    switch (message)
    {
    case WM_COMMAND:
    {
        if(HIWORD(wparam) == EN_CHANGE && reinterpret_cast<HWND>(lparam) == playground->m_text)
        {
            playground->HandleTextChange();
        }
        else if(HIWORD(wparam) == EN_VSCROLL && reinterpret_cast<HWND>(lparam) == playground->m_text)
        {
            POINT scroll_pos;
            SendMessage(playground->m_text, EM_GETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&scroll_pos));
            SendMessage(playground->m_line_numbers, EM_SETSCROLLPOS, 0, reinterpret_cast<LPARAM>(&scroll_pos));
        }
        else
        {
            if(HIWORD(wparam) == BN_CLICKED  && reinterpret_cast<HWND>(lparam) == playground->m_recompile_btn)
                playground->RecompileEverything();
            else if(HIWORD(wparam) == BN_CLICKED && reinterpret_cast<HWND>(lparam) == playground->m_continue_btn)
                playground->ContinueExecution();
            fflush(stdout);
        }
        return 0;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(window_handle, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(window_handle, &ps);
        return 0;
    }
    case WM_SIZE:
    {
        playground->LayoutWindow();
        return 0;
    }
    default:
        return DefWindowProc(window_handle, message, wparam, lparam);
    }
}

std::string ConvertWStringToString(const std::wstring& utf16Str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(utf16Str);
}

int WinMain(HINSTANCE instance_handle, HINSTANCE, char *, int)
{
    int argc;
    LPWSTR *args_wide = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::string> args;
    for(int i = 0; i < argc; i++)
        args.push_back(ConvertWStringToString(args_wide[i]));
    std::vector<char *> argv;
    for(int i = 0; i < argc; i++)
        argv.push_back(const_cast<char *>(args[i].c_str()));
    g_opts = ParseCommandLineOptions(argc, argv.data());
    g_opts.is_playground = true;
    SetLoggingOptions(g_opts.logging_opts);

    WNDCLASS wc = {};
    wc.lpfnWndProc   = PlaygroundWindowProc;
    wc.hInstance     = instance_handle;
    wc.lpszClassName = "Playground Class";
    RegisterClass(&wc);

    HWND window = CreateWindowEx(
        0, "Playground Class", "Swift Playground", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, instance_handle, nullptr);

    if(!window)
    {
        Log(std::string("Failed to open window. Last Error: ") + std::to_string(GetLastError()), LoggingPriority::Error);
        return 1;
    }

    Playground playground = {};
    SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&playground));

    playground.m_window = window;
    playground.m_recompile_btn = CreateButton(instance_handle, window,
                                              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                              "Recompile Everything");
    playground.m_continue_btn = CreateButton(instance_handle, window,
                                             CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                             "Continue Execution From Line 1");
    playground.m_min_line = 0;
    playground.m_num_lines = 1;
    playground.m_line_numbers_width = 45;
    playground.m_text = CreateRichEdit(
        instance_handle, window,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT);
    playground.m_line_numbers = CreateRichEdit(
        instance_handle, window,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT);
    Edit_SetReadOnly(playground.m_line_numbers, true);

    g_output = CreateRichEdit(
        instance_handle, window,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT);

    Edit_SetReadOnly(g_output, true);

    WNDCLASS ui_wc = {};
    ui_wc.lpfnWndProc   = DefWindowProc;
    ui_wc.hInstance     = instance_handle;
    ui_wc.lpszClassName = "UI Class";
    RegisterClass(&ui_wc);

    g_ui = CreateWindowEx(
        0, "UI Class", "", WS_CHILD | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        window, nullptr, instance_handle, nullptr);

    playground.LayoutWindow();

    ShowWindow(window, SW_SHOW);

    AllocConsole();
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    FILE *_;
    freopen_s(&_, "CONOUT$", "w", stdout);
    CreateFile("CONOUT$",  GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    //NOTE(sasha): There are \Bigg{\emph{\textbf{SEVERE}}} performance issues if
    //             pipe buffer size is too small. We just pass the standard on Linux,
    //             which is 0xFFFF.
    if(!CreatePipe(&g_stdout_read_pipe, &g_stdout_write_pipe, nullptr, 0xFFFF))
    {
        Log(std::string("Failed to create pipe. Last Error: ") + std::to_string(GetLastError()), LoggingPriority::Error);
        return 1;
    }
    if(!SetStdHandle(STD_OUTPUT_HANDLE, g_stdout_write_pipe))
    {
        Log(std::string("Failed to set std handle. Last Error: ") + std::to_string(GetLastError()), LoggingPriority::Error);
        return 1;
    }
    int fd = _open_osfhandle(reinterpret_cast<intptr_t>(g_stdout_write_pipe),
                             _O_APPEND | _O_TEXT);
    _dup2(fd, _fileno(stdout));

    HANDLE update_thread = CreateThread(
        nullptr, 0, UpdateOutputTextbox,
        nullptr, 0, nullptr);

    MSG msg = {};
    while(GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    _close(fd);
    return 0;
}
