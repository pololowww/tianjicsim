# 定义矩阵的行和列数
rows = 2
cols = 14
nums = rows * cols
m = 3
n = 21
# 初始化一个全零矩阵
matrix = [[0] * nums for _ in range(nums)]

# 填充矩阵
for num in range(nums):
    x = num % cols
    y = int((num - x) / cols)
    if (y == 0):
        matrix[num][num + cols] = m
    else:
        matrix[num][num - cols] = m

    if (x % 2 == 0):
        matrix[num][num + 1] = n
    else:
        matrix[num][num - 1] = n

for row in matrix:
    print(*row, sep=', ')
