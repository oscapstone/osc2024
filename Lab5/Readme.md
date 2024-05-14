DEMO TODOs:
* 搞懂那些 switch to, 關東東在幹嘛
* 各system call, sp複習
* 把 fork 再改的好看一點，也比較好demo解釋(補註解)
* 搞懂 caller, callee 意義還有為啥 trap要存全部，moveto只要存callee，還有複習各regsister在幹啥的細節(elr啥的那些，tp啥地寫進readme)
* 想清楚lab3 advanced的流程並在板板上嘗試
* 搞懂daif在幹嘛還有那個usr prgram轉過去的0x確認一下，還有timer interrutp複習

exec 的時候，可以 malloc 然後複製一個檔案過去
sp 用給定的就好

user program 從現在的thread開始跑，可以寫一個test thread 裡面CALL EXEC 專門FOR這個的

TODOS:
1. uartread, uartwrite V
2. fork V (probably)
3. update thread for user stack (and exec to let the program run correctly) V

NEW TODO!
1. TIMER INTERRUPT
2. PREEMMIT DISABLE IN CRITICAL SECTION
3. GOOD VIDEO (pls QAQ)
4. MBOX CALL V

因為 saveall 才把東西存進來，load all會再跳回去

syscall裡面傳進去的sp
sp 是在 sp_el0 還是 sp_el1 (整個複製以後 load all 才會對?)
el0 只有存地址，然後 sp 是在 trapframe 上面，所以應該是把整個sp以後複製過去，再把過去的x[0]變成0，並且 child sp正確位置是加上 trapframe 之後

compile完登入以後從 linker 的 0x80000開始，是一個 thread 嗎?


所以fork完要
把 kernel 跟 user stack 都放對地方

x8,...那些 register 原本是在 user 還是 kernel stack，複製的時候可以直接複製嗎，還是他們慧根地址有相依姓

fork return 時 trapframe 是否也需要被複製，還是只要返回值對就好?


要用load all的話 sp要在存的東西之上 (不用的話好像沒辦法給那些值?)

先把save all跟load all改成可以用 trapframe 包起來的，再來想其他東西，檢查x9還是x10

真的不行就先準備 thread，然後問 video player 播不出來能不能拿一半QAQ

elrel1那些是0，好像怪怪的，可以檢查一下 那些正常的質應該要是多少


TOMORO
1. 試跑 fork_test
2. 可能可以試試 blr 跟 run thread
3. 把 fork 改成阿倫的試試看 (同邏輯)
4. 不然可能就得試試看別人的 memory allocator

因為打字才會動，加上有從handler檢查，應該是進入interrupt在等輸入，所以開啟可以nested
https://blog.csdn.net/boildoctor/article/details/123379261