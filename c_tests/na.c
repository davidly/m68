// nano_vt100_single.cpp
// VT100-hardcoded, CP/M-ish keymap terminal editor (single file).
//
// CP/M-style bindings:
//   ^E Up    ^X Down   ^S Left  ^D Right
//   ^R PgUp  ^C PgDn
//   ^F Find
//   ^W Save  ^A Save As
//   ^T Cut line   ^Y Copy line   ^V Paste (uncut) below current line
//   ^Z Exit
//
// Arrow keys + PgUp/PgDn still work.
// Status bar shows row + VISUAL column (tabs expanded to 8-column stops).
// Help bar auto-fits width (incl. 80 cols).
// Soft tabs: pressing Tab inserts spaces to next tab stop; existing tabs remain unchanged.
// No threads, no sockets/network calls.
// No dependency on C++17 std::clamp().
// Ctrl+Z exit returns a value (no exceptions).

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <algorithm>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

// Simple clamp helpers (avoid C++17 std::clamp)
static inline int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static void die(const char* fmt, ...) {
  const char* seq = "\x1b[?25h\x1b[0m\x1b[2J\x1b[H";
  (void)!write(STDOUT_FILENO, seq, (int)strlen(seq));
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  exit(1);
}

struct TermRaw {
  termios orig{};
  bool enabled = false;

  void enable() {
    if (enabled) return;
    if (tcgetattr(STDIN_FILENO, &orig) == -1) die("tcgetattr failed: %s", strerror(errno));

    termios raw = orig;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
      die("tcsetattr failed: %s", strerror(errno));

    enabled = true;
    const char* hide = "\x1b[?25l";
    (void)!write(STDOUT_FILENO, hide, (int)strlen(hide));
  }

  void disable() {
    if (!enabled) return;
    (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
    enabled = false;
    const char* show = "\x1b[?25h";
    (void)!write(STDOUT_FILENO, show, (int)strlen(show));
  }

  ~TermRaw() { disable(); }
};

static int getWindowSize(int& rows, int& cols) {
  winsize ws{};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) return -1;
  rows = (int)ws.ws_row;
  cols = (int)ws.ws_col;
  return 0;
}

enum Key {
  KEY_NULL = 0,

  KEY_CTRL_A = 1,   // Save As
  KEY_CTRL_C = 3,   // PgDn
  KEY_CTRL_D = 4,   // Right
  KEY_CTRL_E = 5,   // Up
  KEY_CTRL_F = 6,   // Find
  KEY_CTRL_R = 18,  // PgUp
  KEY_CTRL_S = 19,  // Left
  KEY_CTRL_T = 20,  // Cut
  KEY_CTRL_V = 22,  // Paste
  KEY_CTRL_W = 23,  // Save
  KEY_CTRL_X = 24,  // Down
  KEY_CTRL_Y = 25,  // Copy
  KEY_CTRL_Z = 26,  // Exit

  KEY_BACKSPACE = 127,
  KEY_CTRL_H = 8,
  KEY_TAB = 9,
  KEY_ENTER = 13,
  KEY_ESC = 27,

  KEY_DEL = 1000,
  KEY_HOME,
  KEY_END,
  KEY_PGUP,
  KEY_PGDN,
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,

  KEY_CTRL_PGUP,
  KEY_CTRL_PGDN
};

