n = 20
l = 1.541087
mu = n/30
sum = 0
for i in range(0,n+1):
    sum+=(l/mu)**i
print(sum)
p0 = 1/sum
print(p0)
print((l/mu))
print((l/mu)**n)
pLoss = p0*(l/mu)**n
print(pLoss)

lPrimo = l*(1-pLoss)

ro = lPrimo/mu

print("lambda' = ",lPrimo)
print("ro = ", ro)
