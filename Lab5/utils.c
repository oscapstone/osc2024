void delay(int time){
    while(time--) { 
        asm volatile("nop"); 
    }
}