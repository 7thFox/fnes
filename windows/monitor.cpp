#include "monitor.h"

#define REGISTER_ROWS 9
#define RIGHT_COL_WIDTH 23

typedef int _box_intersects;
const int UP = 1 << 0;
const int LEFT = 1 << 1;
const int RIGHT = 1 << 2;
const int DOWN = 1 << 3;

void monitor_draw_data(Monitor *monitor);
void monitor_draw_clock(Monitor *monitor);
void monitor_draw_inst(Monitor *monitor);
int monitor_command_mode(Monitor *monitor);
void monitor_set_status(Monitor *monitor, std::string str);
void box_draw(WINDOW *win, _box_intersects intersect, chtype ul, chtype ur, chtype ll, chtype lr);

Monitor *monitor_init(
    Cpu6502 *cpu,
    Rom *rom,
    uint8_t *clk, int *cycle_count,
    uint16_t *address_bus, uint8_t *data_bus)
{
    Monitor *monitor = (Monitor *)malloc(sizeof(Monitor));
    monitor->cpu = cpu;
    monitor->rom = rom;
    monitor->clk = clk;
    monitor->cycle_count = cycle_count;
    monitor->address_bus = address_bus;
    monitor->data_bus = data_bus;

    initscr();
    curs_set(0);
    noecho();
    start_color();
    use_default_colors();
#define ATTR_RED_WITH_NORMAL 1
    init_pair(ATTR_RED_WITH_NORMAL, COLOR_RED, -1);
#define ATTR_BLUE_HIGHLIGHT 2
    init_pair(ATTR_BLUE_HIGHLIGHT, COLOR_BLUE, -1);

    monitor->win_clock = newwin(3, RIGHT_COL_WIDTH, 0, COLS - RIGHT_COL_WIDTH);
    monitor->win_data = newwin(REGISTER_ROWS + 2, RIGHT_COL_WIDTH, 2, COLS - RIGHT_COL_WIDTH);
    monitor->win_inst = newwin(LINES - REGISTER_ROWS - 5, RIGHT_COL_WIDTH, REGISTER_ROWS + 3, COLS - RIGHT_COL_WIDTH);

    monitor->win_status = newwin(3, COLS, LINES - 3, 0);

    refresh();

    return monitor;
}

void monitor_refresh(Monitor *monitor)
{
    box_draw(monitor->win_status, 0, 0, 0, 0, 0);
    wrefresh(monitor->win_status); // Must be first
    monitor_draw_clock(monitor);
    monitor_draw_data(monitor);
    monitor_draw_inst(monitor);

    // DEBUG:
    char str[256];
    sprintf(str, "INST: 0x%02x", monitor->cpu->inst[0]);
    mvaddstr(2, 2, str);

    refresh();
}

char FLAG_NAMES[] = {'C', 'Z', 'I', 'D', 's', 's', 'V', 'N'};
void monitor_draw_data(Monitor *monitor)
{
    if (monitor->win_data == NULL || monitor->cpu == NULL)
    {
        return;
    }

    mvwaddstr(monitor->win_data, 1, 3, "P:");
    for (uint8_t n = 0; n < 8; n++)
    {
        uint8_t shift = 1 << n;
        if ((monitor->cpu->p & (shift)) == shift)
        {
            wattron(monitor->win_data, COLOR_PAIR(ATTR_RED_WITH_NORMAL));
        }
        mvwaddch(monitor->win_data, 1, 6 + 2 * n, FLAG_NAMES[n]);
        wattroff(monitor->win_data, COLOR_PAIR(ATTR_RED_WITH_NORMAL));
    }
    char buffer[RIGHT_COL_WIDTH];
    sprintf(buffer, " A: 0x%02x %10u", monitor->cpu->a, monitor->cpu->a);
    mvwaddstr(monitor->win_data, 2, 2, buffer);
    sprintf(buffer, " X: 0x%02x %10u", monitor->cpu->x, monitor->cpu->x);
    mvwaddstr(monitor->win_data, 3, 2, buffer);
    sprintf(buffer, " Y: 0x%02x %10u", monitor->cpu->y, monitor->cpu->y);
    mvwaddstr(monitor->win_data, 4, 2, buffer);
    sprintf(buffer, "PC: 0x%04x %8u", monitor->cpu->pc, monitor->cpu->pc);
    mvwaddstr(monitor->win_data, 5, 2, buffer);
    sprintf(buffer, " S: 0x%02x %10u", monitor->cpu->s, monitor->cpu->s);
    mvwaddstr(monitor->win_data, 6, 2, buffer);

    sprintf(buffer, "ADDR: 0x%04x %6u", *monitor->address_bus, *monitor->address_bus);
    mvwaddstr(monitor->win_data, 8, 2, buffer);
    sprintf(buffer, "DATA: 0x%02x %8u", *monitor->data_bus, *monitor->data_bus);
    mvwaddstr(monitor->win_data, 9, 2, buffer);

    box_draw(monitor->win_data, UP | DOWN, 0, 0, 0, 0);
    wrefresh(monitor->win_data);
}

