local n = 1000000000
local sum = 0.0
local flip = -1.0
for i = 1, n do
    --flip = flip * -1.0
    flip = -flip
    --sum = sum + flip / (2.0 * i - 1.0)
    sum = sum + flip / (2 * i - 1)
end
print(string.format("%.24f", sum * 4.0))
