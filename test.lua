local sum = 0.0
local flip = -1.0
for i = 1, 1000000000 do
    flip = flip * -1.0
    sum = sum + flip / (2.0 * i - 1.0)
end
print(sum * 4.0)
