exec 的時候，可以 malloc 然後複製一個檔案過去
sp 用給定的就好

user program 從現在的thread開始跑，可以寫一個test thread 裡面CALL EXEC 專門FOR這個的

TODOS:
1. uartread, uartwrite V
2. fork
3. update thread for user stack (and exec to let the program run correctly)
4. timer interrupt