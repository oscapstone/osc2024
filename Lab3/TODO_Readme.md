### TODOs
* exception demo
* basic2 demo
* uart_hex_long
* uart_int

可以直接在irq handler裡面call shell來跑程式，實現真正的async，也可以用gpt的寫法，確實是async。

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

Demo:
可以分開de各項目 (先用basic kernel跑 run 跟 timer，再用最後一個kernel跑uart跟advanced)
uart跟advanced 1 部分可以先講 純async的，de到task再改用task來講

Demo 問:
到底啥時要asm，為啥有些地方block了就不能用了 (or準備demo時想辦法搞懂)
run完到底要不要跑
uart 貼上只能八個

Demo完:
整理code、補知識 (也可休息時補)


Demo 順序:
1. Basic 1與Basic 2輪流 (記得Basic 1要重compile確認會關interrupt)
2. Advanced 1 先 (async 前)，之後async (basic 3)，ok以後再按buf，如果要求要看clock再st看一下。