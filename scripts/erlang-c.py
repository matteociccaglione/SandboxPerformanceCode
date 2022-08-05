import math
sum = 0.0
ro = 0.952232
p0=0.0
m=45
Esi = 129.699707
Esquadro = 26729.7
cquadro = (Esquadro-(Esi)**2)/((Esi)**2)
for i in range(0,m-1):
    p0+=(i*ro)**i/math.factorial(i)
pezzo_grosso = (m*ro)**m/(math.factorial(m)*(1-ro))

p0+=pezzo_grosso
p0=p0**(-1)

Pq = p0*pezzo_grosso

#Etq = Pq*Esi/((1-ro)*m)

Enq = (ro**2/(2*(1-ro)))*(1+cquadro)
Etq = Enq*3.026799

print(Etq)

Ets = Etq+Esi

print(Ets)