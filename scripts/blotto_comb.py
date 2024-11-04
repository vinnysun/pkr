import math


N = 3
S = 5

print('math.comb:', math.comb(S + 2, N - 1))



counts = 0
for i in range(6):
    for j in range(6):
        for k in range(6):
            if i + j + k == 5:
                counts += 1
                print(i, j, k)
print(counts)
