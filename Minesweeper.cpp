/*
 * 扫雷游戏 (EasyX 版本) - 稳定版
 * 编译环境：Visual Studio + EasyX 图形库
 * 使用 C++ 类与对象设计
 * 左键翻开 | 右键标记 | 自动展开 | 计时 | 剩余雷数
 */

#include <graphics.h>
#include <conio.h>
#include <ctime>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <chrono>

 // ==================== 常量定义 ====================
const int CELL_SIZE = 30;     // 格子大小
const int HEADER_HEIGHT = 95;     // 顶部信息栏高度
const int PADDING = 10;     // 边距

// 颜色定义
const COLORREF COLOR_BG = RGB(224, 224, 224);
const COLORREF COLOR_UNREVEALED = RGB(192, 192, 192);
const COLORREF COLOR_HL = RGB(255, 255, 255);
const COLORREF COLOR_SHADOW = RGB(128, 128, 128);
const COLORREF COLOR_REVEALED = RGB(220, 220, 220);
const COLORREF COLOR_NUMBER[9] = {
    RGB(0, 0, 0),       // 0 (不显示)
    RGB(0, 0, 255),     // 1 蓝
    RGB(0, 128, 0),     // 2 绿
    RGB(255, 0, 0),     // 3 红
    RGB(0, 0, 128),     // 4 深蓝
    RGB(128, 0, 0),     // 5 深红
    RGB(0, 128, 128),   // 6 青
    RGB(0, 0, 0),       // 7 黑
    RGB(128, 128, 128)  // 8 灰
};

enum class Difficulty { BEGINNER = 0, INTERMEDIATE, EXPERT };

// ==================== 格子结构体 ====================
struct Cell {
    bool isMine = false;
    bool isRevealed = false;
    bool isFlagged = false;
    int  adjacentMines = 0;
};

// ==================== 扫雷游戏类 ====================
class Minesweeper {
private:
    int rows, cols;                // 雷区行列数
    int totalMines;                // 总雷数
    int flagsUsed;                 // 已使用旗数
    int revealedCount;             // 已翻开的安全格子数
    bool gameOver;                 // 游戏结束标志
    bool gameWon;                  // 胜利标志
    bool firstClick;               // 首次点击标志
    Difficulty difficulty;         // 当前难度

    std::vector<std::vector<Cell>> board;  // 游戏板

    std::mt19937 rng;              // 随机数引擎

    // 计时
    clock_t startTime;
    int elapsedSeconds;
    bool timerRunning;

    // 窗口尺寸
    int windowWidth, windowHeight;

