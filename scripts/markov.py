import math

n = 20
l = 1.541087
mu = 1/30
sum = 0
for i in range(0,n+1):
    sum+=((l/mu)**i)/math.factorial(i)
print("sum = ",sum)
p0 = 1/sum
print("p0 = ",p0)
print("lambda/mu = ", l/mu)
print("(lambda/mu)^n = ",(l/mu)**n)
pLoss = p0*(((l/mu)**n)/math.factorial(n))
print("pLoss = ",pLoss)

lPrimo = l*(1-pLoss)

rho = lPrimo/(n*mu)

print("lambda' = ",lPrimo)
print("rho = ", rho)
