# 定义矩阵的行和列数
rows = 16
cols = 16
nums = rows * cols
m = 5
n = 24
# 初始化一个全零矩阵
matrix = [[0] * nums for _ in range(nums)]

matrix[14][224] = 128
matrix[15][225] = 128
matrix[126][240] = 128
matrix[127][241] = 128
for row in matrix:
    print(*row, sep=', ')