static int readKey() {
  char c;
  while (true) {
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n == 1) break;
    if (n == -1 && errno != EAGAIN) die("read failed: %s", strerror(errno));
  }

  if ((unsigned char)c != KEY_ESC) return (unsigned char)c;

  char seq[8];
  ssize_t n1 = read(STDIN_FILENO, &seq[0], 1);
  ssize_t n2 = read(STDIN_FILENO, &seq[1], 1);
  if (n1 != 1 || n2 != 1) return KEY_ESC;

  if (seq[0] == '[') {
    if (seq[1] >= '0' && seq[1] <= '9') {
      int len = 2;
      while (len < (int)sizeof(seq) - 1) {
        char ch;
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n != 1) break;
        seq[len++] = ch;
        if (ch == '~' || ch == '^' || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
          break;
      }
      seq[len] = '\0';

      int a = 0, b = 0;
      const char* p = &seq[1];
      while (*p >= '0' && *p <= '9') { a = a * 10 + (*p - '0'); p++; }

      if (*p == ';') {
        p++;
        while (*p >= '0' && *p <= '9') { b = b * 10 + (*p - '0'); p++; }
      }

      char term = *p;
      if (term == '~' || term == '^') {
        if (b == 0) {
          switch (a) {
            case 1: return KEY_HOME;
            case 3: return KEY_DEL;
            case 4: return KEY_END;
            case 5: return KEY_PGUP;
            case 6: return KEY_PGDN;
            case 7: return KEY_HOME;
            case 8: return KEY_END;
          }
        } else {
          // Common xterm modifier parameter: 5 = Ctrl
          if (b == 5) {
            if (a == 5) return KEY_CTRL_PGUP;
            if (a == 6) return KEY_CTRL_PGDN;
          }
        }
      }
      return KEY_ESC;
    } else {
      switch (seq[1]) {
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
        case 'H': return KEY_HOME;
        case 'F': return KEY_END;
      }
    }
  } else if (seq[0] == 'O') {
    switch (seq[1]) {
      case 'H': return KEY_HOME;
      case 'F': return KEY_END;
    }
  }

  return KEY_ESC;
}

struct Editor {
  std::string filename;
  std::vector<std::string> lines;

  int cx = 0, cy = 0;
  int rowoff = 0, coloff = 0;

  // Defaults; real size comes from ioctl, but we also fall back in main()
  int screenrows = 60;
  int screencols = 140;

  bool dirty = false;
  char statusmsg[160]{};
  time_t statusmsg_time = 0;

  std::vector<std::string> cutbuf;

  time_t lastCutTime = 0;
  bool lastWasCut = false;
  static const int cutAppendWindowSec = 2;

  void setStatus(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(statusmsg, sizeof(statusmsg), fmt, ap);
    va_end(ap);
    statusmsg_time = time(nullptr);
  }

  void ensureAtLeastOneLine() {
    if (lines.empty()) lines.push_back(std::string());
    cy = clampi(cy, 0, (int)lines.size() - 1);
    cx = clampi(cx, 0, (int)lines[(size_t)cy].size());
  }

  void markNonCutAction() { lastWasCut = false; }

  // Visual column: tabs expand to 8-column stops. Returns 0-based visual column.
  int visualColForCursor(int y, int x) const {
    if (y < 0 || y >= (int)lines.size()) return 0;
    const std::string& s = lines[(size_t)y];
    x = clampi(x, 0, (int)s.size());
    int vcol = 0;
    for (int i = 0; i < x; i++) {
      unsigned char ch = (unsigned char)s[(size_t)i];
      if (ch == '\t') {
        int spaces = 8 - (vcol % 8);
        vcol += spaces;
      } else {
        vcol += 1;
      }
    }
    return vcol;
  }

  void gotoStartOfFile() {
    markNonCutAction();
    ensureAtLeastOneLine();
    cy = 0;
    cx = 0;
    setStatus("Top of file");
  }

  void gotoEndOfFile() {
    markNonCutAction();
    ensureAtLeastOneLine();
    cy = (int)lines.size() - 1;
    cx = 0;
    setStatus("End of file");
  }

  void loadFile(const std::string& path) {
    filename = path;
    lines.clear();

    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
      lines.push_back(std::string());
      dirty = false;
      setStatus("New file");
      return;
    }

    std::string buf;
    char tmp[4096];
    while (true) {
      ssize_t n = read(fd, tmp, sizeof(tmp));
      if (n == 0) break;
      if (n < 0) { close(fd); die("read file failed: %s", strerror(errno)); }
      buf.append(tmp, tmp + n);
    }
    close(fd);

    size_t start = 0;
    while (start <= buf.size()) {
      size_t nl = buf.find('\n', start);
      if (nl == std::string::npos) nl = buf.size();
      std::string line = buf.substr(start, nl - start);
      if (!line.empty() && line.back() == '\r') line.pop_back();
      lines.push_back(line);
      if (nl == buf.size()) break;
      start = nl + 1;
    }