void monitor_draw_clock(Monitor *monitor)
{
    if (monitor->win_clock != NULL)
    {
        char str[64];
        sprintf(str, " Cycle: %d", *monitor->cycle_count);
        mvwaddstr(monitor->win_clock, 1, 1, str);
        waddch(monitor->win_clock, (*monitor->clk) == 1 ? ACS_UARROW : ACS_DARROW);
        box_draw(monitor->win_clock, DOWN, 0, 0, 0, 0);
        wrefresh(monitor->win_clock);
    }
}

void monitor_draw_inst(Monitor *monitor)
{
    if (monitor->win_inst != NULL)
    {
        char str[64];
        wclear(monitor->win_inst);
        int curline = ((LINES - REGISTER_ROWS) / 8) - 1;

        for (int i = 0; i < LINES - REGISTER_ROWS - 7; i++)
        {
            int inst = monitor->cpu->pc + i - curline;
            if (i == curline)
            {
                mvwaddch(monitor->win_inst, i + 1, 1, ACS_RARROW);
                wattron(monitor->win_inst, COLOR_PAIR(ATTR_BLUE_HIGHLIGHT));
            }

            if (inst >= 0x4020 && inst <= 0xFFFF)
            {
                sprintf(str, "0x%02x", monitor->rom->image[inst - 0x4020]);
                mvwaddstr(monitor->win_inst, i + 1, 2, str);
            }
            if (i == curline)
            {
                wattroff(monitor->win_inst, COLOR_PAIR(ATTR_BLUE_HIGHLIGHT));
            }
        }
        box_draw(monitor->win_inst, UP | DOWN, 0, 0, ACS_BTEE, 0);
        wrefresh(monitor->win_inst);
    }
}

void monitor_set_status(Monitor *monitor, std::string str)
{
    if (monitor->win_status != NULL)
    {
        // wclear(monitor->win_status);
        mvwaddstr(monitor->win_status, 1, 2, str.c_str());
        wclrtoeol(monitor->win_status);
        mvwaddch(monitor->win_status, 1, COLS - 1, ACS_VLINE);
        // box(monitor->win_status, 0, 0);
        wrefresh(monitor->win_status);
    }
}

void monitor_step(Monitor *monitor)
{
    monitor_set_status(monitor, "Step: <space> \tCommand Mode: c");
    raw();
    char ch;
    while (1)
    {
        ch = getch();
        char str[8];
        sprintf(str, "0x%02x", ch);
        mvaddstr(5, 5, str);
        switch (ch)
        {
        case 0x03:
            raise(SIGINT);
        case ' ':
            return;
        case 'c':
        case 'C':
            if (monitor_command_mode(monitor) == 1)
            {
                raise(SIGINT);
                return;
            }
            monitor_set_status(monitor, "Step: <space> \tCommand Mode: c");
        }
    }
}

