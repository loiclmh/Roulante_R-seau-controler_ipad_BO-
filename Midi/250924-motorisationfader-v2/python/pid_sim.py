# pid_sim.py
import numpy as np
import matplotlib.pyplot as plt

# ===== Réglages PID & simu =====
Kp = 3.0
Ki = 0.0
Kd = 0.2
fc = 150.0          # Hz, coupure filtre dérivé (EMA)
Ts = 0.001          # s, période (1 kHz)
Tsim = 2.0          # s, durée de la simulation
setpoint = 1.0      # cible (position normalisée 0..1)
u_max = 1.0         # saturation commande

# ===== Modèle simple moteur+fader (2e ordre) =====
# x'' = -(b/J) * x' + (K/J) * u
J = 1.0    # inertie
b = 2.0    # frottement
Kmot = 10.0

N = int(Tsim / Ts)
t = np.arange(N) * Ts
x = np.zeros(N)       # position
v = np.zeros(N)       # vitesse
u_hist = np.zeros(N)  # commande appliquée

# ===== PID discret avec dérivée sur la mesure (filtrée) =====
fs = 1.0 / Ts
alpha = (2*np.pi*fc) / (2*np.pi*fc + fs)  # EMA
integral = 0.0
e_prev = 0.0
meas_prev = 0.0
measdot_f = 0.0

for k in range(1, N):
    meas = x[k-1]
    e = setpoint - meas

    # dérivée sur la mesure (approx) : (meas - meas_prev)/Ts
    measdot = (meas - meas_prev) / Ts
    meas_prev = meas

    # filtrage EMA
    measdot_f = alpha * measdot + (1 - alpha) * measdot_f

    # intégrale (anti-windup simple: seulement si pas saturé après coup précédent)
    integral += e * Ts

    # sortie PID (note: dérivée sur mesure => signe négatif)
    u = Kp*e + Ki*integral - Kd*measdot_f

    # saturation + anti-windup (arrête l’intégrale si ça sature fort)
    if u > u_max:
        u = u_max
        # option: integral -= e * Ts  # si besoin de geler plus fort
    elif u < -u_max:
        u = -u_max
        # option: integral -= e * Ts

    # dynamique du système (Euler)
    a = ( - (b/J) * v[k-1] + (Kmot/J) * u )
    v[k] = v[k-1] + a * Ts
    x[k] = x[k-1] + v[k] * Ts
    u_hist[k] = u

# ===== Traces =====
plt.figure()
plt.plot(t, np.full_like(t, setpoint), linestyle='--', label='Consigne')
plt.plot(t, x, label='Position')
plt.xlabel('Temps (s)')
plt.ylabel('Position')
plt.legend()
plt.grid(True)

plt.figure()
plt.plot(t, u_hist, label='Commande u (saturée)')
plt.xlabel('Temps (s)')
plt.ylabel('u')
plt.legend()
plt.grid(True)

plt.show()
