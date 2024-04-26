# 定义矩阵的行和列数
rows = 7
cols = 6
nums = rows * cols
m = 28
n = 21
# 初始化一个全零矩阵
matrix = [[0] * nums for _ in range(nums)]

# 填充矩阵
for num in range(nums):
    x = num % cols
    y = int((num - x) / cols)
    if ((num + cols) < nums):
        matrix[num][num + cols] = n
    if ((num - cols) > -1):
        matrix[num][num - cols] = n

    i = 1
    while ((num - i) // cols == y):
        matrix[num][num - i] = m
        i += 1
    i = 1
    while ((num + i) // cols == y):
        matrix[num][num + i] = m
        i += 1

for row in matrix:
    print(*row, sep=', ')
