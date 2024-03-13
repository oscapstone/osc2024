# lab0

## 替換使用的編譯器
使用別名通常可用於長時間使用cross compilor，更換版本則只建議用alternatives

### 替換gcc版本
`sudo update-alternatives --set gcc /usr/bin/gcc-X`

### 直接使用別名
```
vim ~/.bashrc
alias gcc='aarch64-linux-gnu-gcc' // append this into the file
source ~/.bashrc
```

## 使用symbolic link
`sudo ln -s /usr/bin/aarch64-linux-gnu-gcc /usr/local/bin/gcc`

## 使用wsl2連接