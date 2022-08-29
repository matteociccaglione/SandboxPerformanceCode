import math

n = 20
l = 1.541087963
mu_i = 1/30
sum = 0
for i in range(0,n+1):
    sum+=((l/mu_i)**i)/math.factorial(i)
print("sum = ",sum)
p0 = 1/sum
print("p0 = ",p0)
print("lambda/mu = ", l/mu_i)
print("(lambda/mu)^n = ",(l/mu_i)**n)
pLoss = p0*(((l/mu_i)**n)/math.factorial(n))
print("pLoss = ",pLoss)

lPrimo = l*(1-pLoss)

rho = lPrimo/(n*mu_i)

print("lambda' = ",lPrimo)
print("rho = ", rho)
