void delay(int time){
    while(time--) { 
        asm volatile("nop"); 
    }
}

void memset(char * t, int size){
    for(int i=0;i<size;i++){
        t[i] = 0;
    }
}