    // 8方向偏移量
    const int dx[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    const int dy[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };

public:
    Minesweeper() : rows(9), cols(9), totalMines(10), flagsUsed(0),
        revealedCount(0), gameOver(false), gameWon(false),
        firstClick(false), difficulty(Difficulty::BEGINNER),
        startTime(0), elapsedSeconds(0), timerRunning(false) {
        auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
        rng.seed(static_cast<unsigned int>(seed));
    }

    // 初始化难度
    void init(Difficulty diff) {
        difficulty = diff;
        switch (diff) {
        case Difficulty::BEGINNER:    rows = 9;  cols = 9;  totalMines = 10; break;
        case Difficulty::INTERMEDIATE: rows = 16; cols = 16; totalMines = 40; break;
        case Difficulty::EXPERT:      rows = 16; cols = 30; totalMines = 99; break;
        }
        reset();
    }

    // 重置游戏状态
    void reset() {
        gameOver = false;
        gameWon = false;
        firstClick = false;
        flagsUsed = 0;
        revealedCount = 0;
        timerRunning = false;
        elapsedSeconds = 0;
        startTime = 0;

        board.assign(rows, std::vector<Cell>(cols));

        // 计算窗口大小
        windowWidth = cols * CELL_SIZE + 2 * PADDING;
        windowHeight = rows * CELL_SIZE + HEADER_HEIGHT + PADDING;
    }

    int getWindowWidth()  const { return windowWidth; }
    int getWindowHeight() const { return windowHeight; }

    bool isReturnBtn(int x, int y) const { return x >= PADDING && x <= PADDING + 70 && y >= 55 && y <= 85; }
    bool isRestartBtn(int x, int y) const { return x >= windowWidth - PADDING - 70 && x <= windowWidth - PADDING && y >= 55 && y <= 85; }

    // 左键点击处理
    void onLeftClick(int mouseX, int mouseY) {
        if (gameOver) return;

        int row, col;
        if (!screenToCell(mouseX, mouseY, row, col)) return;

        // 如果点击的是已经翻开的数字格子，尝试快捷展开(Chording)
        if (board[row][col].isRevealed) {
            autoRevealFromNumber(row, col);
            return;
        }
        if (board[row][col].isFlagged) return;

        // 首次点击安全生成
        if (!firstClick) {
            generateMines(row, col);
            calculateAdjacentMines();
            firstClick = true;
            startTimer();
        }

        // 踩雷
        if (board[row][col].isMine) {
            board[row][col].isRevealed = true;
            gameOver = true;
            gameWon = false;
            stopTimer();
            drawAll();
            return;
        }

        // 正常翻开
        revealCell(row, col);
        checkWin();
        drawAll();
    }

    // 右键标记/取消标记
    void onRightClick(int mouseX, int mouseY) {
        if (gameOver) return;

        int row, col;
        if (!screenToCell(mouseX, mouseY, row, col)) return;

        // 允许右键点击数字也能触发快捷展开
        if (board[row][col].isRevealed) {
            autoRevealFromNumber(row, col);
            return;
        }

        if (board[row][col].isFlagged) {
            board[row][col].isFlagged = false;
            flagsUsed--;
        }
        else {
            board[row][col].isFlagged = true;
            flagsUsed++;
        }
        drawAll();
    }

    // 更新计时器（由外部循环调用）
    void updateTimer() {
        if (!timerRunning) return;
        clock_t now = clock();
        elapsedSeconds = (int)((now - startTime) / CLOCKS_PER_SEC);
        if (elapsedSeconds > 999) elapsedSeconds = 999;

        // 刷新顶部信息（局部重绘）
        BeginBatchDraw();
        drawHeader();
        EndBatchDraw();
    }

    // 绘制整个游戏界面
    void drawAll() const {
        BeginBatchDraw();
        cleardevice();

        drawHeader();
        drawBoard();

        EndBatchDraw();
    }

private:
    // 屏幕坐标转网格坐标
    bool screenToCell(int x, int y, int& row, int& col) const {
        int boardX = PADDING;
        int boardY = HEADER_HEIGHT;

        // 排除超出游戏网格右边界和下边界的部分
        if (x < boardX || y < boardY || x >= boardX + cols * CELL_SIZE || y >= boardY + rows * CELL_SIZE) 
            return false;

        col = (x - boardX) / CELL_SIZE;
        row = (y - boardY) / CELL_SIZE;
        return (col >= 0 && col < cols && row >= 0 && row < rows);
    }

    // 生成地雷，保证 (safeRow,safeCol) 周围3x3无雷
    void generateMines(int safeRow, int safeCol) {
        std::vector<std::pair<int, int>> safeZone;
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc) {
                int r = safeRow + dr, c = safeCol + dc;
                if (r >= 0 && r < rows && c >= 0 && c < cols)
                    safeZone.push_back({ r, c });
            }

        std::vector<std::pair<int, int>> candidates;
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c) {
                bool isSafe = false;
                for (auto& p : safeZone)
                    if (p.first == r && p.second == c) { isSafe = true; break; }
                if (!isSafe) candidates.push_back({ r, c });
            }

