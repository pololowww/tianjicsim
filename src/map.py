# 定义矩阵的行和列数
rows = 4
cols = 8
nums = rows * cols
m = 37
n = 37
# 初始化一个全零矩阵
matrix = [[0] * nums for _ in range(nums)]

# 填充矩阵
for num in range(nums):
    x = num % cols
    y = int((num - x) / cols)
    if (x % 2 == 0):
        matrix[num][num + 1] = m
    else:
        matrix[num][num - 1] = n

    i = 1
    while (num - i * cols > -1):
        matrix[num][num - i * cols] = n
        i = i + 1

    i = 1
    while (num + i * cols < nums):
        matrix[num][num + i * cols] = n
        i = i + 1

for row in matrix:
    print(*row, sep=', ')
