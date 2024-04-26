# 定义矩阵的行和列数
rows = 8
cols = 16
nums = rows * cols
j = 7
i = 7
m = 56
n = 14
# 初始化一个全零矩阵
matrix = [[0] * nums for _ in range(nums)]

# 填充矩阵
for num in range(nums):
    x = num % cols
    y = int((num - x) / cols)

    if (x % 2 == 0):
        matrix[num][num + 1] = i
    else:
        matrix[num][num - 1] = i

    ii = 1
    while ((num - 4 * ii) // cols == y):
        matrix[num][num - 4 * ii] = m
        ii += 1
    ii = 1
    while ((num + 4 * ii) // cols == y):
        matrix[num][num + 4 * ii] = m
        ii += 1

    if (y % 2 == 0):
        matrix[num][num + 16] = n
    else:
        matrix[num][num - 16] = n

    if (num - 64 > -1):
        matrix[num][num - 64] = j

    if (num + 64 < nums):
        matrix[num][num + 64] = j

for row in matrix:
    print(*row, sep=', ')
