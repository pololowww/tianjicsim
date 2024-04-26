# 定义矩阵的行和列数
rows = 3
cols = 12
nums = rows * cols
m = 12
n = 21
# 初始化一个全零矩阵
matrix = [[0] * nums for _ in range(nums)]

# 填充矩阵
for num in range(nums):
    x = num % cols
    y = int((num - x) / cols)
    if (x == 0):
        matrix[num][num + 1] = m
    elif (x == cols - 1):
        matrix[num][num - 1] = m
    else:
        matrix[num][num + 1] = m
        matrix[num][num - 1] = m

    if (y == 0):
        matrix[num][num + cols] = n
    elif (y == rows - 1):
        matrix[num][num - cols] = n
    else:
        matrix[num][num + cols] = n
        matrix[num][num - cols] = n

for row in matrix:
    print(*row, sep=', ')
