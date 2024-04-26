# 定义矩阵的行和列数
rows = 3
cols = 16
nums = rows * cols
m = 44
n = 21
# 初始化一个全零矩阵
matrix = [[0] * nums for _ in range(nums)]

# 填充矩阵
for num in range(nums):
    x = num % cols
    y = int((num - x) / cols)
    i = 1
    while ((num - 4 * i) // cols == y):
        matrix[num][num - 4 * i] = m
        i += 1
    i = 1
    while ((num + 4 * i) // cols == y):
        matrix[num][num + 4 * i] = m
        i += 1

    if (num - cols > -1):
        matrix[num][num - cols] = n

    if (num + cols < nums):
        matrix[num][num + cols] = n

for row in matrix:
    print(*row, sep=', ')