    ensureAtLeastOneLine();
    dirty = false;
    setStatus("Opened %s (%d lines)", filename.c_str(), (int)lines.size());
  }

  bool saveFile(const std::string& path, std::string& err) {
    std::string tmpname = path + ".tmp";
    int fd = open(tmpname.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) { err = std::string("open tmp failed: ") + strerror(errno); return false; }

    for (size_t i = 0; i < lines.size(); i++) {
      const std::string& s = lines[i];
      if (!s.empty()) {
        ssize_t w = write(fd, s.data(), s.size());
        if (w < 0 || (size_t)w != s.size()) {
          err = std::string("write failed: ") + strerror(errno);
          close(fd);
          return false;
        }
      }
      if (i + 1 < lines.size()) {
        const char nl = '\n';
        if (write(fd, &nl, 1) != 1) {
          err = std::string("write newline failed: ") + strerror(errno);
          close(fd);
          return false;
        }
      }
    }

    if (fsync(fd) == -1) { err = std::string("fsync failed: ") + strerror(errno); close(fd); return false; }
    if (close(fd) == -1) { err = std::string("close failed: ") + strerror(errno); return false; }

    if (rename(tmpname.c_str(), path.c_str()) == -1) {
      err = std::string("rename failed: ") + strerror(errno);
      (void)unlink(tmpname.c_str());
      return false;
    }

    filename = path;
    dirty = false;
    return true;
  }

  bool prompt(const std::string& p, std::string& out) {
    out.clear();
    while (true) {
      setStatus("%s%s", p.c_str(), out.c_str());
      refreshScreen();

      int k = readKey();
      if (k == KEY_ESC || k == KEY_CTRL_C) {
        // Note: KEY_CTRL_C is PgDn in this keymap, but inside prompt it's okay to treat as cancel.
        setStatus("Cancelled");
        return false;
      } else if (k == KEY_ENTER) {
        if (!out.empty()) return true;
      } else if (k == KEY_BACKSPACE || k == KEY_CTRL_H || k == KEY_DEL) {
        if (!out.empty()) out.pop_back();
      } else if (k >= 32 && k <= 126) {
        out.push_back((char)k);
      }
    }
  }

  void insertChar(int c) {
    markNonCutAction();
    ensureAtLeastOneLine();
    lines[(size_t)cy].insert(lines[(size_t)cy].begin() + cx, (char)c);
    cx++;
    dirty = true;
  }

  // Soft tab: insert spaces to next 8-column tab stop based on VISUAL column.
  void insertSoftTab() {
    markNonCutAction();
    ensureAtLeastOneLine();
    int vcol = visualColForCursor(cy, cx);
    int spaces = 8 - (vcol % 8);
    if (spaces <= 0) spaces = 8;
    lines[(size_t)cy].insert((size_t)cx, (size_t)spaces, ' ');
    cx += spaces;
    dirty = true;
  }

  void insertNewline() {
    markNonCutAction();
    ensureAtLeastOneLine();
    std::string& line = lines[(size_t)cy];
    std::string right = line.substr((size_t)cx);
    line.erase((size_t)cx);
    lines.insert(lines.begin() + cy + 1, right);
    cy++;
    cx = 0;
    dirty = true;
  }

  void delCharBackspace() {
    markNonCutAction();
    ensureAtLeastOneLine();
    if (cy == 0 && cx == 0) return;

    if (cx > 0) {
      lines[(size_t)cy].erase(lines[(size_t)cy].begin() + (cx - 1));
      cx--;
      dirty = true;
    } else {
      int prevLen = (int)lines[(size_t)(cy - 1)].size();
      lines[(size_t)(cy - 1)] += lines[(size_t)cy];
      lines.erase(lines.begin() + cy);
      cy--;
      cx = prevLen;
      dirty = true;
    }
  }

  void delAtCursor() {
    markNonCutAction();
    ensureAtLeastOneLine();
    if (cx < (int)lines[(size_t)cy].size()) {
      lines[(size_t)cy].erase(lines[(size_t)cy].begin() + cx);
      dirty = true;
      return;
    }
    if (cy + 1 < (int)lines.size()) {
      lines[(size_t)cy] += lines[(size_t)(cy + 1)];
      lines.erase(lines.begin() + (cy + 1));
      dirty = true;
    }
  }

  void copyLineReplace() {
    markNonCutAction();
    ensureAtLeastOneLine();
    cutbuf.clear();
    cutbuf.push_back(lines[(size_t)cy]);
    setStatus("Copied line");
  }

  void cutLineMaybeAppend() {
    ensureAtLeastOneLine();
    time_t now = time(nullptr);

    bool append = lastWasCut && ((now - lastCutTime) <= cutAppendWindowSec);
    if (!append) cutbuf.clear();
    cutbuf.push_back(lines[(size_t)cy]);

    lines.erase(lines.begin() + cy);
    if (lines.empty()) {
      lines.push_back(std::string());
      cy = 0;
      cx = 0;
    } else {
      if (cy >= (int)lines.size()) cy = (int)lines.size() - 1;
      cx = std::min(cx, (int)lines[(size_t)cy].size());
    }

    dirty = true;

    lastWasCut = true;
    lastCutTime = now;

    if (!append) setStatus("Cut line");
    else setStatus("Cut (appended) %d lines", (int)cutbuf.size());
  }

  void uncutPasteBelow() {
    markNonCutAction();
    ensureAtLeastOneLine();
    if (cutbuf.empty()) { setStatus("Cutbuffer empty"); return; }

    int insertAt = std::min(cy + 1, (int)lines.size());
    lines.insert(lines.begin() + insertAt, cutbuf.begin(), cutbuf.end());

    cy = insertAt;
    cx = 0;
    dirty = true;

    if (cutbuf.size() == 1) setStatus("Pasted 1 line");
    else setStatus("Pasted %d lines", (int)cutbuf.size());
  }

  void moveCursor(int key) {
    markNonCutAction();
    ensureAtLeastOneLine();
    switch (key) {
      case KEY_LEFT:
        if (cx > 0) cx--;
        else if (cy > 0) { cy--; cx = (int)lines[(size_t)cy].size(); }
        break;
      case KEY_RIGHT:
        if (cx < (int)lines[(size_t)cy].size()) cx++;
        else if (cy + 1 < (int)lines.size()) { cy++; cx = 0; }
        break;
      case KEY_UP:
        if (cy > 0) cy--;
        break;
      case KEY_DOWN:
        if (cy + 1 < (int)lines.size()) cy++;
        break;
      case KEY_HOME:
        cx = 0;
        break;
      case KEY_END:
        cx = (int)lines[(size_t)cy].size();
        break;
    }
    if (cy >= 0 && cy < (int)lines.size()) {
      int len = (int)lines[(size_t)cy].size();
      if (cx > len) cx = len;
    }
  }

  void pageMove(bool down) {
    markNonCutAction();
    ensureAtLeastOneLine();
    int textrows = std::max(1, screenrows - 3);
    int times = textrows;
    if (!down) cy = std::max(0, cy - times);
    else cy = std::min((int)lines.size() - 1, cy + times);
    cx = std::min(cx, (int)lines[(size_t)cy].size());
  }

  void scroll() {
    ensureAtLeastOneLine();
    if (cy < rowoff) rowoff = cy;

    int textrows = screenrows - 3;
    if (textrows < 1) textrows = 1;
    if (cy >= rowoff + textrows) rowoff = cy - textrows + 1;

    if (cx < coloff) coloff = cx;
    if (cx >= coloff + screencols) coloff = cx - screencols + 1;
  }

  static void abAppend(std::string& ab, const char* s, size_t n) { ab.append(s, n); }

  std::string renderLine(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    int col = 0;
    for (unsigned char ch : s) {
      if (ch == '\t') {
        int spaces = 8 - (col % 8);
        for (int i = 0; i < spaces; i++) { out.push_back(' '); col++; }
      } else if (ch >= 32 && ch <= 126) {
        out.push_back((char)ch);
        col++;
      } else {
        out.push_back('?');
        col++;
      }
    }
    return out;
  }

  void drawRows(std::string& ab) {
    int textrows = screenrows - 3;
    if (textrows < 1) textrows = 1;

    for (int y = 0; y < textrows; y++) {
      int filerow = y + rowoff;
      if (filerow >= (int)lines.size()) {
        abAppend(ab, "~", 1);
      } else {
        std::string r = renderLine(lines[(size_t)filerow]);
        if (coloff < (int)r.size()) r = r.substr((size_t)coloff);
        else r.clear();
        if ((int)r.size() > screencols) r.resize((size_t)screencols);
        abAppend(ab, r.data(), r.size());
      }
      abAppend(ab, "\x1b[K", 3);
      abAppend(ab, "\r\n", 2);
    }
  }

  void drawStatusBar(std::string& ab) {
    abAppend(ab, "\x1b[7m", 4);

    char left[160];
    char right[160];

    const char* name = filename.empty() ? "[No Name]" : filename.c_str();
    snprintf(left, sizeof(left), " %s%s - %d lines ",
             name, dirty ? " (modified)" : "", (int)lines.size());

    int percent = lines.empty() ? 0 : (int)((100LL * (cy + 1)) / (long long)lines.size());
    int dispLine = cy + 1;
    int dispCol  = visualColForCursor(cy, cx) + 1;
    snprintf(right, sizeof(right), " Ln %d, Col %d  %d%% ", dispLine, dispCol, percent);

    int l = (int)strlen(left);
    int r = (int)strlen(right);

    if (l > screencols) l = screencols;
    abAppend(ab, left, (size_t)l);

    while (l < screencols) {
      if (screencols - l == r) { abAppend(ab, right, (size_t)r); break; }
      abAppend(ab, " ", 1);
      l++;
    }

    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
  }

  void drawMessageBar(std::string& ab) {
    abAppend(ab, "\x1b[K", 3);
    if (statusmsg[0] != '\0') {
      time_t now = time(nullptr);
      if (difftime(now, statusmsg_time) < 5.0) {
        int len = (int)strlen(statusmsg);
        if (len > screencols) len = screencols;
        abAppend(ab, statusmsg, (size_t)len);
      }
    }
    abAppend(ab, "\r\n", 2);
  }

  void drawHelpBar(std::string& ab) {
    // Compact CP/M-ish hints; hard-fit to width.
    std::string help =
      " ^EUp ^XDn ^S< ^D>  ^RPgUp ^CPgDn  ^FFind  ^WSave ^ASaveAs  ^TCut ^YCopy ^VPaste  ^ZExit ";
    if ((int)help.size() > screencols) help.resize((size_t)screencols);
    if ((int)help.size() < screencols) help.append((size_t)(screencols - (int)help.size()), ' ');

    abAppend(ab, "\x1b[7m", 4);
    abAppend(ab, help.data(), help.size());
    abAppend(ab, "\x1b[m", 3);
  }

  void refreshScreen() {
    scroll();

    std::string ab;
    ab.reserve((size_t)screenrows * (size_t)(screencols + 32));

    abAppend(ab, "\x1b[?25l", 6);
    abAppend(ab, "\x1b[H", 3);

    drawRows(ab);
    drawStatusBar(ab);
    drawMessageBar(ab);
    drawHelpBar(ab);

    int textrows = screenrows - 3;
    if (textrows < 1) textrows = 1;

    int rx = cx - coloff;
    int ry = cy - rowoff;
    ry = clampi(ry, 0, textrows - 1);
    rx = clampi(rx, 0, screencols - 1);

    char buf[64];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", ry + 1, rx + 1);
    abAppend(ab, buf, strlen(buf));

    abAppend(ab, "\x1b[?25h", 6);
    (void)!write(STDOUT_FILENO, ab.data(), ab.size());
  }

  bool confirm(const std::string& question) {
    setStatus("%s (y/n)", question.c_str());
    refreshScreen();
    while (true) {
      int k = readKey();
      if (k == 'y' || k == 'Y') return true;
      if (k == 'n' || k == 'N' || k == KEY_ESC) return false;
    }
  }

  void doSearch() {
    markNonCutAction();
    std::string q;
    if (!prompt("Find: ", q)) return;

    auto findFrom = [&](int sy, int sx, int& oy, int& ox) -> bool {
      if (q.empty()) return false;
      for (int pass = 0; pass < 2; pass++) {
        for (int y = sy; y < (int)lines.size(); y++) {
          const std::string& s = lines[(size_t)y];
          size_t start = 0;
          if (y == sy) start = (size_t)clampi(sx, 0, (int)s.size());
          size_t pos = s.find(q, start);
          if (pos != std::string::npos) { oy = y; ox = (int)pos; return true; }
          sx = 0;
        }
        sy = 0; sx = 0;
      }
      return false;
    };

    int fy=-1, fx=-1;
    if (findFrom(cy, cx + 1, fy, fx) || findFrom(0, 0, fy, fx)) {
      cy = fy; cx = fx;
      setStatus("Found");
    } else {
      setStatus("Not found");
    }
  }

  void writeOutPrompted() {
    markNonCutAction();
    std::string path = filename;
    std::string newp;
    if (!prompt("Save As: ", newp)) return;
    path = newp;

    std::string err;
    if (saveFile(path, err)) setStatus("Wrote %s", filename.c_str());
    else setStatus("Write failed: %s", err.c_str());
  }

  void quickSave() {
    markNonCutAction();
    if (filename.empty()) {
      writeOutPrompted();
      return;
    }
    std::string err;
    if (saveFile(filename, err)) setStatus("Saved %s", filename.c_str());
    else setStatus("Save failed: %s", err.c_str());
  }

  // Returns true to continue running, false to exit.
  bool processKey(int key) {
    switch (key) {
      // Exit: ^Z
      case KEY_CTRL_Z:
        markNonCutAction();
        if (dirty && !confirm("Unsaved changes. Quit anyway?")) {
          setStatus("Quit cancelled");
          return true;
        }
        return false;

      // CP/M-ish navigation
      case KEY_CTRL_E: moveCursor(KEY_UP); return true;
      case KEY_CTRL_X: moveCursor(KEY_DOWN); return true;
      case KEY_CTRL_S: moveCursor(KEY_LEFT); return true;
      case KEY_CTRL_D: moveCursor(KEY_RIGHT); return true;

      // Page
      case KEY_CTRL_R: pageMove(false); return true; // PgUp
      case KEY_CTRL_C: pageMove(true);  return true; // PgDn

      // Find
      case KEY_CTRL_F: doSearch(); return true;

      // Save / Save As
      case KEY_CTRL_W: quickSave(); return true;
      case KEY_CTRL_A: writeOutPrompted(); return true;

      // Cut / Copy / Paste
      case KEY_CTRL_T: cutLineMaybeAppend(); return true;
      case KEY_CTRL_Y: copyLineReplace(); return true;
      case KEY_CTRL_V: uncutPasteBelow(); return true;

      // Keep arrow keys and PgUp/PgDn working
      case KEY_PGUP: pageMove(false); return true;
      case KEY_PGDN: pageMove(true);  return true;

      case KEY_HOME:
      case KEY_END:
      case KEY_UP:
      case KEY_DOWN:
      case KEY_LEFT:
      case KEY_RIGHT:
        moveCursor(key);
        return true;

      // Optional: Ctrl+PgUp/PgDn (if your terminal emits them)
      case KEY_CTRL_PGUP: gotoStartOfFile(); return true;
      case KEY_CTRL_PGDN: gotoEndOfFile();   return true;

      case KEY_DEL: delAtCursor(); return true;

      case KEY_BACKSPACE:
      case KEY_CTRL_H: delCharBackspace(); return true;

      case KEY_ENTER: insertNewline(); return true;

      case KEY_TAB: insertSoftTab(); return true;

      default:
        if (key >= 32 && key <= 126) insertChar(key);
        else markNonCutAction();
        return true;
    }
  }
};

