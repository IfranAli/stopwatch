#include <chrono>
#include <curses.h>
#include <iostream>
#include <csignal>
#include <thread>

namespace BLK_LTR {

  constexpr const char *NUMBERS =
      " ##### ###   ##### ##### #   # ##### ##### ##### ##### #####      "
      " #   #   #   #   #     # #   # #     #         # #   # #   #  ### "
      " #   #   #   #   #     # #   # #     #         # #   # #   #  ### "
      " #   #   #       #     # #   # #     #         # #   # #   #      "
      " #   #   #   ##### ##### ##### ##### ##### ##### ##### #####      "
      " #   #   #   #         #     #     # #   #     # #   #     #      "
      " #   #   #   #         #     #     # #   #     # #   #     #  ### "
      " #   #   #   #         #     #     # #   #     # #   #     #  ### "
      " ##### ##### ##### #####     # ##### #####     # ##### #####      ";

  constexpr const char *NUMBERS2 =
      "   *     *    ***  *****     * *****  ***  *****  ***   ***       "
      "  * *   **   *   *     *    ** *     *   *     * *   * *   *  *** "
      " *   * * *       *    *    * * *     *        *  *   * *   *  *** "
      " *   *   *       *   *    *  * * **  *       *   *   * *  **      "
      " *   *   *      *   ***  *   * **  * * **    *    ***   ** *      "
      " *   *   *     *       * *****     * **  *  *    *   *     *      "
      " *   *   *    *        *     *     * *   *  *    *   *     *  *** "
      "  * *    *   *     *   *     * *   * *   * *     *   * *   *  *** "
      "   *   ***** *****  ***      *  ***   ***  *      ***   **        ";

  char const *style{NUMBERS};

  constexpr int NUMLETTERS = 11;
  constexpr int LETTERHEIGHT = 9;
  constexpr int LETTERWIDTH = 6;
  constexpr int ALPHWIDTH = LETTERWIDTH * NUMLETTERS;
  const wchar_t *font = L"\x2588";
  void PrintLetterNC(WINDOW *win, int x, int y, int idx) {
    int index = idx * LETTERWIDTH;

    wmove(win, y, x);
    for (int i = 0; i < LETTERHEIGHT; ++i) {
      waddnstr(win, &style[index], LETTERWIDTH);
      wmove(win, ++y, x);
      index += ALPHWIDTH;
    }
  }

} /* namespace BLK_LTR */

namespace TIMER {

  enum STATE {
    RUNNING,
    STOPPED,
  };

  class Timer {
   public:
    Timer(unsigned minutes);
    void AddSeconds(unsigned minutes);
    bool Tick();
    STATE state;

    int total_seconds{0};
  };

  Timer::Timer(unsigned seconds) {
    state = STATE::STOPPED;
    AddSeconds(seconds);
  }

  void Timer::AddSeconds(unsigned seconds) {
    total_seconds += seconds;

    if (total_seconds < 0)
      total_seconds = 0;
    if (total_seconds > 5940)
      total_seconds = 5940;
  }

  bool Timer::Tick() { return --total_seconds == 0; }

} // namespace TIMER

void CenterWindow();

bool display_big_text{false};
constexpr int PADDING = 0;
constexpr int MAX_BLOCKS = 5;
constexpr int MIN_WINDOW_WIDTH =
    BLK_LTR::LETTERWIDTH * MAX_BLOCKS + (PADDING * 2);
constexpr int MIN_WINDOW_HEIGHT = BLK_LTR::LETTERHEIGHT + (PADDING * 2);

void Print(WINDOW *win, const TIMER::Timer &timer) {
  CenterWindow();
  char text[MAX_BLOCKS + 1];
  snprintf(text, MAX_BLOCKS + 1, "%02d:%02d", timer.total_seconds / 60,
           timer.total_seconds % 60);

  wattron(win, (timer.state == TIMER::RUNNING) ? COLOR_PAIR(1) : COLOR_PAIR(2));

  if (display_big_text) {
    mvwprintw(win, 0, 0, text);
  } else {
    for (int i = 0; i < MAX_BLOCKS; ++i) {
      BLK_LTR::PrintLetterNC(win, i * BLK_LTR::LETTERWIDTH + PADDING, PADDING,
                             text[i] - '0');
    }
  }

  /* wborder(win, '|', '|', '-', '-', '+', '+', '+', '+'); */
  wattrset(win, A_NORMAL);
}

TIMER::Timer timer(90);
WINDOW *win_timer{nullptr};
void CleanUp() {
  delwin(win_timer);
  endwin();
}

int prev_cols;
int prev_rows;
void CenterWindow() {
  int cols, rows;
  getmaxyx(stdscr, rows, cols);

  int y = (rows / 2) - (MIN_WINDOW_HEIGHT / 2);
  int x = (cols / 2) - (MIN_WINDOW_WIDTH / 2);

  if (prev_cols != cols || prev_rows != rows) {
    prev_cols = cols;
    prev_rows = rows;
    wclear(stdscr);
    wrefresh(stdscr);
  }

  display_big_text = (cols < MIN_WINDOW_WIDTH || rows < MIN_WINDOW_HEIGHT);
  mvwin(win_timer, y, x);
}

void SignalCallbackHandler(int sig_num) {
  if (timer.state == TIMER::RUNNING) {
    timer.state = TIMER::STOPPED;
    return;
  }
  CleanUp();
  exit(0);
}

int main(int argc, char **argv) {
  signal(SIGINT, SignalCallbackHandler);

  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, true);

  if (!has_colors()) {
    puts("colors not supported");
    return 1;
  }
  use_default_colors();
  start_color();

  init_pair(1, COLOR_GREEN, -1);
  init_pair(2, COLOR_RED, -1);

  refresh();
  win_timer = newwin(MIN_WINDOW_HEIGHT, MIN_WINDOW_WIDTH, 0, 0);

  auto sleep_duration = std::chrono::seconds(1);
  bool exit{false};

  while (!exit) {
    Print(win_timer, timer);
    wrefresh(win_timer);

    if (timer.state == TIMER::RUNNING) {
      std::this_thread::sleep_for(sleep_duration);
      if (timer.Tick()) {
        timer.state = TIMER::STOPPED;
      }
    } else {
      switch (std::getchar()) {
        case 'h':timer.AddSeconds(-30);
          break;
        case 'j':timer.AddSeconds(-60);
          break;
        case 'k':timer.AddSeconds(60);
          break;
        case 'l':timer.AddSeconds(30);
          break;

        case 's':
          if (timer.total_seconds) {
            timer.state = TIMER::RUNNING;
          }
          break;

        case 't':
          BLK_LTR::style = (BLK_LTR::style == BLK_LTR::NUMBERS)
                           ? BLK_LTR::NUMBERS2
                           : BLK_LTR::NUMBERS;
          break;

        case 'q':exit = true;
          break;
      }
    }
  }

  CleanUp();
}
