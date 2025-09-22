# 🎬 Roulante Réseau Cinéma  

## 📖 Description  
Ce projet vise à créer une **infrastructure réseau compacte et mobile** pour le cinéma et l’événementiel.  
L’objectif est de centraliser le **contrôle DMX et MIDI** via un iPad, en s’appuyant sur un **switch PoE+** et un **point d’accès Wi-Fi 6E**.  

La roulante permet de :  
- Alimenter et connecter des **nœuds DMX PoE** (Cerise U2 Pro).  
- Héberger un **contrôleur MIDI Ethernet** (basé sur Raspberry Pi Pico / PoE splitter).  
- Fournir une connexion **Wi-Fi stable** pour le pilotage.  
- Assurer une alimentation centralisée via PoE pour limiter le câblage.  

---

## 🎯 Objectifs  
- ✅ Réduire le matériel embarqué → un seul switch + un AP Wi-Fi.  
- ✅ Avoir un système **silencieux** et fiable, adapté à un plateau de tournage.  
- ✅ Optimiser les coûts en utilisant du matériel réseau **accessible** (TP-Link / Ubiquiti).  
- ✅ Prévoir une extension future (batterie embarquée, monitoring via routeur).  
- ✅ Intégrer un **univers DMX annexe** pour piloter des **LED custom** via contrôleurs dédiés.  

---

## 🛠️ Étapes du projet  

1. **🟢 Partie Réseau (mise en pause)**  
   - Choix d’un **switch PoE+ manageable** (IGMP Snooping, fanless).  
   - Intégration d’un **AP Wi-Fi 6E PoE+**.  
   - Connexion et alimentation des nœuds DMX et du contrôleur MIDI.  

2. **🔵 Partie MIDI (en cours)**  
   Conception d’un **contrôleur MIDI personnalisé** réparti sur **3 modules** :  
   - 11 faders motorisés,  
   - 166 touches mécaniques,  
   - 6 encodeurs rotatifs.  

   Chaque module est basé sur un **Raspberry Pi Pico** et sera relié en **Ethernet via PoE**.  
   L’objectif est d’avoir un **contrôleur compact, fiable et évolutif**, pensé pour le **plateau de tournage**.  

3. **🔴 Partie Roulante**  
   - Intégration physique du switch, de l’AP Wi-Fi et des nodes DMX.  
   - Organisation compacte et silencieuse pour le tournage.  

4. **🟡 Partie Batterie (à venir)**  
   - Système batterie embarqué (~3 kWh LiFePO₄).  
   - Monitoring en temps réel via interface réseau.  
   - Alimentation : AP, switch, MIDI, nodes, accessoires lumière.  

5. **🟠 Partie Annexe**  
   - Ajout d’un **univers DMX supplémentaire**, dédié aux **contrôleurs LED custom**.  
   - Extension logicielle et matérielle une fois le réseau principal stabilisé.  

---

## 📌 Roadmap  

### 🟢 Réseau  
- [x] Définition des besoins (bande passante, norme Wi-Fi, budget PoE).  
- [x] Sélection du matériel (switch PoE+ manageable, AP Wi-Fi 6E).  
- [ ] Intégration (installation, alimentation PoE, câblage).  
- [ ] Validation (tests DMX/MIDI, stabilité plateau).  

### 🔵 MIDI  
- [ ] Prototype d’un premier module (faders, touches, encodeurs).  
- [ ] Intégration PoE (alimentation + communication via Ethernet/PoE).  
- [ ] Conception finale en 3 modules (11 faders, 166 touches, 6 encodeurs).  
- [ ] Design final (ergonomie et fiabilité tournage).  

### 🔴 Roulante  
- [x] Cahier des charges.
- [ ] Desing achitectural.  
- [ ] Design mécanique (disposition switch, AP, nodes DMX, contrôleur MIDI).  
- [ ] Fabrication et intégration dans la roulante.  

### 🟡 Batterie  
- [ ] Étude énergétique (besoins réels : switch, AP, MIDI, DMX, accessoires).  
- [ ] Prototype cellule LiFePO₄.  
- [ ] Construction de la batterie finale (~3 kWh) + monitoring réseau.  

### 🟠 Annexe  
- [ ] Ajout d’un univers DMX dédié aux contrôleurs LED custom.  
- [ ] Intégration dans le réseau principal.  
- [ ] Tests sur plateau (réactivité, stabilité).  

---

## 🛠️ Technos utilisées  
- **Réseau** : Ethernet, PoE+ (802.3at), IGMP Snooping (sACN).  
- **Wi-Fi** : Wi-Fi 6E (2,4 / 5 / 6 GHz), PoE+.  
- **DMX** : Nodes Cerise U2 Pro (PoE).  
- **MIDI** : Raspberry Pi Pico, drivers moteurs (DRV8871 / L298N), faders motorisés, encodeurs, touches mécaniques.  
- **Énergie** : Batterie LiFePO₄, monitoring réseau, alimentation centralisée via PoE.  

---