int main(int argc, char** argv) {
  TermRaw tr;
  tr.enable();

  Editor E;

  // Fallback default if window size can't be determined:
  if (getWindowSize(E.screenrows, E.screencols) == -1) {
    E.screenrows = 60;
    E.screencols = 140;
  }

  if (E.screenrows < 4) E.screenrows = 4;

  if (argc >= 2) E.loadFile(argv[1]);
  else {
    E.lines.push_back(std::string());
    E.dirty = false;
    E.setStatus("^Z Exit | ^W Save | ^A SaveAs | ^F Find | ^T Cut | ^Y Copy | ^V Paste | ^E/^X/^S/^D Move | ^R/^C Page");
  }

  for (;;) {
    E.refreshScreen();
    int k = readKey();
    if (!E.processKey(k)) break;

    // Occasionally refresh size (still safe; if ioctl fails, keep current size)
    static int tick = 0;
    tick++;
    if ((tick & 0xFF) == 0) {
      int r, c;
      if (getWindowSize(r, c) == 0) {
        E.screenrows = std::max(4, r);
        E.screencols = std::max(20, c);
      }
    }
  }

  const char* clear = "\x1b[2J\x1b[H\x1b[0m";
  (void)!write(STDOUT_FILENO, clear, (int)strlen(clear));
  return 0;
}