        std::shuffle(candidates.begin(), candidates.end(), rng);
        int place = min(totalMines, (int)candidates.size());
        for (int i = 0; i < place; ++i)
            board[candidates[i].first][candidates[i].second].isMine = true;
    }

    // 计算每个格子周围的雷数
    void calculateAdjacentMines() {
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c) {
                if (board[r][c].isMine) continue;
                int count = 0;
                for (int d = 0; d < 8; ++d) {
                    int nr = r + dx[d], nc = c + dy[d];
                    if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && board[nr][nc].isMine)
                        count++;
                }
                board[r][c].adjacentMines = count;
            }
    }

    // 翻开格子，自动展开空白区域
    void revealCell(int row, int col) {
        if (board[row][col].isRevealed || board[row][col].isFlagged || board[row][col].isMine)
            return;

        std::vector<std::pair<int, int>> queue;
        queue.push_back({ row, col });
        board[row][col].isRevealed = true;
        revealedCount++;

        for (size_t i = 0; i < queue.size(); ++i) {
            int r = queue[i].first, c = queue[i].second;
            if (board[r][c].adjacentMines > 0) continue;

            for (int d = 0; d < 8; ++d) {
                int nr = r + dx[d], nc = c + dy[d];
                if (nr >= 0 && nr < rows && nc >= 0 && nc < cols &&
                    !board[nr][nc].isRevealed && !board[nr][nc].isFlagged && !board[nr][nc].isMine) {
                    board[nr][nc].isRevealed = true;
                    revealedCount++;
                    queue.push_back({ nr, nc });
                }
            }
        }
    }

    // 检查胜利
    void checkWin() {
        if (revealedCount == rows * cols - totalMines) {
            gameOver = true;
            gameWon = true;
            stopTimer();
            // 自动标记剩余雷
            for (int r = 0; r < rows; ++r)
                for (int c = 0; c < cols; ++c)
                    if (board[r][c].isMine && !board[r][c].isFlagged) {
                        board[r][c].isFlagged = true;
                        flagsUsed++;
                    }
        }
    }

    void startTimer() { startTime = clock(); timerRunning = true; }
    void stopTimer() { timerRunning = false; }

    // 快捷展开逻辑（当周围棋帜数等于数字时，自动翻开其余格子）
    void autoRevealFromNumber(int row, int col) {
        if (!board[row][col].isRevealed || board[row][col].adjacentMines == 0) return;

        int flagCount = 0;
        for (int d = 0; d < 8; ++d) {
            int nr = row + dx[d], nc = col + dy[d];
            if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && board[nr][nc].isFlagged) {
                flagCount++;
            }
        }

        if (flagCount == board[row][col].adjacentMines) {
            bool hitMine = false;
            for (int d = 0; d < 8; ++d) {
                int nr = row + dx[d], nc = col + dy[d];
                if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
                    if (!board[nr][nc].isRevealed && !board[nr][nc].isFlagged) {
                        if (board[nr][nc].isMine) {
                            board[nr][nc].isRevealed = true;
                            hitMine = true;
                        } else {
                            revealCell(nr, nc);
                        }
                    }
                }
            }
            if (hitMine) {
                gameOver = true;
                gameWon = false;
                stopTimer();
            } else {
                checkWin();
            }
            drawAll();
        }
    }

    // ==================== 绘图函数 ====================
    void drawHeader() const {
        // 剩余雷数
        int remaining = totalMines - flagsUsed;
        TCHAR buf[16];
        _stprintf_s(buf, _T("%03d"), remaining);
        RECT r1 = { PADDING, 10, PADDING + 60, 45 };
        setfillcolor(BLACK);
        settextcolor(RGB(255, 50, 50));
        fillrectangle(r1.left, r1.top, r1.right, r1.bottom);
        setbkmode(TRANSPARENT);
        settextstyle(20, 0, _T("Consolas"));
        drawtext(buf, &r1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // 计时器
        _stprintf_s(buf, _T("%03d"), elapsedSeconds);
        RECT r2 = { windowWidth - PADDING - 60, 10, windowWidth - PADDING, 45 };
        setfillcolor(BLACK);
        fillrectangle(r2.left, r2.top, r2.right, r2.bottom);
        drawtext(buf, &r2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // 返回按钮
        RECT rRet = { PADDING, 55, PADDING + 70, 85 };
        setfillcolor(COLOR_UNREVEALED);
        solidrectangle(rRet.left, rRet.top, rRet.right, rRet.bottom);
        setcolor(COLOR_SHADOW);
        rectangle(rRet.left, rRet.top, rRet.right, rRet.bottom);
        settextstyle(16, 0, _T("黑体"));
        settextcolor(BLACK);
        drawtext(_T("返回"), (RECT*)&rRet, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // 重玩按钮
        RECT rRes = { windowWidth - PADDING - 70, 55, windowWidth - PADDING, 85 };
        setfillcolor(COLOR_UNREVEALED);
        solidrectangle(rRes.left, rRes.top, rRes.right, rRes.bottom);
        setcolor(COLOR_SHADOW);
        rectangle(rRes.left, rRes.top, rRes.right, rRes.bottom);
        settextstyle(16, 0, _T("黑体"), 0, 0, FW_BOLD, false, false, false);
        drawtext(_T("重玩"), (RECT*)&rRes, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    void drawBoard() const {
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                drawCell(r, c);
    }

    void drawCell(int row, int col) const {
        int x = PADDING + col * CELL_SIZE;
        int y = HEADER_HEIGHT + row * CELL_SIZE;
        const Cell& cell = board[row][col];

        if (cell.isRevealed) {
            setfillcolor(COLOR_REVEALED);
            solidrectangle(x, y, x + CELL_SIZE, y + CELL_SIZE);
            setcolor(COLOR_SHADOW);
            rectangle(x, y, x + CELL_SIZE, y + CELL_SIZE);

            if (cell.isMine)
                drawMine(x, y, false);
            else if (cell.adjacentMines > 0)
                drawNumber(x, y, cell.adjacentMines);
        }
        else {
            drawUnrevealedCell(x, y);
            if (cell.isFlagged) drawFlag(x, y);

            // 游戏结束显示未标记的雷
            if (gameOver && cell.isMine && !cell.isFlagged) {
                setfillcolor(COLOR_REVEALED);
                solidrectangle(x, y, x + CELL_SIZE, y + CELL_SIZE);
                setcolor(COLOR_SHADOW);
                rectangle(x, y, x + CELL_SIZE, y + CELL_SIZE);
                drawMine(x, y, false);
            }
        }
    }

    void drawUnrevealedCell(int x, int y) const {
        setfillcolor(COLOR_UNREVEALED);
        solidrectangle(x, y, x + CELL_SIZE, y + CELL_SIZE);
        // 凸起效果
        setcolor(COLOR_HL);
        setlinestyle(PS_SOLID, 2);
        line(x, y + CELL_SIZE, x, y);
        line(x, y, x + CELL_SIZE, y);
        setcolor(COLOR_SHADOW);
        line(x + 1, y + CELL_SIZE, x + CELL_SIZE, y + CELL_SIZE);
        line(x + CELL_SIZE, y + CELL_SIZE, x + CELL_SIZE, y + 1);
        setcolor(BLACK);
        setlinestyle(PS_SOLID, 1);
    }

    void drawMine(int x, int y, bool exploded) const {
        int cx = x + CELL_SIZE / 2, cy = y + CELL_SIZE / 2, r = CELL_SIZE / 3;
        setfillcolor(exploded ? RED : BLACK);
        setcolor(BLACK);
        solidcircle(cx, cy, r);
        setcolor(WHITE);
        line(cx - r + 2, cy, cx + r - 2, cy);
        line(cx, cy - r + 2, cx, cy + r - 2);
    }

    void drawNumber(int x, int y, int num) const {
        settextstyle(CELL_SIZE - 4, 0, _T("Arial"));
        setbkmode(TRANSPARENT);
        settextcolor(COLOR_NUMBER[num]);
        TCHAR str[2] = { (TCHAR)(_T('0') + num), _T('\0') };
        RECT rect = { x, y, x + CELL_SIZE, y + CELL_SIZE };
        drawtext(str, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    void drawFlag(int x, int y) const {
        int cx = x + CELL_SIZE / 2, cy = y + CELL_SIZE / 2, h = CELL_SIZE / 3;
        setcolor(BLACK);
        setlinestyle(PS_SOLID, 2);
        line(cx, cy - h, cx, cy + h);
        POINT pts[3] = {
            {cx - h + 2, cy - h + 2},
            {cx - h + 2, cy - h + 2 + h},
            {cx + h - 2, cy - h + 2 + h / 2}
        };
        setfillcolor(RED);
        solidpolygon(pts, 3);
        setcolor(BLACK);
        polygon(pts, 3);
    }
};

// ==================== 难度选择（在已创建的窗口中绘制） ====================
Difficulty showDifficultyMenu(int winWidth, int winHeight) {
    // 背景
    setfillcolor(COLOR_BG);
    solidrectangle(0, 0, winWidth, winHeight);

    settextstyle(24, 0, _T("黑体"));
    setbkmode(TRANSPARENT);
    outtextxy(winWidth / 2 - 60, 30, _T("选择难度"));

    RECT btn1 = { winWidth / 2 - 100, 80, winWidth / 2 + 100, 120 };
    RECT btn2 = { winWidth / 2 - 100, 140, winWidth / 2 + 100, 180 };
    RECT btn3 = { winWidth / 2 - 100, 200, winWidth / 2 + 100, 240 };

    auto drawBtn = [&](const RECT& r, const TCHAR* text, bool hover) {
        setfillcolor(hover ? RGB(180, 180, 180) : COLOR_UNREVEALED);
        solidrectangle(r.left, r.top, r.right, r.bottom);
        setcolor(BLACK);
        rectangle(r.left, r.top, r.right, r.bottom);
        settextstyle(18, 0, _T("宋体"));
        drawtext(text, (RECT*)&r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        };

    ExMessage msg;
    Difficulty selected = Difficulty::BEGINNER;
    bool chosen = false;

    while (!chosen) {
        bool h1 = false, h2 = false, h3 = false;
        if (peekmessage(&msg, EX_MOUSE)) {
            if (msg.message == WM_LBUTTONDOWN) {
                if (msg.x > btn1.left && msg.x < btn1.right && msg.y > btn1.top && msg.y < btn1.bottom)
                    selected = Difficulty::BEGINNER, chosen = true;
                if (msg.x > btn2.left && msg.x < btn2.right && msg.y > btn2.top && msg.y < btn2.bottom)
                    selected = Difficulty::INTERMEDIATE, chosen = true;
                if (msg.x > btn3.left && msg.x < btn3.right && msg.y > btn3.top && msg.y < btn3.bottom)
                    selected = Difficulty::EXPERT, chosen = true;
            }
            h1 = (msg.x > btn1.left && msg.x < btn1.right && msg.y > btn1.top && msg.y < btn1.bottom);
            h2 = (msg.x > btn2.left && msg.x < btn2.right && msg.y > btn2.top && msg.y < btn2.bottom);
            h3 = (msg.x > btn3.left && msg.x < btn3.right && msg.y > btn3.top && msg.y < btn3.bottom);
        }

        BeginBatchDraw();
        setfillcolor(COLOR_BG);
        solidrectangle(0, 0, winWidth, winHeight);
        outtextxy(winWidth / 2 - 60, 30, _T("选择难度"));
        drawBtn(btn1, _T("初级 (9x9, 10雷)"), h1);
        drawBtn(btn2, _T("中级 (16x16, 40雷)"), h2);
        drawBtn(btn3, _T("高级 (30x16, 99雷)"), h3);
        EndBatchDraw();

        Sleep(20);
    }

    return selected;
}


// ==================== 主函数（修正版）====================
int main() {
    const int MENU_W = 400, MENU_H = 300;
    initgraph(MENU_W, MENU_H);
    HWND hwnd = GetHWnd();

    bool appRunning = true;
    while (appRunning) {
        // Create menu window
        closegraph();
        initgraph(MENU_W, MENU_H);
        hwnd = GetHWnd();

        setbkcolor(COLOR_BG);
        cleardevice();

        Difficulty diff = showDifficultyMenu(MENU_W, MENU_H);

        Minesweeper game;
        game.init(diff);
        FlushMouseMsgBuffer();

        // Resize via EasyX specific close and init routine since resizing buffer directly isn't easily supported without recreating
        closegraph();
        initgraph(game.getWindowWidth(), game.getWindowHeight());
        hwnd = GetHWnd();
        setbkcolor(COLOR_BG);
        cleardevice();
        game.drawAll();

        ExMessage msg;
        bool backToMenu = false;
        clock_t lastTimerUpdate = clock();

        while (!backToMenu && appRunning) {
            while (peekmessage(&msg, EX_MOUSE)) {
                switch (msg.message) {
                case WM_LBUTTONDOWN:
                    if (game.isReturnBtn(msg.x, msg.y)) {
                        backToMenu = true;
                    } else if (game.isRestartBtn(msg.x, msg.y)) {
                        game.init(diff);
                        game.drawAll();
                    } else {
                        game.onLeftClick(msg.x, msg.y);
                    }
                    break;
                case WM_RBUTTONDOWN:
                    // 只有在点击非顶部UI区域时才触发右键标记
                    if (msg.y > HEADER_HEIGHT) {
                        game.onRightClick(msg.x, msg.y);
                    }
                    break;
                }
            }
            if (clock() - lastTimerUpdate > CLOCKS_PER_SEC / 2) {
                game.updateTimer();
                lastTimerUpdate = clock();
            }
            if (_kbhit() && _getch() == 27) appRunning = false;
            Sleep(20);
        }
    }
    closegraph();
    return 0;
}