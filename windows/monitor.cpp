#include "monitor.h"

#define REGISTER_ROWS 9
#define RIGHT_COL_WIDTH 48

char Monitor::FLAG_NAMES[] = {'C', 'Z', 'I', 'D', 'B', 's', 'V', 'N'};

Monitor::Monitor(
    components::Cpu6502 *cpu,
    components::Rom *rom,
    components::Ram *ram,
    int *cycle_count,
    uint16_t *address_bus, uint8_t *data_bus)
{
    this->cpu = cpu;
    this->rom = rom;
    this->ram = ram;
    this->cycle_count = cycle_count;
    this->address_bus = address_bus;
    this->data_bus = data_bus;

    initscr();
    curs_set(0);
    noecho();
    raw();
    start_color();
    use_default_colors();
#define ATTR_RED_WITH_NORMAL 1
    init_pair(ATTR_RED_WITH_NORMAL, COLOR_RED, -1);
#define ATTR_BLUE_HIGHLIGHT 2
    init_pair(ATTR_BLUE_HIGHLIGHT, COLOR_BLUE, -1);
#define ATTR_CYAN 3
    init_pair(ATTR_CYAN, COLOR_CYAN, -1);

    this->win_clock = newwin(3, RIGHT_COL_WIDTH, 0, COLS - RIGHT_COL_WIDTH);
    this->win_data = newwin(REGISTER_ROWS + 2, RIGHT_COL_WIDTH, 2, COLS - RIGHT_COL_WIDTH);
    this->win_inst = newwin(LINES - REGISTER_ROWS - 5, RIGHT_COL_WIDTH, REGISTER_ROWS + 3, COLS - RIGHT_COL_WIDTH);

    this->win_status = newwin(3, COLS, LINES - 3, 0);

    wrefresh(stdscr);
}

Monitor::~Monitor() {}

void Monitor::refresh()
{
    box_draw(this->win_status, 0, 0, 0, 0, 0);
    wrefresh(this->win_status); // Must be first
    this->draw_clock();
    this->draw_data();
    this->draw_inst();
    this->draw_ram();

    // DEBUG:
    // char str[256];
    // sprintf(str, "INST: 0x%02x", this->cpu->inst[0]);
    // mvaddstr(2, 2, str);
}

void Monitor::draw_ram()
{

    char str[32];
    //sprintf(str, "%d %d", COLS - RIGHT_COL_WIDTH - 6, (COLS - RIGHT_COL_WIDTH - 6)/(2 * 3));
    mvwaddstr(stdscr, 1, 1, str);

    int inc = (COLS - RIGHT_COL_WIDTH - 6) / 3;
    for (int l = 0; l < LINES - 2 - 3; l++){
        sprintf(str, "$%04x ", l*inc);
        mvwaddstr(stdscr, 1 + l, 1, str);
        for (int b = 0; b < inc; b++){
            if (*this->address_bus == l*inc+b){
                wattron(stdscr, COLOR_PAIR(ATTR_RED_WITH_NORMAL));
            }
            sprintf(str, "%02x ", this->ram->get_byte(l*inc+b));
            waddstr(stdscr, str);
            wattroff(stdscr,COLOR_PAIR(ATTR_RED_WITH_NORMAL));
        }
    }

        wrefresh(stdscr);
}

void Monitor::draw_data()
{
    if (this->win_data == NULL || this->cpu == NULL)
    {
        return;
    }

    mvwaddstr(this->win_data, 1, 3, "P:");
    for (uint8_t n = 0; n < 8; n++)
    {
        uint8_t shift = 1 << n;
        if ((this->cpu->get_p() & (shift)) == shift)
        {
            wattron(this->win_data, COLOR_PAIR(ATTR_RED_WITH_NORMAL));
        }
        mvwaddch(this->win_data, 1, 6 + 2 * n, FLAG_NAMES[n]);
        wattroff(this->win_data, COLOR_PAIR(ATTR_RED_WITH_NORMAL));
    }
    char buffer[RIGHT_COL_WIDTH];
    sprintf(buffer, " A: 0x%02x %10u", this->cpu->get_a(), this->cpu->get_a());
    mvwaddstr(this->win_data, 2, 2, buffer);
    sprintf(buffer, " X: 0x%02x %10u", this->cpu->get_x(), this->cpu->get_x());
    mvwaddstr(this->win_data, 3, 2, buffer);
    sprintf(buffer, " Y: 0x%02x %10u", this->cpu->get_y(), this->cpu->get_y());
    mvwaddstr(this->win_data, 4, 2, buffer);
    sprintf(buffer, "PC: 0x%04x %8u", this->cpu->get_pc(), this->cpu->get_pc());
    mvwaddstr(this->win_data, 5, 2, buffer);
    sprintf(buffer, " S: 0x%02x %10u", this->cpu->get_s(), this->cpu->get_s());
    mvwaddstr(this->win_data, 6, 2, buffer);

    sprintf(buffer, "ADDR: 0x%04x %6u", *this->address_bus, *this->address_bus);
    mvwaddstr(this->win_data, 8, 2, buffer);
    sprintf(buffer, "DATA: 0x%02x %8u", *this->data_bus, *this->data_bus);
    mvwaddstr(this->win_data, 9, 2, buffer);

    box_draw(this->win_data, UP | DOWN, 0, 0, 0, 0);
    wrefresh(this->win_data);
}

