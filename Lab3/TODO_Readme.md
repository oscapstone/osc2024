### TODOs

On 板子:
* run完打指令怪怪的(要先打一次)
* help 的那行會消失
* 沒有 \r
上面問題的原因是我先在read的時候處理了 shell 才去 write

接下來要:
* 最後一個(全部) 用cpio20000000重編、處裡\r的問題
* 可試著改write再跑code解決那個顯示問題
* 準備demo (搞懂啥時blocK還有所有咚咚 (細節都要，cause最後一個估計會被扣分) )
* 記得再compile一次disable interrupt

* 記得要把 跳到user program的指令改成0x3c0 而非0x3c5!
* 可以試試看關掉timer的asm(但感覺不用)

Demo:
Step1: Basic 1,2 in kernel basic 1, 2
Step2: Advanced 1, then basic 3, advanced 2 (or use Basic 3 Adv 1, then the kernel with advanced 2)
可以預備一個沒有task queue的，如果task queue怪怪的改用這個

可以分開de各項目 (先用basic kernel跑 run 跟 timer，再用最後一個kernel跑uart跟advanced)
先de 3個 timer (順序: 2, 1, 3)
再de async，證明可以uart，最後 一個短timer配狂按 證明有queue
uart跟advanced 1 部分可以先講 純async的，de到task再改用task來講
st不要67(可問助教)

Demo 問:
到底啥時要asm，為啥有些地方block了就不能用了 (or準備demo時想辦法搞懂)
uart 貼上只能八個

Demo完:
整理code、補知識 (也可休息時補)


Demo 順序:
1. Basic 1與Basic 2輪流 (記得Basic 1要重compile確認會關interrupt)
2. Advanced 1 先 (async 前)，之後async (basic 3)，ok以後再按buf，如果要求要看clock再st看一下。