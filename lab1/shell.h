#ifndef SHELL_H
#define SHELL_H

// 定義緩衝區的最大長度
#define MAX_BUFFER_LEN 128

// 特殊字符的枚舉類型
enum SPECIAL_CHARACTER {
  BACK_SPACE = 127,     // 退格鍵
  LINE_FEED = 10,       // 換行鍵
  CARRIAGE_RETURN = 13, // 回車鍵

  REGULAR_INPUT = 1000, // 常規輸入
  NEW_LINE = 1001,      // 新的一行

  UNKNOWN = -1, // 未知字符

};

// 啟動Shell的函數
void shell_start();
// 解析輸入字符的函數，返回特殊字符枚舉值
enum SPECIAL_CHARACTER parse(char);
// 命令控制器，根據解析的結果執行不同的操作
void command_controller(enum SPECIAL_CHARACTER, char c, char[], int *);

#endif