int monitor_command_mode(Monitor *monitor)
{
#define CMD_MODE_LINES 5
#define CMD_MODE_COLS 25
    WINDOW *interactive = newwin(CMD_MODE_LINES + 2, CMD_MODE_COLS + 4, 5, 5);
    scrollok(interactive, true);
    box(interactive, 0, 0);

    monitor_set_status(monitor, "Exit: <esc>");
    mvwaddstr(interactive, 1, 1, "Enter Command Bytes: ");

    echo();

    for (int cmds = 1; true; cmds++)
    {
        int ln;
        if (cmds >= CMD_MODE_LINES)
        {
            wmove(interactive, CMD_MODE_LINES + 1, 0);
            wclrtoeol(interactive);
            wscrl(interactive, 1);
            ln = CMD_MODE_LINES;
        }
        else
        {
            ln = cmds + 1;
        }
        box(interactive, 0, 0);
        wmove(interactive, ln, 2);
        wrefresh(interactive);
        int wordcount = 0;
        uint8_t byte = 0;
        int nbytes = 0;
        uint8_t bytes[8];
        for (int i = 0; i < CMD_MODE_COLS; i++)
        {
            char ch = wgetch(interactive);
            if (ch == 0x03) // SIGINT
            {
                wclear(interactive);
                wrefresh(interactive);
                delwin(interactive);
                noecho();
                monitor_refresh(monitor);
                return 1;
            }
            else if (ch == 0x1b) // ESC
            {
                wclear(interactive);
                wrefresh(interactive);
                delwin(interactive);
                noecho();
                monitor_refresh(monitor);
                return 0;
            }
            else if (ch == 0x0a) // Enter
            {
                if (wordcount > 0)
                {
                    if (wordcount == 1) // 1 word byte, shift back
                    {
                        byte >>= 4;
                    }
                    bytes[nbytes++] = byte;
                }
                break;
            }
            else if (wordcount < 2 && ch >= '0' && ch <= '9')
            {
                byte |= (ch - '0') << (4 * (1 - wordcount));
                wordcount++;
            }
            else if (wordcount < 2 && ch >= 'A' && ch <= 'F')
            {
                byte |= (ch - 'A' + 0x0a) << (4 * (1 - wordcount));
                wordcount++;
            }
            else if (wordcount < 2 && ch >= 'a' && ch <= 'f')
            {
                byte |= (ch - 'a' + 0x0a) << (4 * (1 - wordcount));
                wordcount++;
            }
            else if (ch == ' ' && wordcount > 0) // End of word
            {
                if (wordcount == 1) // 1 word byte, shift back
                {
                    byte >>= 4;
                }
                bytes[nbytes++] = byte;
                byte = 0;
                wordcount = 0;
            }
            else // Bad entry; re-echo in red
            {
                wattron(interactive, COLOR_PAIR(ATTR_RED_WITH_NORMAL));
                mvwaddch(interactive, ln, 2 + i, ch);
                wattroff(interactive, COLOR_PAIR(ATTR_RED_WITH_NORMAL));
            }
        }

        wmove(interactive, ln, 2);
        char buffer[8];
        for (int j = 0; j < nbytes; j++)
        {
            sprintf(buffer, "0x%02x ", bytes[j]);
            waddstr(interactive, buffer);
        }
        wclrtoeol(interactive);

        // TODO:
        if (monitor->cpu != NULL)
        {
            // cpu->a++;
            // cpu->a++;
            monitor_draw_data(monitor);
        }
    }
}

void monitor_end(Monitor *monitor)
{
    endwin();
}

void box_draw(WINDOW *win, _box_intersects intersect, chtype ul, chtype ur, chtype ll, chtype lr)
{
    if (ul == 0)
        ul = ((intersect & UP) == UP) && ((intersect & LEFT) == LEFT) ? ACS_PLUS
                                                                      : ((intersect & UP) == UP) ? ACS_LTEE
                                                                                                 : ((intersect & LEFT) == LEFT) ? ACS_TTEE
                                                                                                                                : ACS_ULCORNER;

    if (ur == 0)
        ur = ((intersect & UP) == UP) && ((intersect & RIGHT) == RIGHT) ? ACS_PLUS
                                                                        : ((intersect & UP) == UP) ? ACS_RTEE
                                                                                                   : ((intersect & RIGHT) == RIGHT) ? ACS_TTEE
                                                                                                                                    : ACS_URCORNER;

    if (ll == 0)
        ll = ((intersect & DOWN) == DOWN) && ((intersect & LEFT) == LEFT) ? ACS_PLUS
                                                                          : ((intersect & DOWN) == DOWN) ? ACS_LTEE
                                                                                                         : ((intersect & LEFT) == LEFT) ? ACS_BTEE
                                                                                                                                        : ACS_LLCORNER;

    if (lr == 0)
        lr = ((intersect & DOWN) == DOWN) && ((intersect & RIGHT) == RIGHT) ? ACS_PLUS
                                                                            : ((intersect & DOWN) == DOWN) ? ACS_RTEE
                                                                                                           : ((intersect & RIGHT) == RIGHT) ? ACS_BTEE
                                                                                                                                            : ACS_LRCORNER;

    wborder(win,
            ACS_VLINE,
            ACS_VLINE,
            ACS_HLINE,
            ACS_HLINE,
            ul,
            ur,
            ll,
            lr);
}
