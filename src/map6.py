# 定义矩阵的行和列数
rows = 4
cols = 16
nums = rows * cols
m = 23
n = 92
# 初始化一个全零矩阵
matrix = [[0] * nums for _ in range(nums)]

# 填充矩阵
for num in range(nums):
    x = num % cols
    y = int((num - x) / cols)
    i = 1
    while ((num - i) // 4 == num // 4):
        matrix[num][num - i] = m
        i += 1
    i = 1
    while ((num + i) // 4 == num // 4):
        matrix[num][num + i] = m
        i += 1

    i = 1
    while ((num - cols * i) > -1):
        matrix[num][num - cols * i] = n
        i += 1
    i = 1
    while ((num + cols * i) < nums):
        matrix[num][num + cols * i] = n
        i += 1

for row in matrix:
    print(*row, sep=', ')
