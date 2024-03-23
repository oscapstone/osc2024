with open('/dev/pts/33', 'wb', buffering=0) as tty:
    n = tty.write('jo'.encode())
    print(n)
    # b = tty.read()
    # print(b)
