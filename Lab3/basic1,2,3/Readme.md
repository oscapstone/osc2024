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