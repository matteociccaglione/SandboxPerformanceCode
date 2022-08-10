import math

sum = 0.0
ro = 0.844224
p0=0.0
m=50
Esi = 150
Esquadro = 26729.7
cquadro = (Esquadro-(Esi)**2)/((Esi)**2)

for i in range(0,m):
    p0+=((m*ro)**i)/math.factorial(i)
pezzo_grosso = ((m*ro)**m)/(math.factorial(m)*(1-ro))

p0+=pezzo_grosso


Pq = pezzo_grosso/p0

Etq = Pq*Esi/((1-ro)*m)

Enq = (ro**2/(2*(1-ro)))*(1+cquadro)
#Etq = Enq*3.026799

print(Etq)

Ets = Etq+Esi

print(Ets)
"""

#Multi server
m=1
Pq1 = 0.0
p1 = 0.585947
ro1 = ro*p1
p0=0.0

for i in range(0,m):
    p0+=((m*ro1)**i)/math.factorial(i)
pezzo_grosso = ((m*ro1)**m)/(math.factorial(m)*(1-ro1))

p0+=pezzo_grosso


Pq1 = pezzo_grosso/p0

print(Pq1)
print(Pq)
Etq1 = Pq1*Esi/(m*(1-ro1))
print(Etq1)

Etq2 = Pq*Esi/(m*(1-ro)*(1-ro1))
print(Etq2)
Etq = p1*Etq1 + (1-p1)*Etq2

Ets = Etq + Esi
print(Etq)
print(Ets)
print(Etq1 + Esi)
"""