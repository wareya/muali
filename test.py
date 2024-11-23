
sum = 0.0
flip = -1.0
for i in range(1, 1000000001):
    flip *= -1.0
    sum += flip / (2*i - 1)
print(sum * 4.0)