void Monitor::draw_clock()
{
    if (this->win_clock != NULL)
    {
        char str[64];
        sprintf(str, " Cycle: %d", *this->cycle_count);
        mvwaddstr(this->win_clock, 1, 1, str);
        // waddch(this->win_clock, (*this->clk) == 1 ? ACS_UARROW : ACS_DARROW);

        if (this->cpu->get_is_fetching())
        {
            wattron(this->win_clock, COLOR_PAIR(ATTR_RED_WITH_NORMAL));
            waddstr(this->win_clock, "  fetch!");
            wattroff(this->win_clock, COLOR_PAIR(ATTR_RED_WITH_NORMAL));
        }
        else {

            waddstr(this->win_clock, "        ");
        }

        box_draw(this->win_clock, DOWN, 0, 0, 0, 0);
        wrefresh(this->win_clock);
    }
}

void Monitor::draw_inst()
{
    if (this->win_inst != NULL)
    {
        char strdis[64];
        char strbytes[64];
        char straddr[64];
        wclear(this->win_inst);
        // int curline = ((LINES - REGISTER_ROWS) / 8) - 1;

        int inst = this->cpu->get_inst_start();
        for (int i = 0; i < LINES - REGISTER_ROWS - 7; i++)
        {
            sprintf(straddr, "$%04x  ", inst);

            mvwaddstr(this->win_inst, i + 1, 2, straddr);

            if (inst >= 0x4020 && inst <= 0xFFFF)
            {
                inst += this->rom->debug_get_at_addr(inst, strbytes, strdis);

                // if (i == 0)
                // {
                //     mvwaddch(this->win_inst, i + 1, 1, ACS_RARROW);
                // }

                wattron(this->win_inst, COLOR_PAIR(ATTR_CYAN));
                waddstr(this->win_inst, strbytes);
                wattroff(this->win_inst, COLOR_PAIR(ATTR_CYAN));

                if (i == 0)
                {
                    wattron(this->win_inst, COLOR_PAIR(ATTR_BLUE_HIGHLIGHT));
                }
                waddstr(this->win_inst, strdis);
                if (i == 0)
                {
                    wattroff(this->win_inst, COLOR_PAIR(ATTR_BLUE_HIGHLIGHT));
                }
            }
            else
            {
                waddch(this->win_inst, '.');
            }
        }
        box_draw(this->win_inst, UP | DOWN, 0, 0, ACS_BTEE, 0);
        wrefresh(this->win_inst);
    }
}

void Monitor::set_status(std::string str)
{
    if (this->win_status != NULL)
    {
        mvwaddstr(this->win_status, 1, 2, str.c_str());
        wclrtoeol(this->win_status);
        mvwaddch(this->win_status, 1, COLS - 1, ACS_VLINE);
        wrefresh(this->win_status);
    }
}

void Monitor::step()
{
    this->set_status("Step: <space> \tCommand Mode: c");
    char ch;
    while (1)
    {
        ch = getch();
        switch (ch)
        {
        case 0x03:
            raise(SIGINT);
        case ' ':
            return;
        case 'c':
        case 'C':
            if (this->enter_command_mode() == 1)
            {
                raise(SIGINT);
                return;
            }
            this->set_status("Step: <space> \tCommand Mode: c");
        }
    }
}

int Monitor::enter_command_mode()
{
#define CMD_MODE_LINES 5
#define CMD_MODE_COLS 25
    WINDOW *interactive = newwin(CMD_MODE_LINES + 2, CMD_MODE_COLS + 4, 5, 5);
    scrollok(interactive, true);
    box(interactive, 0, 0);

    this->set_status("Exit: <esc>");
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
                this->refresh();
                return 1;
            }
            else if (ch == 0x1b) // ESC
            {
                wclear(interactive);
                wrefresh(interactive);
                delwin(interactive);
                noecho();
                this->refresh();
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
        if (this->cpu != NULL)
        {
            // cpu->a++;
            // cpu->a++;
            this->draw_data();
        }
    }
}

void Monitor::end()
{
    endwin();
}

void Monitor::box_draw(WINDOW *win, _box_intersects intersect, chtype ul, chtype ur, chtype ll, chtype lr)
{
    if (ul == 0)
        ul = ((intersect & UP) == UP) && ((intersect & LEFT) == LEFT) ? ACS_PLUS : ((intersect & UP) == UP) ? ACS_LTEE : ((intersect & LEFT) == LEFT) ? ACS_TTEE : ACS_ULCORNER;

    if (ur == 0)
        ur = ((intersect & UP) == UP) && ((intersect & RIGHT) == RIGHT) ? ACS_PLUS : ((intersect & UP) == UP) ? ACS_RTEE : ((intersect & RIGHT) == RIGHT) ? ACS_TTEE : ACS_URCORNER;

    if (ll == 0)
        ll = ((intersect & DOWN) == DOWN) && ((intersect & LEFT) == LEFT) ? ACS_PLUS : ((intersect & DOWN) == DOWN) ? ACS_LTEE : ((intersect & LEFT) == LEFT) ? ACS_BTEE : ACS_LLCORNER;

    if (lr == 0)
        lr = ((intersect & DOWN) == DOWN) && ((intersect & RIGHT) == RIGHT) ? ACS_PLUS : ((intersect & DOWN) == DOWN) ? ACS_RTEE : ((intersect & RIGHT) == RIGHT) ? ACS_BTEE : ACS_LRCORNER;

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
