
sum = 0.0
flip = -1.0
for i in range(1, 1000000000):
    flip *= -1.0
    sum += flip / (2.0*i - 1.0)
print(sum * 4.0)
