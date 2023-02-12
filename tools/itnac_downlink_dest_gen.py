import sys
res = ""

for i in range(100):
    res += f" host[{i}](ipv6)"

with open('dest_addr.txt', 'a+') as file:
    file.write(res)
